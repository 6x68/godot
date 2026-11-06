/**************************************************************************/
/*  scene_manager.cpp                                                     */
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

#include "scene_manager.hpp"

#include "core/io/resource_request.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

#include "package_manager.hpp"
#include "scene/resources/packed_scene.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

void SceneManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_current_scene"), &SceneManager::get_current_scene);
	ClassDB::bind_method(D_METHOD("load_scene", "path", "async", "as_loading_screen"), &SceneManager::load_scene, DEFVAL(true), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("load_game_scene", "path", "game_info"), &SceneManager::load_game_scene, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("is_loading"), &SceneManager::is_loading);
	ClassDB::bind_method(D_METHOD("is_game_scene_loaded"), &SceneManager::is_game_scene_loaded);
	ClassDB::bind_method(D_METHOD("started_from_boot_scene"), &SceneManager::started_from_boot_scene);
	ClassDB::bind_method(D_METHOD("mark_boot"), &SceneManager::mark_boot);
	ClassDB::bind_method(D_METHOD("set_cancel_requested", "cancel_requested"), &SceneManager::set_cancel_requested);
	ClassDB::bind_method(D_METHOD("is_cancel_requested"), &SceneManager::is_cancel_requested);
	ClassDB::bind_method(D_METHOD("set_user_context", "user_context"), &SceneManager::set_user_context);
	ClassDB::bind_method(D_METHOD("get_user_context"), &SceneManager::get_user_context);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "current_scene", PROPERTY_HINT_RESOURCE_TYPE, "Node"), "", "get_current_scene");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "game_scene_loaded"), "", "is_game_scene_loaded");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cancel_requested"), "set_cancel_requested", "is_cancel_requested");
	ADD_PROPERTY(PropertyInfo(Variant::NIL, "user_context"), "set_user_context", "get_user_context");

	ADD_SIGNAL(MethodInfo("loading_game", PropertyInfo(Variant::DICTIONARY, "game_info")));
	ADD_SIGNAL(MethodInfo("loading_progress", PropertyInfo(Variant::FLOAT, "progress")));
	ADD_SIGNAL(MethodInfo("game_loaded", PropertyInfo(Variant::INT, "err", PROPERTY_HINT_ENUM, "Error")));
	ADD_SIGNAL(MethodInfo("loading_scene", PropertyInfo(Variant::STRING, "scene_path")));
	ADD_SIGNAL(MethodInfo("unloading_scene"));
	ADD_SIGNAL(MethodInfo("unloading_game"));
	ADD_SIGNAL(MethodInfo("scene_loaded", PropertyInfo(Variant::INT, "err", PROPERTY_HINT_ENUM, "Error"), PropertyInfo(Variant::STRING, "scene_path")));
	ADD_SIGNAL(MethodInfo("message", PropertyInfo(Variant::NIL, "topic"), PropertyInfo(Variant::NIL, "data")));
}

SceneManager::SceneManager() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "Singleton for SceneManager already exists.");
	singleton = this;
}

void SceneManager::_initialize_manager() {
	if (_init) {
		return;
	}

	SceneTree *tree = SceneTree::get_singleton();
	if (tree == nullptr) {
		return;
	}

	Node *current_scene = tree->get_current_scene();
	if (current_scene == nullptr) {
		return;
	}

	_init = true;
	_current_scene_id = current_scene->get_instance_id();
}

Node *SceneManager::get_current_scene() {
	_initialize_manager();
	return Object::cast_to<Node>(ObjectDB::get_instance(_current_scene_id));
}

void SceneManager::load_scene(const String &p_scene_path, bool p_async, bool p_as_loading_screen) {
	if (_loading) {
		return;
	}

	_loading = true;
	_cancel_requested = false;
	_initialize_manager();

	call_deferred(SNAME("_load_scene"), p_scene_path, ResourceLoader::CACHE_MODE_REUSE, p_async, (int64_t)p_as_loading_screen);
}

