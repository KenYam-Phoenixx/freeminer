/*
Minetest
Copyright (C) 2016 juhdanad, Daniel Juhasz <juhdanad@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "raycast.h"
#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include <quaternion.h>
#include "constants.h"
#include "util/numeric.h"

bool RaycastSort::operator() (const PointedThing &pt1,
	const PointedThing &pt2) const
{
	// "nothing" cannot be sorted
	assert(pt1.type != POINTEDTHING_NOTHING);
	assert(pt2.type != POINTEDTHING_NOTHING);
	f32 pt1_distSq = pt1.distanceSq;

	// Add some bonus when one of them is an object
	if (pt1.type != pt2.type) {
		if (pt1.type == POINTEDTHING_OBJECT)
			pt1_distSq -= BS * BS;
		else if (pt2.type == POINTEDTHING_OBJECT)
			pt1_distSq += BS * BS;
	}

	// returns false if pt1 is nearer than pt2
	if (pt1_distSq < pt2.distanceSq) {
		return false;
	}

	if (pt1_distSq == pt2.distanceSq) {
		// Sort them to allow only one order
		if (pt1.type == POINTEDTHING_OBJECT)
			return (pt2.type == POINTEDTHING_OBJECT
				&& pt1.object_id < pt2.object_id);

		return (pt2.type == POINTEDTHING_OBJECT
				|| pt1.node_undersurface < pt2.node_undersurface);
	}
	return true;
}


RaycastState::RaycastState(const core::line3d<opos_t> &shootline,
	bool objects_pointable, bool liquids_pointable) :
	m_shootline(shootline),
	m_iterator(shootline.start / BS, oposToV3f(shootline.getVector() / BS)),
	m_previous_node(m_iterator.m_current_node_pos),
	m_objects_pointable(objects_pointable),
	m_liquids_pointable(liquids_pointable)
{
}

bool boxLineCollision(const aabb3f &box, const v3opos_t &start,
	const v3opos_t &dir, v3opos_t *collision_point, v3f *collision_normal)
{
	if (box.isPointInside(oposToV3f(start))) {
		*collision_point = start;
		collision_normal->set(0, 0, 0);
		return true;
	}
	float m = 0;

	// Test X collision
	if (dir.X != 0) {
		if (dir.X > 0)
			m = (box.MinEdge.X - start.X) / dir.X;
		else
			m = (box.MaxEdge.X - start.X) / dir.X;

		if (m >= 0 && m <= 1) {
			*collision_point = start + dir * m;
			if ((collision_point->Y >= box.MinEdge.Y)
					&& (collision_point->Y <= box.MaxEdge.Y)
					&& (collision_point->Z >= box.MinEdge.Z)
					&& (collision_point->Z <= box.MaxEdge.Z)) {
				collision_normal->set((dir.X > 0) ? -1 : 1, 0, 0);
				return true;
			}
		}
	}

	// Test Y collision
	if (dir.Y != 0) {
		if (dir.Y > 0)
			m = (box.MinEdge.Y - start.Y) / dir.Y;
		else
			m = (box.MaxEdge.Y - start.Y) / dir.Y;

		if (m >= 0 && m <= 1) {
			*collision_point = start + dir * m;
			if ((collision_point->X >= box.MinEdge.X)
					&& (collision_point->X <= box.MaxEdge.X)
					&& (collision_point->Z >= box.MinEdge.Z)
					&& (collision_point->Z <= box.MaxEdge.Z)) {
				collision_normal->set(0, (dir.Y > 0) ? -1 : 1, 0);
				return true;
			}
		}
	}

	// Test Z collision
	if (dir.Z != 0) {
		if (dir.Z > 0)
			m = (box.MinEdge.Z - start.Z) / dir.Z;
		else
			m = (box.MaxEdge.Z - start.Z) / dir.Z;

		if (m >= 0 && m <= 1) {
			*collision_point = start + dir * m;
			if ((collision_point->X >= box.MinEdge.X)
					&& (collision_point->X <= box.MaxEdge.X)
					&& (collision_point->Y >= box.MinEdge.Y)
					&& (collision_point->Y <= box.MaxEdge.Y)) {
				collision_normal->set(0, 0, (dir.Z > 0) ? -1 : 1);
				return true;
			}
		}
	}
	return false;
}

// from inline vector3df quaternion::operator* (const vector3df& v) const
inline irr::core::vector3d<opos_t> quaternion_operator_star(
		const core::quaternion &self, const irr::core::vector3d<opos_t> &v)
{
	// nVidia SDK implementation

	irr::core::vector3d<opos_t> uv, uuv;
	const irr::core::vector3d<opos_t> qvec(self.X, self.Y, self.Z);
	uv = qvec.crossProduct(v);
	uuv = qvec.crossProduct(uv);
	uv *= (2.0f * self.W);
	uuv *= 2.0f;

	return v + uv + uuv;
}

bool boxLineCollision(const aabb3f &box, const v3f &rotation,
	const v3opos_t &start, const v3opos_t &dir,
	v3opos_t *collision_point, v3f *collision_normal, v3f *raw_collision_normal)
{
	// Inversely transform the ray rather than rotating the box faces;
	// this allows us to continue using a simple ray - AABB intersection
	core::quaternion rot(rotation * core::DEGTORAD);
	rot.makeInverse();

	bool collision = boxLineCollision(box, quaternion_operator_star(rot, start), quaternion_operator_star(rot, dir), collision_point, collision_normal);
	if (!collision) return collision;

	// Transform the results back
	rot.makeInverse();
	*collision_point = quaternion_operator_star(rot,  *collision_point);
	*raw_collision_normal = *collision_normal;
	*collision_normal = rot * *collision_normal;
	return collision;
}
