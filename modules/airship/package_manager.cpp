/**************************************************************************/
/*  package_manager.cpp                                                   */
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

#include "package_manager.hpp"

#include "core/object/class_db.h"

void PackageManager::_bind_methods() {
	// Package loading/swap methods are bound in a later pass.
}

PackageManager::PackageManager() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "Singleton for PackageManager already exists.");
	singleton = this;
}

bool PackageManager::activate_staged_package_globals(const String &p_game_package_id, Node *p_scene) {
	// Staged global activation needs the full package loader.
	return false;
}

void PackageManager::unload_package_deferred_resource_pack(const String &p_game_package_id) {
	// Deferred resource pack unloading needs the full package loader.
}