void SceneManager::load_game_scene(const String &p_scene_path, Dictionary p_game_info) {
	if (_loading) {
		return;
	}

	_loading = true;
	_cancel_requested = false;
	_initialize_manager();

	call_deferred(SNAME("_load_game_scene"), p_scene_path, p_game_info);
}

void SceneManager::_load_game_scene(const String &p_scene_path, Dictionary p_game_info) {
	emit_signal(SNAME("loading_game"), p_game_info);

	_loading_game_package_id = p_game_info.get("id", String());

	// Game scenes are always loaded asynchronously so the loading screen keeps running.
	_load_scene(p_scene_path, ResourceLoader::CACHE_MODE_REUSE, true, SCENE_TYPE_GAME);
}

void SceneManager::_load_scene(const String &p_scene_path, ResourceLoader::CacheMode p_cache_mode, bool p_async, int p_scene_type) {
	emit_signal(SNAME("loading_scene"), p_scene_path);

	if (p_scene_type == SCENE_TYPE_GAME || p_async) {
		ResourceRequest *request = ResourceRequest::create(p_scene_path, String(), false, p_cache_mode);
		ERR_FAIL_NULL(request);

		_current_load_req_id = request->get_instance_id();

		request->connect(SNAME("completed"),
				callable_mp(this, &SceneManager::_on_scene_loaded).bind(_current_load_req_id, p_scene_path, p_scene_type));
		request->connect(SNAME("progress_updated"), callable_mp(this, &SceneManager::_on_scene_load_progress));
		return;
	}

	_loading = false;

	Error err = OK;
	Ref<Resource> res = ResourceLoader::load(p_scene_path, String(), p_cache_mode, &err);

	if (res.is_null()) {
		emit_signal(SNAME("scene_loaded"), (int64_t)FAILED, p_scene_path);
		return;
	}

	_unload_current_scene(false);

	Ref<PackedScene> packed_scene = res;
	if (packed_scene.is_null()) {
		_instantiate_scene(Ref<PackedScene>(), String());
	} else {
		_instantiate_scene(packed_scene, String());
	}

	emit_signal(SNAME("scene_loaded"), (int64_t)OK, p_scene_path);
}

void SceneManager::_on_scene_loaded(Ref<Resource> p_res, ObjectID p_req_id, const String &p_scene_path, int p_scene_type) {
	ResourceRequest *request = Object::cast_to<ResourceRequest>(ObjectDB::get_instance(p_req_id));
	ERR_FAIL_NULL_MSG(request, "Parameter \"request\" is null.");

	Error err = request->get_error();
	request->queue_free();

	String game_package_id;
	bool instantiate = false;

	if (p_scene_type == SCENE_TYPE_GAME) {
		if (_cancel_requested) {
			_loading = false;

			emit_signal(SNAME("scene_loaded"), (int64_t)ERR_SKIP, p_scene_path);
			emit_signal(SNAME("game_loaded"), (int64_t)ERR_SKIP);
			return;
		}

		if (p_res.is_valid()) {
			// Keep the loading screen alive until the game scene is in the tree.
			_unload_current_scene(true);
			game_package_id = _loading_game_package_id;
			instantiate = true;
		}
	} else if (p_res.is_valid()) {
		_unload_current_scene(false);
		instantiate = true;
	}

	if (!instantiate) {
		_loading = false;

		emit_signal(SNAME("scene_loaded"), (int64_t)err, p_scene_path);
		if (p_scene_type == SCENE_TYPE_GAME) {
			emit_signal(SNAME("game_loaded"), (int64_t)err);
			_loading_game_package_id = String();
		}
		return;
	}

	Ref<PackedScene> packed_scene = p_res;
	ObjectID instantiated_id;
	if (packed_scene.is_null()) {
		instantiated_id = _instantiate_scene(Ref<PackedScene>(), game_package_id);
	} else {
		instantiated_id = _instantiate_scene(packed_scene, game_package_id);
	}

	if (p_scene_type == SCENE_TYPE_LOADING_SCREEN) {
		_loading_screen_id = instantiated_id;
		_loading = false;
	} else if (p_scene_type == SCENE_TYPE_GAME) {
		_unload_loading_screen();
		_current_scene_is_game = true;
		_current_game_package_id = _loading_game_package_id;
	} else {
		_loading = false;
	}

	emit_signal(SNAME("scene_loaded"), (int64_t)OK, p_scene_path);

	if (p_scene_type == SCENE_TYPE_GAME) {
		emit_signal(SNAME("game_loaded"), (int64_t)OK);
		_loading_game_package_id = String();
	}
}

