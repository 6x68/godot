/**************************************************************************/
/*  airship_streaming_budget.h                                            */
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

#include "core/os/mutex.h"

// Global, thread-safe accounting for how much memory and how much per-frame
// time the cloud streaming pipelines may spend installing decoded payloads.
//
// Two independent limits are enforced:
//
//  * `platform/streaming/decoded_memory_mb` caps the total number of bytes of
//    decoded payload that may be resident at any one time. Pipelines claim
//    room with `reserve()` and hand it back with `release()`; a payload that
//    grows after being installed is adjusted with `resize()`.
//
//  * `platform/streaming/install_budget_mb` caps how many bytes may be
//    installed during a single frame, and each pipeline's own
//    `installs_per_frame` caps how many payloads it may install per frame.
//    Pipelines ask `can_install()` before starting an install and report the
//    finished cost with `installed()`.
//
// Both sets of counters roll over lazily: the per-frame counters are reset by
// `refresh_frame()` the first time it is called after `Engine`'s process
// frame counter has advanced, so a long stall cannot let a huge backlog
// through.
class AirshipStreamingBudget {
public:
	enum Kind {
		KIND_TEXTURE,
		KIND_AUDIO,
		KIND_MESH,
		KIND_BUFFER,
		KIND_COUNT,
	};

private:
	mutable Mutex mutex;

	// Settings, re-read by refresh_frame() once per frame.
	uint64_t decoded_memory_limit = 0; // Bytes.
	uint64_t frame_install_budget = 0; // Bytes.
	uint64_t kind_frame_install_budget = 0; // Bytes, per kind.
	uint32_t kind_installs_per_frame[KIND_COUNT] = {};

	// Per-frame state, cleared by refresh_frame().
	uint32_t frame_install_count = 0;
	uint64_t frame_reserved_bytes = 0;
	uint64_t frame_reserved_usec = 0;
	uint64_t last_frame = 0;

	// Persistent state.
	uint64_t reserved_bytes = 0;

	static AirshipStreamingBudget *singleton;

	void refresh_frame();

public:
	AirshipStreamingBudget() {}
	~AirshipStreamingBudget() {}

	// Only the singleton lifecycle is public; the budget cannot be created
	// by accident and has to be reached through get_singleton().
	static void initialize();
	static void uninitialize();
	static AirshipStreamingBudget *get_singleton() { return singleton; }

	// Attempts to claim `p_bytes` of the decoded memory allowance. On success
	// the caller must eventually call `release()` with the same reference it
	// stores the reservation in.
	bool reserve(uint64_t &r_reserved, uint64_t p_bytes);

	// Grows or shrinks an existing reservation in place. Returns false and
	// leaves `r_reserved` untouched when the new size does not fit.
	bool resize(uint64_t &r_reserved, uint64_t p_bytes);

	// Returns the allowance claimed by `r_reserved` to the pool.
	void release(uint64_t &r_reserved);

	// True when another install of `p_bytes` for `p_kind` may be started this
	// frame. Checked by every pipeline's `_process()` immediately before an
	// install.
	bool can_install(Kind p_kind, uint64_t p_bytes);

	// Records a finished install so that the per-frame budget is consumed.
	void installed(Kind p_kind, uint64_t p_bytes, uint64_t p_usec);

	// Installs recorded on the current frame, exposed for the debug stats.
	uint64_t get_reserved() const;
	uint32_t get_frame_install_count() const;
};
