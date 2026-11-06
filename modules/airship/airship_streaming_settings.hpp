/**************************************************************************/
/*  airship_streaming_settings.hpp                                        */
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

#include "core/object/object.h"
#include "core/string/ustring.h"

// The `platform/*` project settings that configure the cloud streaming
// pipelines. They are declared here (and registered from `register_types.cpp`
// at `MODULE_INITIALIZATION_LEVEL_SCENE`) so that every pipeline reads the
// same values from a single place.
class AirshipStreamingSettings {
public:
	// Global install budget.
	static int64_t install_budget_usec;
	static int64_t install_budget_mb;
	static int64_t decoded_memory_mb;

	// Per-pipeline tuning. Each pipeline reads the group matching its
	// `AirshipStreamingBudget::Kind`.
	static int64_t textures_cache_size_mb;
	static int64_t textures_concurrent_downloads;
	static int64_t textures_decode_workers;
	static int64_t textures_installs_per_frame;
	static int64_t textures_uploads_per_frame;
	static bool textures_export_enabled;

	static int64_t audio_cache_size_mb;
	static int64_t audio_concurrent_downloads;
	static int64_t audio_decode_workers;
	static int64_t audio_installs_per_frame;
	static bool audio_export_enabled;

	static int64_t meshes_cache_size_mb;
	static int64_t meshes_concurrent_downloads;
	static int64_t meshes_decode_workers;
	static int64_t meshes_installs_per_frame;
	static bool meshes_export_enabled;

	static int64_t buffers_cache_size_mb;
	static int64_t buffers_concurrent_downloads;
	static int64_t buffers_decode_workers;
	static int64_t buffers_installs_per_frame;
	static bool buffers_export_enabled;

	// Reads the value back from ProjectSettings so that a change made at
	// runtime is picked up without a restart.
	static void reload();

	static int64_t get_cache_size_mb(int p_kind);
	static int64_t get_concurrent_downloads(int p_kind);
	static int64_t get_decode_workers(int p_kind);
	static int64_t get_installs_per_frame(int p_kind);
	static int64_t get_uploads_per_frame(int p_kind);
	static bool is_export_enabled(int p_kind);

	AirshipStreamingSettings() {}
};

// Hard ceiling on the number of installs a single frame may perform,
// independent of the per-frame byte and microsecond budgets.
#define MAX_INSTALLS_PER_FRAME 64