void SceneManager::_on_scene_load_progress(float p_progress) {
	emit_signal(SNAME("loading_progress"), p_progress);
}

ObjectID SceneManager::_instantiate_scene(const Ref<PackedScene> &p_packed_scene, const String &p_game_package_id) {
	Node *node = nullptr;
	if (p_packed_scene.is_valid()) {
		node = p_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_DISABLED);
	}
	ERR_FAIL_NULL_V(node, ObjectID());

	PackageManager *package_manager = PackageManager::get_singleton();
	if (!p_game_package_id.is_empty() && package_manager != nullptr &&
			package_manager->activate_staged_package_globals(p_game_package_id, node)) {
		_current_scene_id = node->get_instance_id();
		return _current_scene_id;
	}

	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL_V(tree, ObjectID());
	tree->get_root()->add_child(node, false, Node::INTERNAL_MODE_DISABLED);
	tree->set_current_scene(node);

	_current_scene_id = node->get_instance_id();
	return _current_scene_id;
}

void SceneManager::_unload_current_scene(bool p_keep_loading_screen) {
	// The loading screen stays mounted while a game scene loads behind it.
	if (_loading_screen_id.is_valid() && _current_scene_id == _loading_screen_id && p_keep_loading_screen) {
		return;
	}

	String game_package_id = _current_game_package_id;
	bool current_scene_is_game = _current_scene_is_game;

	if (_current_scene_id.is_valid()) {
		Node *node = Object::cast_to<Node>(ObjectDB::get_instance(_current_scene_id));
		if (node != nullptr) {
			emit_signal(SNAME("unloading_scene"));
			if (current_scene_is_game) {
				emit_signal(SNAME("unloading_game"));
			}
			memdelete(node);
		}
	}

	_current_scene_id = ObjectID();
	_current_scene_is_game = false;
	_current_game_package_id = String();

	if (current_scene_is_game && !game_package_id.is_empty()) {
		PackageManager *package_manager = PackageManager::get_singleton();
		if (package_manager != nullptr) {
			package_manager->unload_package_deferred_resource_pack(game_package_id);
		}
	}
}

void SceneManager::_unload_loading_screen() {
	ObjectID loading_screen_id = _loading_screen_id;
	_loading_screen_id = ObjectID();

	if (loading_screen_id.is_valid()) {
		Object *object = ObjectDB::get_instance(loading_screen_id);
		if (object != nullptr) {
			memdelete(object);
		}
	}
}

void SceneManager::_request_cancel_load(ObjectID p_load_req_id) {
	ResourceRequest *request = Object::cast_to<ResourceRequest>(ObjectDB::get_instance(p_load_req_id));
	if (request != nullptr) {
		request->cancel();
	}
}

void SceneManager::mark_boot() {
	_initialize_manager();
	_from_boot_scene = true;
}

void SceneManager::set_cancel_requested(bool p_cancel_requested) {
	_cancel_requested = p_cancel_requested;
	if (p_cancel_requested && _loading) {
		_request_cancel_load(_current_load_req_id);
	}
}

bool SceneManager::is_cancel_requested() const {
	return _cancel_requested;
}

bool SceneManager::is_loading() const {
	return _loading;
}

bool SceneManager::is_game_scene_loaded() const {
	return _current_scene_is_game;
}

bool SceneManager::started_from_boot_scene() const {
	return _from_boot_scene;
}

void SceneManager::set_user_context(const Variant &p_user_context) {
	_user_context = p_user_context;
}

Variant SceneManager::get_user_context() const {
	return _user_context;
}
