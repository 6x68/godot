/**************************************************************************/
/*  airship_streaming_cache.h                                             */
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

#pragma once

#include "core/io/resource.h"
#include "core/object/ref_counted.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/variant/dictionary.h"

class String;

// Tracks which payloads are worth keeping resident and evicts the ones that
// have not been used for a while.
//
// Every entry starts out *unpinned*: it is a valid cache candidate and will be
// dropped by `poll()` once the cache grows past its byte budget. A consumer
// that needs a payload to stay resident (because it is read every frame, e.g.
// a material's current albedo) calls `pin()`, which takes it out of the
// eviction running entirely. `unpin()` puts it back.
//
// Eviction is driven from `poll()`, which the pipelines call once per frame
// from `_process()`.
class AirshipStreamingCache {
public:
	// A single cached payload.
	struct Entry {
		String key; // Cache key, e.g. "textures/sky_d".
		Ref<Resource> resource; // The resident payload, may be null.
		uint64_t bytes = 0; // Approximate resident size, for budgeting.
		uint64_t last_used = 0; // Frame counter of the last pin()/store().
		uint32_t pin_count = 0; // > 0 means "do not evict".
	};

private:
	mutable Mutex mutex;

	// key -> entry.
	HashMap<String, Entry *> entries;
	// Directory name -> the entries whose key lives in it, so a whole
	// directory can be dropped when a package is unloaded.
	HashMap<String, LocalVector<Entry *>> directories;

	// Unpinned entries ordered oldest-first, used to pick eviction victims.
	LocalVector<Entry *> lru;

	uint64_t max_bytes = 0;
	uint64_t used_bytes = 0;

	// Frame the LRU order was last rebuilt for. Rebuilding every frame is
	// wasteful when nothing is being evicted.
	uint64_t last_maintain_frame = UINT64_MAX;

	void _maintain();
	void _remove_entry(Entry *p_entry);
	String _directory_for(const String &p_key) const;

public:
	AirshipStreamingCache();
	~AirshipStreamingCache();

	// Takes `p_resource` out of the eviction running. `p_key` identifies it;
	// pinning a second resource under an already-pinned key replaces the
	// previous one.
	void pin(const String &p_key, const Ref<Resource> &p_resource);

	// Allows `p_key` to be evicted again.
	void unpin(const String &p_key);

	// Returns the resident payload for `p_key`, or an empty Ref.
	Ref<Resource> get(const String &p_key) const;

	// Records how many bytes are resident for `p_key`, so that the budget
	// accounts for a payload that grew after being pinned.
	void set_bytes(const String &p_key, uint64_t p_bytes);

	// Evicts unpinned entries, oldest first, until the cache fits in its
	// byte budget. Safe to call every frame.
	void poll();

	// Drops every entry, pinned or not. Used when a package is unloaded and
	// the whole cache becomes invalid.
	void flush();

	// Applies a new byte budget; evicts immediately if already over it.
	void set_max_bytes(uint64_t p_bytes);

	uint64_t get_used_bytes() const;
	uint64_t get_max_bytes() const;
	uint32_t get_entry_count() const;
	uint32_t get_pinned_count() const;

	// Debug/statistics dictionary, bound to script as `get_stats()`.
	Dictionary get_stats() const;
};
