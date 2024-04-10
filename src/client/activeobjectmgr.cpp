/*
Minetest
Copyright (C) 2010-2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

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

#include <cmath>
#include <log.h>
#include <string>
#include "irr_v3d.h"
#include "irrlichttypes.h"
#include "profiler.h"
#include "util/numeric.h"
#include "activeobjectmgr.h"

namespace client
{

ActiveObjectMgr::~ActiveObjectMgr()
{
	if (!m_active_objects.empty()) {
		warningstream << "client::ActiveObjectMgr::~ActiveObjectMgr(): not cleared."
				<< std::endl;
		clear();
	}
}

void ActiveObjectMgr::step(
		float dtime, const std::function<void(const ClientActiveObjectPtr&)> &f)
{
	 TimeTaker timr(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " obj="+ std::to_string(m_active_objects.size()));

	g_profiler->avg("ActiveObjectMgr: CAO count [#]", m_active_objects.size());

	// Same as in server activeobjectmgr.
	std::vector<u16> ids = getAllIds();

	for (u16 id : ids) {
		auto it = m_active_objects.find(id);
		if (it == m_active_objects.end())
			continue; // obj was removed
		f(it->second);
	}

	m_active_objects_deleted.clear();
}

// clang-format off
bool ActiveObjectMgr::registerObject(std::shared_ptr<ClientActiveObject> obj)
{
	assert(obj); // Pre-condition
	if (obj->getId() == 0) {
		u16 new_id = getFreeId();
		if (new_id == 0) {
			infostream << "Client::ActiveObjectMgr::registerObject(): "
					<< "no free id available" << std::endl;

			return false;
		}
		obj->setId(new_id);
	}

	if (!isFreeId(obj->getId())) {
		infostream << "Client::ActiveObjectMgr::registerObject(): "
				<< "id is not free (" << obj->getId() << ")" << std::endl;
		return false;
	}
	infostream << "Client::ActiveObjectMgr::registerObject(): "
			<< "added (id=" << obj->getId() << ")" << std::endl;
	m_active_objects.insert_or_assign(obj->getId(), obj);
	return true;
}

void ActiveObjectMgr::removeObject(u16 id)
{
	verbosestream << "Client::ActiveObjectMgr::removeObject(): "
			<< "id=" << id << std::endl;
	auto obj = getActiveObject(id);
	if (!obj) {
		infostream << "Client::ActiveObjectMgr::removeObject(): "
				<< "id=" << id << " not found" << std::endl;
		return;
	}

	//std::unique_ptr<ClientActiveObject> obj = std::move(it->second);
	m_active_objects.erase(id);

	obj->removeFromScene(true);
	
	m_active_objects_deleted.emplace_back(std::move(obj));
}

// clang-format on
void ActiveObjectMgr::getActiveObjects(const v3opos_t &origin, f32 max_d,
		std::vector<DistanceSortedActiveObject> &dest)
{
	opos_t max_d2 = max_d * max_d;
	for (auto &ao_it : m_active_objects) {
		const auto obj = ao_it.second;

		opos_t d2 = (obj->getPosition() - origin).getLengthSQ();

		if (d2 > max_d2)
			continue;

		dest.emplace_back(obj, d2);
	}
}

std::vector<DistanceSortedActiveObject> ActiveObjectMgr::getActiveSelectableObjects(const core::line3d<opos_t> &shootline)
{
	std::vector<DistanceSortedActiveObject> dest;
	f32 max_d = shootline.getLength();
	v3opos_t dir = shootline.getVector().normalize();

	for (auto &ao_it : m_active_objects) {
		auto obj = ao_it.second;

		aabb3f selection_box;
		if (!obj->getSelectionBox(&selection_box))
			continue;

		v3opos_t obj_center = obj->getPosition() + v3fToOpos(selection_box.getCenter());
		f32 obj_radius_sq = selection_box.getExtent().getLengthSQ() / 4;

		v3opos_t c = obj_center - shootline.start;
		opos_t a = dir.dotProduct(c);           // project c onto dir
		f32 b_sq = c.getLengthSQ() - a * a;  // distance from shootline to obj_center, squared

		if (b_sq > obj_radius_sq)
			continue;

		// backward- and far-plane
		f32 obj_radius = std::sqrt(obj_radius_sq);
		if (a < -obj_radius || a > max_d + obj_radius)
			continue;

		dest.emplace_back(obj, a);
	}
	return dest;
}

} // namespace client
