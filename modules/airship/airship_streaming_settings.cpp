/**************************************************************************/
/*  airship_streaming_settings.cpp                                        */
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

#include "./airship_streaming_settings.hpp"

#include "core/config/project_settings.h"

int64_t AirshipStreamingSettings::install_budget_usec = 2000;
int64_t AirshipStreamingSettings::install_budget_mb = 8;
int64_t AirshipStreamingSettings::decoded_memory_mb = 64;

int64_t AirshipStreamingSettings::textures_cache_size_mb = 1024;
int64_t AirshipStreamingSettings::textures_concurrent_downloads = 4;
int64_t AirshipStreamingSettings::textures_decode_workers = 2;
int64_t AirshipStreamingSettings::textures_installs_per_frame = 64;
int64_t AirshipStreamingSettings::textures_uploads_per_frame = 64;
bool AirshipStreamingSettings::textures_export_enabled = false;

int64_t AirshipStreamingSettings::audio_cache_size_mb = 512;
int64_t AirshipStreamingSettings::audio_concurrent_downloads = 4;
int64_t AirshipStreamingSettings::audio_decode_workers = 2;
int64_t AirshipStreamingSettings::audio_installs_per_frame = 64;
bool AirshipStreamingSettings::audio_export_enabled = true;

int64_t AirshipStreamingSettings::meshes_cache_size_mb = 1024;
int64_t AirshipStreamingSettings::meshes_concurrent_downloads = 4;
int64_t AirshipStreamingSettings::meshes_decode_workers = 2;
int64_t AirshipStreamingSettings::meshes_installs_per_frame = 64;
bool AirshipStreamingSettings::meshes_export_enabled = true;

int64_t AirshipStreamingSettings::buffers_cache_size_mb = 256;
int64_t AirshipStreamingSettings::buffers_concurrent_downloads = 4;
int64_t AirshipStreamingSettings::buffers_decode_workers = 2;
int64_t AirshipStreamingSettings::buffers_installs_per_frame = 64;
bool AirshipStreamingSettings::buffers_export_enabled = true;

void AirshipStreamingSettings::reload() {
	// The cached statics above only hold the registered defaults; every
	// consumer reads the live value through ProjectSettings instead, so there
	// is nothing to refresh here. Kept as an explicit hook so that the
	// settings can later be given a cache with proper invalidation.
}

int64_t AirshipStreamingSettings::get_cache_size_mb(int p_kind) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	switch (p_kind) {
		case 0:
			return ps->get_setting("platform/textures/cache_size_mb", textures_cache_size_mb);
		case 1:
			return ps->get_setting("platform/audio/cache_size_mb", audio_cache_size_mb);
		case 2:
			return ps->get_setting("platform/meshes/cache_size_mb", meshes_cache_size_mb);
		case 3:
			return ps->get_setting("platform/buffers/cache_size_mb", buffers_cache_size_mb);
	}
	return 0;
}

int64_t AirshipStreamingSettings::get_concurrent_downloads(int p_kind) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	switch (p_kind) {
		case 0:
			return ps->get_setting("platform/textures/concurrent_downloads", textures_concurrent_downloads);
		case 1:
			return ps->get_setting("platform/audio/concurrent_downloads", audio_concurrent_downloads);
		case 2:
			return ps->get_setting("platform/meshes/concurrent_downloads", meshes_concurrent_downloads);
		case 3:
			return ps->get_setting("platform/buffers/concurrent_downloads", buffers_concurrent_downloads);
	}
	return 0;
}

int64_t AirshipStreamingSettings::get_decode_workers(int p_kind) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	switch (p_kind) {
		case 0:
			return ps->get_setting("platform/textures/decode_workers", textures_decode_workers);
		case 1:
			return ps->get_setting("platform/audio/decode_workers", audio_decode_workers);
		case 2:
			return ps->get_setting("platform/meshes/decode_workers", meshes_decode_workers);
		case 3:
			return ps->get_setting("platform/buffers/decode_workers", buffers_decode_workers);
	}
	return 0;
}

int64_t AirshipStreamingSettings::get_installs_per_frame(int p_kind) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	switch (p_kind) {
		case 0:
			return ps->get_setting("platform/textures/uploads_per_frame", textures_installs_per_frame);
		case 1:
			return ps->get_setting("platform/audio/installs_per_frame", audio_installs_per_frame);
		case 2:
			return ps->get_setting("platform/meshes/installs_per_frame", meshes_installs_per_frame);
		case 3:
			return ps->get_setting("platform/buffers/installs_per_frame", buffers_installs_per_frame);
	}
	return 0;
}

int64_t AirshipStreamingSettings::get_uploads_per_frame(int p_kind) {
	return get_installs_per_frame(p_kind);
}

bool AirshipStreamingSettings::is_export_enabled(int p_kind) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	switch (p_kind) {
		case 0:
			return ps->get_setting("platform/textures/export_enabled", textures_export_enabled);
		case 1:
			return ps->get_setting("platform/audio/export_enabled", audio_export_enabled);
		case 2:
			return ps->get_setting("platform/meshes/export_enabled", meshes_export_enabled);
		case 3:
			return ps->get_setting("platform/buffers/export_enabled", buffers_export_enabled);
	}
	return false;
}
