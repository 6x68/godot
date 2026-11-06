/**************************************************************************/
/*  base_spring.cpp                                                       */
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

#include "./base_spring.hpp"

#include "core/math/math_funcs.h"
#include "core/object/class_db.h"

// The spring counts as settled once it is this close to the target and
// moving slower than this many units per second.
#define SETTLE_EPSILON 0.0001

void BaseSpring::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_angular_frequency", "angular_frequency"), &BaseSpring::set_angular_frequency);
	ClassDB::bind_method(D_METHOD("get_angular_frequency"), &BaseSpring::get_angular_frequency);
	ClassDB::bind_method(D_METHOD("set_damping_ratio", "damping_ratio"), &BaseSpring::set_damping_ratio);
	ClassDB::bind_method(D_METHOD("get_damping_ratio"), &BaseSpring::get_damping_ratio);
	ClassDB::bind_method(D_METHOD("is_at_target"), &BaseSpring::is_at_target);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angular_frequency", PROPERTY_HINT_RANGE, "0,200,0.01,or_greater"), "set_angular_frequency", "get_angular_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping_ratio", PROPERTY_HINT_RANGE, "0,4,0.001,or_greater"), "set_damping_ratio", "get_damping_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "at_target"), "", "is_at_target");

	BIND_ENUM_CONSTANT(UPDATE_IDLE);
	BIND_ENUM_CONSTANT(UPDATE_PHYSICS);
	BIND_ENUM_CONSTANT(UPDATE_PROCESS);
}

void BaseSpring::set_angular_frequency(double p_angular_frequency) {
	// A frequency of zero can never reach the target, and a negative one
	// would run the spring backwards, so both are rejected.
	angular_frequency = MAX(0.0, p_angular_frequency);
}

void BaseSpring::set_damping_ratio(double p_damping_ratio) {
	damping_ratio = MAX(0.0, p_damping_ratio);
}

// Semi-implicit (symplectic) Euler on the standard damped-spring system
//     x'' = -2*zeta*w*x' - w^2*(x - target)
//
// Integrating velocity first and then using the *new* velocity for the
// position update is what keeps the system stable across the very large
// timesteps a frame hitch produces; an explicit Euler step would gain energy
// and blow up.
//
// The three overloads below are the shared numerical core. They are separate
// rather than a template because the component-wise test for "settled" needs a
// per-type size, which a plain `double` does not have.

// Advances one scalar spring component in place. `T` is `double` for the
// 1D spring and `real_t` for the per-component axes of the 2D/3D springs.
template <typename T>
static double spring_step1d(double p_delta, double p_w, double p_zeta, T &r_value, T p_target, T &r_velocity) {
	const double disp = r_value - p_target;
	r_velocity += ((r_velocity * (-2.0 * p_zeta * p_w)) + (disp * (-(p_w * p_w)))) * p_delta;
	r_value += r_velocity * p_delta;
	return r_value;
}

double BaseSpring::step(double p_delta, double &r_value, double p_target, double &r_velocity) {
	if (p_delta <= 0.0) {
		return r_value;
	}
	if (angular_frequency <= 0.0) {
		// Degenerate spring: snap to the target and stop moving.
		r_value = p_target;
		r_velocity = 0.0;
		at_target = true;
		return r_value;
	}
	spring_step1d(p_delta, angular_frequency, damping_ratio, r_value, p_target, r_velocity);
	at_target = Math::abs(r_value - p_target) <= SETTLE_EPSILON && Math::abs(r_velocity) <= SETTLE_EPSILON;
	if (at_target) {
		// Snap rather than asymptote, otherwise the node would never report
		// itself settled in exact arithmetic.
		r_value = p_target;
		r_velocity = 0.0;
	}
	return r_value;
}

Vector2 BaseSpring::step(double p_delta, Vector2 &r_value, const Vector2 &p_target, Vector2 &r_velocity) {
	if (p_delta <= 0.0) {
		return r_value;
	}
	if (angular_frequency <= 0.0) {
		r_value = p_target;
		r_velocity = Vector2();
		at_target = true;
		return r_value;
	}
	// Each axis is an independent 1D spring, so they are integrated
	// separately rather than as a 2D vector.
	spring_step1d(p_delta, angular_frequency, damping_ratio, r_value.x, p_target.x, r_velocity.x);
	spring_step1d(p_delta, angular_frequency, damping_ratio, r_value.y, p_target.y, r_velocity.y);
	at_target = Math::abs(r_value.x - p_target.x) <= SETTLE_EPSILON && Math::abs(r_velocity.x) <= SETTLE_EPSILON && Math::abs(r_value.y - p_target.y) <= SETTLE_EPSILON && Math::abs(r_velocity.y) <= SETTLE_EPSILON;
	if (at_target) {
		r_value = p_target;
		r_velocity = Vector2();
	}
	return r_value;
}

Vector3 BaseSpring::step(double p_delta, Vector3 &r_value, const Vector3 &p_target, Vector3 &r_velocity) {
	if (p_delta <= 0.0) {
		return r_value;
	}
	if (angular_frequency <= 0.0) {
		r_value = p_target;
		r_velocity = Vector3();
		at_target = true;
		return r_value;
	}
	spring_step1d(p_delta, angular_frequency, damping_ratio, r_value.x, p_target.x, r_velocity.x);
	spring_step1d(p_delta, angular_frequency, damping_ratio, r_value.y, p_target.y, r_velocity.y);
	spring_step1d(p_delta, angular_frequency, damping_ratio, r_value.z, p_target.z, r_velocity.z);
	at_target = Math::abs(r_value.x - p_target.x) <= SETTLE_EPSILON && Math::abs(r_velocity.x) <= SETTLE_EPSILON && Math::abs(r_value.y - p_target.y) <= SETTLE_EPSILON && Math::abs(r_velocity.y) <= SETTLE_EPSILON && Math::abs(r_value.z - p_target.z) <= SETTLE_EPSILON && Math::abs(r_velocity.z) <= SETTLE_EPSILON;
	if (at_target) {
		r_value = p_target;
		r_velocity = Vector3();
	}
	return r_value;
}
