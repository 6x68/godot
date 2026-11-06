/**************************************************************************/
/*  airship_streaming_budget.cpp                                          */
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

#include "./airship_streaming_budget.hpp"

#include "airship_streaming_settings.hpp"

#include "core/config/engine.h"
#include "core/os/memory.h"

AirshipStreamingBudget *AirshipStreamingBudget::singleton = nullptr;

void AirshipStreamingBudget::initialize() {
	ERR_FAIL_NULL(singleton);
	singleton = memnew(AirshipStreamingBudget);
}

void AirshipStreamingBudget::uninitialize() {
	memdelete(singleton);
	singleton = nullptr;
}

void AirshipStreamingBudget::refresh_frame() {
	// Re-read the project settings once per frame so that a settings change
	// takes effect without requiring a restart, and roll the per-frame
	// counters over.
	decoded_memory_limit = (uint64_t)AirshipStreamingSettings::decoded_memory_mb * 1024 * 1024;
	frame_install_budget = (uint64_t)AirshipStreamingSettings::install_budget_mb * 1024 * 1024;
	// Each pipeline gets an equal slice of the frame install budget.
	kind_frame_install_budget = frame_install_budget / KIND_COUNT;
	for (int i = 0; i < KIND_COUNT; i++) {
		kind_installs_per_frame[i] = AirshipStreamingSettings::get_installs_per_frame((Kind)i);
	}

	frame_install_count = 0;
	frame_reserved_bytes = 0;
	frame_reserved_usec = 0;
}

bool AirshipStreamingBudget::reserve(uint64_t &r_reserved, uint64_t p_bytes) {
	if (p_bytes == 0) {
		r_reserved = 0;
		return true;
	}
	MutexLock lock(mutex);
	refresh_frame();
	if (reserved_bytes == 0) {
		reserved_bytes = p_bytes;
		r_reserved = p_bytes;
		return true;
	}
	// `reserved_bytes` is non-zero, so the allowance has been read this
	// frame and the limit is meaningful.
	if (decoded_memory_limit < p_bytes || decoded_memory_limit - p_bytes < reserved_bytes) {
		return false;
	}
	reserved_bytes += p_bytes;
	r_reserved = p_bytes;
	return true;
}

bool AirshipStreamingBudget::resize(uint64_t &r_reserved, uint64_t p_bytes) {
	MutexLock lock(mutex);
	refresh_frame();
	uint64_t others = reserved_bytes - r_reserved;
	if (reserved_bytes == r_reserved) {
		// First reservation of this frame: there is no meaningful limit yet.
		r_reserved = p_bytes;
		reserved_bytes = p_bytes + others;
		return true;
	}
	if (decoded_memory_limit < p_bytes || decoded_memory_limit - p_bytes < others) {
		return false;
	}
	r_reserved = p_bytes;
	reserved_bytes = p_bytes + others;
	return true;
}

void AirshipStreamingBudget::release(uint64_t &r_reserved) {
	MutexLock lock(mutex);
	if (r_reserved > reserved_bytes) {
		reserved_bytes = 0;
	} else {
		reserved_bytes -= r_reserved;
	}
	r_reserved = 0;
}

bool AirshipStreamingBudget::can_install(Kind p_kind, uint64_t p_bytes) {
	MutexLock lock(mutex);
	refresh_frame();
	if (kind_installs_per_frame[p_kind] == 0) {
		// This pipeline installs nothing this frame.
		return true;
	}
	if (frame_install_count >= MAX_INSTALLS_PER_FRAME) {
		return false;
	}
	if (frame_reserved_bytes >= kind_frame_install_budget) {
		return false;
	}
	if (frame_install_budget < p_bytes || frame_install_budget - p_bytes < frame_reserved_bytes) {
		return false;
	}
	return true;
}

void AirshipStreamingBudget::installed(Kind p_kind, uint64_t p_bytes, uint64_t p_usec) {
	MutexLock lock(mutex);
	frame_install_count++;
	frame_reserved_bytes += p_bytes;
	frame_reserved_usec += p_usec;
}

uint64_t AirshipStreamingBudget::get_reserved() const {
	MutexLock lock(mutex);
	return reserved_bytes;
}

uint32_t AirshipStreamingBudget::get_frame_install_count() const {
	MutexLock lock(mutex);
	return frame_install_count;
}
