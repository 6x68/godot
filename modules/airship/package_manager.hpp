/**************************************************************************/
/*  package_manager.h                                                     */
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

#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/variant/dictionary.h"

#include "scene/main/node.h"

// Per-package record of the engine state a package patched, kept so the
// patch can be rolled back when the package is unloaded.
class PackageInfo : public Object {
	GDCLASS(PackageInfo, Object)

public:
	struct LoadedGlobal {
		ObjectID node_id;
		bool singleton = false;
		bool singleton_registered = false;
		bool added_to_tree = false;
	};
	struct LoadedInputAction {
		bool had_action = false;
		uint8_t pad[3] = {};
		float deadzone = 0.0f;
		Vector<Ref<InputEvent>> events;
	};

	HashMap<String, LoadedGlobal> _loaded_globals;
	HashMap<String, LoadedInputAction> _loaded_input_actions;
	Vector<String> _loaded_global_order;
	String _pck_path;
	Dictionary _metadata;
	String _autoloads_filepath;
	int64_t _asset_version = 0;
	bool _stage_globals_until_game_scene = false;
	bool _replace_existing_autoloads = false;
	bool _preserve_global_script_slots = false;
	bool _loading = false;
	bool _loaded = false;
	bool _loaded_project_settings = false;
};

// Downloads, mounts and swaps downloadable content packages, and applies
// their engine-level patches (autoloads, input map, project settings).
class PackageManager : public Node {
	GDCLASS(PackageManager, Node)

public:
	PackageManager();

	static PackageManager *get_singleton() { return singleton; }

	// Takes ownership of p_scene by adding it to the tree itself, when the
	// package stages its globals until the game scene is live. Returns true
	// if the package handled the parenting.
	bool activate_staged_package_globals(const String &p_game_package_id, Node *p_scene);

	// Resource packs can only be removed once nothing references them, so
	// the unload is deferred to the end of the frame.
	void unload_package_deferred_resource_pack(const String &p_game_package_id);

	Variant user_context;
	Vector<String> pending_resource_pack_unloads;
	int64_t expected_base_builtin_version = -1;
	int64_t expected_base_menu_version = 0;
	bool game_builtin_loaded = false;
	bool swapping_builtin = false;

protected:
	static void _bind_methods();

private:
	HashMap<String, PackageInfo *> _loaded_packages;

	static inline PackageManager *singleton = nullptr;
};

#endif // PACKAGE_MANAGER_H
