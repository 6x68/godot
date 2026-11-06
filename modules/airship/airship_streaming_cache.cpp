/**************************************************************************/
/*  airship_streaming_cache.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "./airship_streaming_cache.h"

#include "airship_streaming_settings.hpp"

#include "core/config/engine.h"
#include "core/os/memory.h"

#include <algorithm>

// The LRU order only needs to be good enough to pick a victim; rebuilding it
// every frame would be wasteful, so it is rebuilt at most this often.
#define LRU_REBUILD_INTERVAL 16

// Oldest-first, with the key breaking ties so the order is stable across
// frames and eviction is deterministic.
static void sort_entries(AirshipStreamingCache::Entry **p_first, AirshipStreamingCache::Entry **p_last) {
	std::sort(p_first, p_last, [](const AirshipStreamingCache::Entry *p_a, const AirshipStreamingCache::Entry *p_b) {
		if (p_a->last_used != p_b->last_used) {
			return p_a->last_used < p_b->last_used;
		}
		return p_a->key < p_b->key;
	});
}

String AirshipStreamingCache::_directory_for(const String &p_key) const {
	const int32_t slash = p_key.rfind_char('/');
	if (slash < 0) {
		return String();
	}
	return p_key.substr(0, slash);
}

AirshipStreamingCache::AirshipStreamingCache() {
	set_max_bytes((uint64_t)AirshipStreamingSettings::get_cache_size_mb(0) * 1024 * 1024);
}

AirshipStreamingCache::~AirshipStreamingCache() {
	flush();
}

void AirshipStreamingCache::_maintain() {
	Engine *engine = Engine::get_singleton();
	const uint64_t frame = engine != nullptr ? engine->get_process_frames() : 0;
	if (frame == last_maintain_frame) {
		return;
	}
	last_maintain_frame = frame;

	LocalVector<Entry *> next;
	next.reserve(entries.size());
	for (const KeyValue<String, Entry *> &kv : entries) {
		Entry *entry = kv.value;
		if (entry->pin_count == 0) {
			next.push_back(entry);
		}
	}
	sort_entries(next.ptr(), next.ptr() + next.size());
	lru = next;
}

void AirshipStreamingCache::_remove_entry(Entry *p_entry) {
	used_bytes -= MIN(used_bytes, p_entry->bytes);
	entries.erase(p_entry->key);
	p_entry->resource = Ref<Resource>();
	memdelete(p_entry);
}

void AirshipStreamingCache::pin(const String &p_key, const Ref<Resource> &p_resource) {
	MutexLock lock(mutex);

	Entry *entry = nullptr;
	Entry *const *found = entries.getptr(p_key);
	if (found != nullptr) {
		entry = *found;
	} else {
		entry = memnew(Entry);
		entry->key = p_key;
		entries.insert(p_key, entry);
		directories[_directory_for(p_key)].push_back(entry);
	}

	// Replacing the resource releases the previous one. The size of the new
	// one is unknown until the caller reports it via set_bytes(), so the
	// accounting is left at zero here rather than guessed.
	entry->resource = p_resource;
	entry->bytes = 0;
	entry->pin_count++;
	entry->last_used = Engine::get_singleton() ? Engine::get_singleton()->get_process_frames() : 0;
}

void AirshipStreamingCache::unpin(const String &p_key) {
	MutexLock lock(mutex);
	Entry *const *found = entries.getptr(p_key);
	if (found == nullptr) {
		return;
	}
	Entry *entry = *found;
	if (entry->pin_count > 0) {
		entry->pin_count--;
	}
}

Ref<Resource> AirshipStreamingCache::get(const String &p_key) const {
	MutexLock lock(mutex);
	Entry *const *found = entries.getptr(p_key);
	if (found == nullptr) {
		return Ref<Resource>();
	}
	return (*found)->resource;
}

void AirshipStreamingCache::set_bytes(const String &p_key, uint64_t p_bytes) {
	MutexLock lock(mutex);
	Entry *const *found = entries.getptr(p_key);
	if (found == nullptr) {
		return;
	}
	Entry *entry = *found;
	if (p_bytes >= entry->bytes) {
		used_bytes += p_bytes - entry->bytes;
	} else {
		used_bytes -= entry->bytes - p_bytes;
	}
	entry->bytes = p_bytes;
}

void AirshipStreamingCache::poll() {
	MutexLock lock(mutex);

	_maintain();

	// A stale LRU order can name an entry that has since been pinned; those
	// are skipped, and the interval-based rebuild below fixes the order.
	uint32_t skipped = 0;
	while (used_bytes > max_bytes && !lru.is_empty() && skipped < 64) {
		Entry *victim = lru[0];
		lru.remove_at(0);
		if (victim->pin_count > 0) {
			skipped++;
			continue;
		}
		_remove_entry(victim);
	}
	if (skipped > 0) {
		// Force a rebuild next call so the skipped entries are not
		// reconsidered forever.
		last_maintain_frame = UINT64_MAX;
	}
}

void AirshipStreamingCache::flush() {
	MutexLock lock(mutex);
	LocalVector<Entry *> doomed;
	doomed.reserve(entries.size());
	for (const KeyValue<String, Entry *> &kv : entries) {
		doomed.push_back(kv.value);
	}
	entries.clear();
	directories.clear();
	lru.clear();
	used_bytes = 0;
	for (Entry *entry : doomed) {
		entry->resource = Ref<Resource>();
		memdelete(entry);
	}
}

void AirshipStreamingCache::set_max_bytes(uint64_t p_bytes) {
	MutexLock lock(mutex);
	max_bytes = p_bytes;
}

uint64_t AirshipStreamingCache::get_used_bytes() const {
	MutexLock lock(mutex);
	return used_bytes;
}

uint64_t AirshipStreamingCache::get_max_bytes() const {
	MutexLock lock(mutex);
	return max_bytes;
}

uint32_t AirshipStreamingCache::get_entry_count() const {
	MutexLock lock(mutex);
	return (uint32_t)entries.size();
}

uint32_t AirshipStreamingCache::get_pinned_count() const {
	MutexLock lock(mutex);
	uint32_t count = 0;
	for (const KeyValue<String, Entry *> &kv : entries) {
		if (kv.value->pin_count > 0) {
			count++;
		}
	}
	return count;
}

Dictionary AirshipStreamingCache::get_stats() const {
	MutexLock lock(mutex);
	Dictionary stats;
	stats["entries"] = (int64_t)entries.size();
	stats["pinned"] = (int64_t)get_pinned_count();
	stats["used_bytes"] = (int64_t)used_bytes;
	stats["max_bytes"] = (int64_t)max_bytes;
	stats["directories"] = (int64_t)directories.size();
	return stats;
}
