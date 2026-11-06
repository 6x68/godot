/**************************************************************************/
/*  resource_request.h                                                    */
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

#ifndef RESOURCE_REQUEST_H
#define RESOURCE_REQUEST_H

#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"

#include "scene/main/node.h"

// Polls a threaded resource load and reports it through signals.
// A request is a Node so that it can be driven by the scene tree
// processing loop; it is parented to the tree root and freed by the owner
// once `completed` has been handled.
class ResourceRequest : public Node {
	GDCLASS(ResourceRequest, Node)

public:
	static ResourceRequest *create(const String &p_path, const String &p_type_hint = "", bool p_use_sub_threads = false, ResourceLoader::CacheMode p_cache_mode = ResourceLoader::CACHE_MODE_REUSE);

	bool is_done() const;
	bool is_success() const;
	Error get_error() const;
	Ref<Resource> get_resource() const;
	String get_resource_path() const;

	void cancel();

protected:
	static void _bind_methods();

private:
	void _start_request();
	void _done(Ref<Resource> p_res, Error p_err);
	void _notification(int p_what);

	Ref<Resource> _resource;
	String _path;
	String _type_hint;
	ResourceLoader::CacheMode _cache_mode = ResourceLoader::CACHE_MODE_REUSE;
	float _progress = 0.0f;
	bool _use_sub_threads = false;
	bool _is_done = false;
	Error _error = OK;
};

#endif // RESOURCE_REQUEST_H
