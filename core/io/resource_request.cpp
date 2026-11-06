/**************************************************************************/
/*  resource_request.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            GODOT ENGINE                               */
/*                       https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-present Juan Linietsky, Ariel Manzur.             */
/**************************************************************************/
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
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "resource_request.h"

#include "core/error/error_macros.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

void ResourceRequest::_bind_methods() {
	ClassDB::bind_static_method("ResourceRequest",
			D_METHOD("create", "path", "type_hint", "use_sub_threads", "cache_mode"),
			&ResourceRequest::create,
			DEFVAL(String()),
			DEFVAL(false),
			DEFVAL(ResourceLoader::CACHE_MODE_REUSE));

	ClassDB::bind_method(D_METHOD("is_done"), &ResourceRequest::is_done);
	ClassDB::bind_method(D_METHOD("is_success"), &ResourceRequest::is_success);
	ClassDB::bind_method(D_METHOD("get_error"), &ResourceRequest::get_error);
	ClassDB::bind_method(D_METHOD("get_resource_path"), &ResourceRequest::get_resource_path);
	ClassDB::bind_method(D_METHOD("get_resource"), &ResourceRequest::get_resource);
	ClassDB::bind_method(D_METHOD("cancel"), &ResourceRequest::cancel);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "done"), "", "is_done");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "success"), "", "is_success");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "error"), "", "get_error");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "resource_path"), "", "get_resource_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource"), "", "get_resource");

	ADD_SIGNAL(MethodInfo("progress_updated", PropertyInfo(Variant::FLOAT, "progress")));
	ADD_SIGNAL(MethodInfo("completed", PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "Resource")));
}

ResourceRequest *ResourceRequest::create(const String &p_path, const String &p_type_hint, bool p_use_sub_threads, ResourceLoader::CacheMode p_cache_mode) {
	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL_V(tree, nullptr);

	ResourceRequest *request = memnew(ResourceRequest);
	request->_path = p_path;
	request->_type_hint = p_type_hint;
	request->_use_sub_threads = p_use_sub_threads;
	request->_cache_mode = p_cache_mode;

	request->set_process_internal(true);
	tree->get_root()->add_child(request, false, Node::INTERNAL_MODE_DISABLED);
	request->call_deferred(SNAME("_start_request"));

	return request;
}

void ResourceRequest::_start_request() {
	Error err = ResourceLoader::load_threaded_request(_path, _type_hint, _use_sub_threads, _cache_mode);
	if (err != OK) {
		if (!_is_done) {
			_done(Ref<Resource>(), err);
		}
		return;
	}

	if (ResourceLoader::load_threaded_get_status(_path, &_progress) != ResourceLoader::THREAD_LOAD_LOADED) {
		return;
	}

	Error get_err = OK;
	Ref<Resource> res = ResourceLoader::load_threaded_get(_path, &get_err);
	if (get_err != OK) {
		if (!_is_done) {
			_done(Ref<Resource>(), get_err);
		}
		return;
	}

	if (!_is_done) {
		_done(res, OK);
	}
}

void ResourceRequest::_notification(int p_what) {
	if (p_what != NOTIFICATION_PROCESS) {
		return;
	}

	switch (ResourceLoader::load_threaded_get_status(_path, &_progress)) {
		case ResourceLoader::THREAD_LOAD_IN_PROGRESS: {
			emit_signal(SNAME("progress_updated"), _progress);
		} break;
		case ResourceLoader::THREAD_LOAD_LOADED: {
			emit_signal(SNAME("progress_updated"), 1.0f);

			Error err = OK;
			Ref<Resource> res = ResourceLoader::load_threaded_get(_path, &err);
			if (!_is_done) {
				_done(res, err);
			}
		} break;
		default: {
			ERR_PRINT("Failed to load resource: " + _path);
			if (!_is_done) {
				_done(Ref<Resource>(), FAILED);
			}
		} break;
	}
}

void ResourceRequest::_done(Ref<Resource> p_res, Error p_err) {
	_is_done = true;
	_error = p_err;
	if (p_res != _resource) {
		_resource = p_res;
	}

	set_process_internal(false);
	emit_signal(SNAME("completed"), p_res);
}

void ResourceRequest::cancel() {
	if (_is_done) {
		return;
	}

	call_deferred(SNAME("_done"), Variant(), ERR_SKIP);
}

bool ResourceRequest::is_done() const {
	return _is_done;
}

bool ResourceRequest::is_success() const {
	return _is_done && _error == OK;
}

Error ResourceRequest::get_error() const {
	return _error;
}

Ref<Resource> ResourceRequest::get_resource() const {
	return _resource;
}

String ResourceRequest::get_resource_path() const {
	return _path;
}
