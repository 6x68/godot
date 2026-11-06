/**************************************************************************/
/*  base_spring.h                                                         */
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

#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "scene/main/node.h"

// Damped-spring easing, shared by Spring1D, Spring2D and Spring3D.
//
// The spring is parameterised by an angular frequency (how quickly it tries
// to reach the target) and a damping ratio:
//
//   * below 1 -> underdamped: overshoots and oscillates before settling,
//   * exactly 1 -> critically damped: reaches the target as fast as possible
//     without overshooting,
//   * above 1 -> overdamped: approaches the target lazily from below.
//
// The concrete subclasses differ only in the value type they integrate, so
// everything except `calculate()`, `reset()`, `set_value()` and the
// target/velocity accessors lives here.
class BaseSpring : public Node {
	GDCLASS(BaseSpring, Node)

public:
	// Which callback advances the spring.
	enum UpdateType {
		UPDATE_IDLE, // Do not step; the value stays where it was last set.
		UPDATE_PHYSICS, // Step from `_physics_process()`.
		UPDATE_PROCESS, // Step from `_process()`.
	};

protected:
	double angular_frequency = 20.0;
	double damping_ratio = 1.0;
	UpdateType update_type = UPDATE_IDLE;
	bool at_target = true;

	// Advances one damped-spring step in place and returns the new value.
	//
	// `r_value`, `p_target` and `r_velocity` are the subclass' storage, so
	// the numerical core is shared instead of being written three times.
	double step(double p_delta, double &r_value, double p_target, double &r_velocity);
	Vector2 step(double p_delta, Vector2 &r_value, const Vector2 &p_target, Vector2 &r_velocity);
	Vector3 step(double p_delta, Vector3 &r_value, const Vector3 &p_target, Vector3 &r_velocity);

	static void _bind_methods();

public:
	// Steps the spring by `p_delta` seconds and writes the new value back.
	virtual void calculate(double p_delta) = 0;

	// Teleports the spring to `p_value` with no velocity.
	virtual void reset(double p_value) = 0;

	// Teleports the spring to `p_value`, keeping the current velocity.
	virtual void set_value(double p_value) = 0;

	// True once the spring has settled on its target, letting callers skip
	// redundant work.
	bool is_at_target() const { return at_target; }

	void set_angular_frequency(double p_angular_frequency);
	double get_angular_frequency() const { return angular_frequency; }

	void set_damping_ratio(double p_damping_ratio);
	double get_damping_ratio() const { return damping_ratio; }

	void set_update_type(UpdateType p_update_type) { update_type = p_update_type; }
	UpdateType get_update_type() const { return update_type; }

	~BaseSpring() override {}
};

VARIANT_ENUM_CAST(BaseSpring::UpdateType);
