/**************************************************************************/
/*  scene_manager.h                                                       */
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

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "core/io/resource_loader.h"
#include "core/object/object_id.h"
#include "core/variant/dictionary.h"

#include "scene/main/node.h"

class PackedScene;

class SceneManager : public Node {
	GDCLASS(SceneManager, Node)

public:
	enum SceneType {
		SCENE_TYPE_NORMAL = 0,
		SCENE_TYPE_LOADING_SCREEN = 1,
		SCENE_TYPE_GAME = 2,
	};

	SceneManager();

	// Kicks off a scene load. `p_async` routes through ResourceRequest, the
	// synchronous path is only taken for non-game scenes.
	void load_scene(const String &p_scene_path, bool p_async = true, bool p_as_loading_screen = false);
	void load_game_scene(const String &p_scene_path, Dictionary p_game_info = Dictionary());

	Node *get_current_scene();
	bool is_loading() const;
	bool is_game_scene_loaded() const;
	bool started_from_boot_scene() const;

	// Records the scene that was already active as the boot scene, so that
	// started_from_boot_scene() stays valid after the first transition.
	void mark_boot();
	void set_cancel_requested(bool p_cancel_requested);
	bool is_cancel_requested() const;

	void set_user_context(const Variant &p_user_context);
	Variant get_user_context() const;

	static inline SceneManager *singleton = nullptr;

protected:
	static void _bind_methods();

private:
	void _initialize_manager();

	void _load_scene(const String &p_scene_path, ResourceLoader::CacheMode p_cache_mode, bool p_async, int p_scene_type);
	void _load_game_scene(const String &p_scene_path, Dictionary p_game_info);
	void _on_scene_loaded(Ref<Resource> p_res, ObjectID p_req_id, const String &p_scene_path, int p_scene_type);
	void _on_scene_load_progress(float p_progress);

	ObjectID _instantiate_scene(const Ref<PackedScene> &p_packed_scene, const String &p_game_package_id);
	void _unload_current_scene(bool p_keep_loading_screen);
	void _unload_loading_screen();

	static void _request_cancel_load(ObjectID p_load_req_id);

	ObjectID _current_scene_id;
	ObjectID _current_load_req_id;
	ObjectID _loading_screen_id;
	String _loading_game_package_id;
	String _current_game_package_id;
	Variant _user_context;
	bool _current_scene_is_game = false;
	bool _loading = false;
	bool _init = false;
	bool _from_boot_scene = false;
	bool _cancel_requested = false;
};

#endif // SCENE_MANAGER_H
