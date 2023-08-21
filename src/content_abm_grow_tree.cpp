/*
Copyright (C) 2023 proller <proler@gmail.com>
*/

/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cmath>
#include <cstdint>
#include "mapnode.h"
#include "nodedef.h"
#include "server.h"
#include "serverenvironment.h"

constexpr auto grow_debug = false;
//constexpr auto grow_debug_no_die = false;

// Trees use param2 for rotation, level1 is free
inline uint8_t get_tree_water_level(const MapNode &n)
{
	return n.getParam1();
}
inline void set_tree_water_level(MapNode &n, const uint8_t level)
{
	n.setParam1(level);
}

// Leaves use param1 for light, level2 is free
inline uint8_t get_leaves_water_level(const MapNode &n)
{
	return n.getParam2();
}

inline void set_leaves_water_level(MapNode &n, const uint8_t level)
{
	n.setParam2(level);
}

inline auto getLight(const auto &ndef, const auto &n)
{
	const auto lightingFlags = ndef->getLightingFlags(n);
	return std::max(n.getLight(LIGHTBANK_DAY, lightingFlags),
			n.getLight(LIGHTBANK_NIGHT, lightingFlags));
}

const v3pos_t leaves_grow_dirs[] = {
		// +right, +top, +back
		v3pos_t{0, 1, 0},  // 1 top
		v3pos_t{0, 0, 1},  // 2 back
		v3pos_t{0, 0, -1}, // 3 front
		v3pos_t{1, 0, 0},  // 4 right
		v3pos_t{-1, 0, 0}, // 5 left
		v3pos_t{0, -1, 0}, // 6 bottom
};

const v3pos_t leaves_look_dirs[] = {
		// +right, +top, +back
		v3pos_t{0, 0, 0},  // 0 self
		v3pos_t{0, 1, 0},  // 1 top
		v3pos_t{0, 0, 1},  // 2 back
		v3pos_t{0, 0, -1}, // 3 front
		v3pos_t{1, 0, 0},  // 4 right
		v3pos_t{-1, 0, 0}, // 5 left
		v3pos_t{0, -1, 0}, // 6 bottom
};

constexpr auto D_BOTTOM = 6;
constexpr auto D_TOP = 1;
constexpr auto D_SELF = 0;
constexpr auto D_BACK = 2;
constexpr auto D_FRONT = 3;
constexpr auto D_RIGHT = 4;
constexpr auto D_LEFT = 5;

struct GrowParams
{
	int tree_water_max = 50; // todo: depend on humidity 10-100
	int tree_grow_water_min = 20;
	int tree_grow_heat_min = 7;
	int tree_grow_heat_max = 40;
	int tree_grow_light_max = 12;		   // grow more leaves around before grow tree up
	int tree_get_water_from_humidity = 70; // rain start
	int tree_get_water_max_from_humidity = 30; // max level to get from air
	int tree_grow_chance = 10;
	int leaves_water_max = 20; // todo: depend on humidity 2-20
	int leaves_grow_light_min = 8;
	int leaves_grow_water_min_top = 3;
	int leaves_grow_water_min_bottom = 4;
	int leaves_grow_water_min_side = 2;
	int leaves_grow_heat_max = 40;
	int leaves_grow_heat_min = 3;
	int leaves_grow_prefer_top = 0;
	int leaves_die_light_max = 7; // 8
	int leaves_die_heat_max = -1;
	int leaves_die_heat_min = 55;
	int leaves_die_chance = 5;
	int leaves_die_from_liquid = 1;
	int leaves_to_fruit_water_min = 9;
	int leaves_to_fruit_heat_min = 15;
	int leaves_to_fruit_light_min = 10;
	int leaves_to_fruit_chance = 10;

	GrowParams(const ContentFeatures &cf, bool grow_debug_fast = false)
	{
		if (cf.groups.contains("tree_water_max"))
			tree_water_max = cf.groups.at("tree_water_max");
		if (cf.groups.contains("tree_grow_water_min"))
			tree_grow_water_min = cf.groups.at("tree_grow_water_min");
		if (cf.groups.contains("tree_grow_heat_min"))
			tree_grow_heat_min = cf.groups.at("tree_grow_heat_min");
		if (cf.groups.contains("tree_grow_heat_max"))
			tree_grow_heat_max = cf.groups.at("tree_grow_heat_max");
		if (cf.groups.contains("tree_grow_light_max"))
			tree_grow_light_max = cf.groups.at("tree_grow_light_max");
		if (cf.groups.contains("tree_grow_chance"))
			tree_grow_chance = grow_debug_fast ? 0 : cf.groups.at("tree_grow_chance");
		if (cf.groups.contains("tree_get_water_from_humidity"))
			tree_get_water_from_humidity = cf.groups.at("tree_get_water_from_humidity");
		if (cf.groups.contains("tree_get_water_max_from_humidity"))
			tree_get_water_max_from_humidity =
					cf.groups.at("tree_get_water_max_from_humidity");
		if (cf.groups.contains("leaves_water_max"))
			leaves_water_max = cf.groups.at("leaves_water_max");
		if (cf.groups.contains("leaves_grow_light_min"))
			leaves_grow_light_min = cf.groups.at("leaves_grow_light_min");
		if (cf.groups.contains("leaves_grow_water_min_top"))
			leaves_grow_water_min_top = cf.groups.at("leaves_grow_water_min_top");
		if (cf.groups.contains("leaves_grow_water_min_bottom"))
			leaves_grow_water_min_bottom = cf.groups.at("leaves_grow_water_min_bottom");
		if (cf.groups.contains("leaves_grow_water_min_side"))
			leaves_grow_water_min_side = cf.groups.at("leaves_grow_water_min_side");
		if (cf.groups.contains("leaves_grow_heat_max"))
			leaves_grow_heat_max = cf.groups.at("leaves_grow_heat_max");
		if (cf.groups.contains("leaves_grow_prefer_top"))
			leaves_grow_prefer_top = cf.groups.at("leaves_grow_prefer_top");
		if (cf.groups.contains("leaves_grow_heat_min"))
			leaves_grow_heat_min = cf.groups.at("leaves_grow_heat_min");
		if (cf.groups.contains("leaves_die_light_max"))
			leaves_die_light_max = cf.groups.at("leaves_die_light_max");
		if (cf.groups.contains("leaves_die_heat_max"))
			leaves_die_heat_max = cf.groups.at("leaves_die_heat_max");
		if (cf.groups.contains("leaves_die_heat_min"))
			leaves_die_heat_min = cf.groups.at("leaves_die_heat_min");
		if (cf.groups.contains("leaves_die_chance"))
			leaves_die_chance = grow_debug_fast ? 0 : cf.groups.at("leaves_die_chance");
		if (cf.groups.contains("leaves_die_from_liquid"))
			leaves_die_from_liquid = cf.groups.at("leaves_die_from_liquid");
		if (cf.groups.contains("leaves_to_fruit_water_min"))
			leaves_to_fruit_water_min = cf.groups.at("leaves_to_fruit_water_min");
		if (cf.groups.contains("leaves_to_fruit_heat_min"))
			leaves_to_fruit_heat_min = cf.groups.at("leaves_to_fruit_heat_min");
		if (cf.groups.contains("leaves_to_fruit_light_min"))
			leaves_to_fruit_light_min = cf.groups.at("leaves_to_fruit_light_min");
		if (cf.groups.contains("leaves_to_fruit_chance"))
			leaves_to_fruit_chance = cf.groups.at("leaves_to_fruit_chance");
	}
};

class GrowTree : public ActiveBlockModifier
{
	std::unordered_map<content_t, content_t> tree_to_leaves, tree_to_fruit;
	std::unordered_map<content_t, GrowParams> type_params;

	bool grow_debug_fast = false;
	//  bool grow_debug = false;
public:
	GrowTree(ServerEnvironment *env, NodeDefManager *ndef)
	{
		// g_settings->getBoolNoEx("grow_debug", grow_debug);
		g_settings->getBoolNoEx("grow_debug_fast", grow_debug_fast);

		std::vector<content_t> ids;
		ndef->getIds("group:grow_tree", ids);
		for (const auto &id_tree : ids) {
			const auto &cf_tree = ndef->get(id_tree);
			type_params.emplace(id_tree, GrowParams(cf_tree, grow_debug_fast));
			if (!cf_tree.liquid_alternative_source.empty()) {
				const auto id_leaves = ndef->getId(cf_tree.liquid_alternative_source);
				tree_to_leaves[id_tree] = id_leaves;

				const auto &cf_leaves = ndef->get(id_leaves);
				type_params.emplace(id_leaves, GrowParams(cf_leaves));
				if (!cf_leaves.liquid_alternative_source.empty())
					tree_to_fruit[id_tree] =
							ndef->getId(cf_leaves.liquid_alternative_source);
			}
		}
	}
	virtual const std::vector<std::string> getTriggerContents() const override
	{
		return {"group:grow_tree"};
	}
	virtual const std::vector<std::string> getRequiredNeighbors(
			bool activate) const override
	{
		return {};
	}
	// u32 getNeighborsRange() override { return 3; }
	virtual float getTriggerInterval() override { return grow_debug_fast ? 0.1 : 5; }
	virtual u32 getTriggerChance() override { return grow_debug_fast ? 1 : 5; }
	bool getSimpleCatchUp() override { return true; }
	virtual pos_t getMinY() override { return -MAX_MAP_GENERATION_LIMIT; };
	virtual pos_t getMaxY() override { return MAX_MAP_GENERATION_LIMIT; };
	virtual void trigger(ServerEnvironment *env, v3pos_t pos, MapNode n,
			u32 active_object_count, u32 active_object_count_wider, v3pos_t,
			bool activate) override
	{
		ServerMap *map = &env->getServerMap();
		const auto *ndef = env->getGameDef()->ndef();
		float heat = map->updateBlockHeat(env, pos);

		// dont grow to sides if can grow up
		bool top_is_not_tree{false};
		bool around_all_is_tree{true};
		int8_t near_tree{0};
		int8_t near_soil{0};
		int8_t near_liquid{0};
		content_t leaves_content{CONTENT_IGNORE};
		content_t fruit_content{CONTENT_IGNORE};

		struct Neighbor
		{
			MapNode node{};
			content_t content{CONTENT_IGNORE};
			bool is_liquid{false};
			bool is_my_leaves{false};
			bool is_any_leaves{false};
			bool is_fruit{false};
			bool is_tree{false};
			bool is_soil{false};
			bool side{false};
			bool top{false};
			bool bottom{false};
			bool self{false};
			uint8_t light{0};
			bool allow_grow_by_rotation{false};
			uint8_t facedir{0};
			ContentFeatures *cf{nullptr};
			int16_t water_level{0};
			v3pos_t pos{};
		};

		Neighbor nbh[7]{};
		{
			size_t i = 0;
			for (const auto &dir : leaves_look_dirs) {
				auto &nb = nbh[i];
				const bool is_self = i == D_SELF;
				nb.pos = pos + dir;

				nb.node = is_self ? n : map->getNodeTry(nb.pos);
				if (!nb.node) {
					return;
				}

				nb.content = nb.node.getContent();
				nb.cf = (ContentFeatures *)&ndef->get(nb.content);
				nb.light = getLight(ndef, nb.node);
				nb.top = i == D_TOP;
				nb.bottom = i == D_BOTTOM;
				nb.side = !nb.bottom && !nb.top;
				nb.self = i == D_SELF;
				nb.is_tree = is_self || nbh[D_SELF].content == nb.content ||
							 nb.cf->groups.contains("tree");

				if (is_self) {
					leaves_content = tree_to_leaves.contains(nb.content)
											 ? tree_to_leaves.at(nb.content)
											 : CONTENT_IGNORE;
					fruit_content = tree_to_fruit.contains(nb.content)
											? tree_to_fruit.at(nb.content)
											: CONTENT_IGNORE;
				}
				//DUMP(is_self, leaves_content);

				if (!is_self) {
					if (!nb.top && !nb.bottom && around_all_is_tree && !nb.is_tree)
						around_all_is_tree = false;

					nb.is_my_leaves = nb.content == leaves_content;
					nb.is_any_leaves =
							nb.is_my_leaves || nb.cf->groups.contains("leaves");
					nb.is_fruit = nb.content == fruit_content;
					// DUMP(is_self, nb.is_leaves, "=", nb.content, "==", (int)leaves_content);
					nb.is_liquid = nb.cf->groups.contains("liquid");
					near_liquid += nb.is_liquid;
					///has_liquids.emplace_back(nb.pos);

					if (nb.top && !nb.is_tree)
						top_is_not_tree = true;

					nb.is_soil = nb.cf->groups.contains("soil");
					near_soil += nb.is_soil;

					if (!nb.top && !nb.bottom && nb.is_tree) {
						++near_tree;
					}
				}

				nb.water_level = nb.is_my_leaves ? get_leaves_water_level(nb.node)
								 : nb.is_tree	 ? get_tree_water_level(nb.node)
												 : 0;

				nb.facedir = nb.node.getFaceDir(ndef);

				const auto &self_facedir = nbh[D_SELF].facedir;

				if (nb.self) {
					// Can self grow to up/down?
					nb.allow_grow_by_rotation =
							((self_facedir >= 0) && (self_facedir <= 3)) ||
							((self_facedir >= 20) && (self_facedir <= 23));
				} else if (nb.top || nb.bottom) {
					nb.allow_grow_by_rotation = nbh[D_SELF].allow_grow_by_rotation;
				} else if (i == D_FRONT || i == D_BACK) {
					// Can self grow this sides?
					nb.allow_grow_by_rotation = self_facedir == 7 || self_facedir == 9;
				} else if (i == D_LEFT || i == D_RIGHT) {
					nb.allow_grow_by_rotation = self_facedir == 18 || self_facedir == 12;
				}

				// if (grow_debug) DUMP(i, nb.allow_grow_by_rotation, nb.facedir, self_facedir);
				++i;
			}
		}

		const auto &params = type_params.at(nbh[D_SELF].content);
		const auto &content = nbh[D_SELF].content;

		const auto &self_allow_grow_by_rotation = nbh[D_SELF].allow_grow_by_rotation;
		int16_t &self_water_level = nbh[D_SELF].water_level;

		const auto self_water_level_orig = self_water_level;

		const auto save = [&]() {
			if (self_water_level_orig != self_water_level) {
				set_tree_water_level(n, self_water_level);
				map->setNode(pos, n);
			}
		};
		const auto decrease = [&](auto &level, int amount = 1) -> auto {
			if (level <= amount)
				return false;
			level -= amount;
			return true;
		};

		if (params.tree_get_water_from_humidity &&
				self_water_level < params.tree_get_water_max_from_humidity && near_soil &&
				self_allow_grow_by_rotation && !near_liquid) {
			float humidity = map->updateBlockHumidity(env, pos);
			if (humidity >= params.tree_get_water_from_humidity) {
				if (grow_debug_fast) {
					self_water_level = params.tree_get_water_max_from_humidity;
				} else {
					// TODO: depend on   += ceil( max_from_air * (params.tree_get_water_from_humidity)/(100-humidity))
					++self_water_level;
				}
				// if (grow_debug)DUMP("absorbair", self_water_level,self_water_level_orig, params.tree_get_water_max_from_humidity, humidity,grow_debug_fast);
			}
		}

		for (int i = D_SELF + 1; i <= D_BOTTOM; ++i) {
			auto &nb = nbh[i];
			const bool allow_grow_by_light =
					!nb.top || nb.light <= params.tree_grow_light_max;
			bool up_all_leaves = true;
			//DUMP("gr", i, nb.top, nb.bottom, allow_grow_by_light, nb.water_level, nb.is_leaves, nb.is_tree, nb.is_liquid, nb.is_soil);

			// Absorb water from near water blocks, leave one level
			// DUMP("absorb?", nb.pos.Y, self_water_level, params.tree_water_max, (int)near_soil, (int)near_liquid, allow_grow_up_by_rotation, nb.is_liquid);
			if (self_water_level < params.tree_water_max && near_soil &&
					self_allow_grow_by_rotation && nb.is_liquid) {
				// TODO: cached and random
				auto level = nb.node.getLevel(ndef);

				// TODO: allow get all water if bottom of water != water
				if (level <= 1)
					return;
				//auto amount = grow_debug_fast ? level - 1 : 1;
				auto amount = level - 1;
				if (self_water_level + amount > params.tree_water_max)
					amount = params.tree_water_max - self_water_level;
				level -= amount;

				nb.node.setLevel(ndef, level);

				if (!grow_debug_fast)
					map->setNode(nb.pos, nb.node);
				self_water_level += amount;
				//set_tree_water_level(n, self_water_level);
				//map->setNode(p, n);
				//if (grow_debug) DUMP("absorbwater", self_water_level, level, amount);
			}

			// Light recalc sometimes too rare
			if (nb.top && !allow_grow_by_light && leaves_content != CONTENT_IGNORE) {
				// DUMP(i, p.Y, top, allow_grow_by_light, up_all_leaves);
				for (pos_t li = 1; li <= LIGHT_SUN - params.tree_grow_light_max; ++li) {
					const auto p_up = pos + v3pos_t{0, li, 0};
					const auto n_up = li == 1 ? nbh[D_TOP].node : map->getNodeTry(p_up);
					// DUMP(top, allow_grow_by_light, up_all_leaves, n_up.getContent());
					if (!n_up || n_up.getContent() == CONTENT_AIR) {
						up_all_leaves = false;
						break;
					}
				}
			}

			//  DUMP(i, pos.Y, self_water_level, nb.top, nb.water_level, allow_grow_by_light, up_all_leaves, nb.is_my_leaves, nb.is_any_leaves, up_all_leaves, nb.light, params.leaves_die_light_max, nb.allow_grow_by_rotation, nb.is_liquid, nb.cf->name);
			auto tree_grow = [&]() {
				if (content == nb.content)
					return false;

				if (!((!params.tree_grow_heat_min || heat > params.tree_grow_heat_min) &&
							(!params.tree_grow_heat_max ||
									heat < params.tree_grow_heat_max)))
					return false;

				if (self_water_level < params.tree_grow_water_min)
					return false;

				if (!nb.allow_grow_by_rotation)
					return false;

				if (!(nb.is_any_leaves || nb.is_fruit || nb.cf->buildable_to ||
							nb.is_liquid || nb.is_soil || nb.cf->groups.contains("sand")))
					return false;

				if (nb.top && nb.content == CONTENT_AIR)
					return false;

				if (nb.top && nb.is_any_leaves) {
					if (nb.light < params.leaves_grow_light_min)
						return false;

					if (!(allow_grow_by_light || up_all_leaves))
						return false;
				}

				// dont grow too deep in liquid
				if (nb.bottom) {
					if (nb.is_liquid && nb.light <= 0)
						return false;

					if (near_tree >= 1)
						return false;
				}

				if (!(grow_debug_fast || activate ||
							!myrand_range(
									0, params.tree_grow_chance * (nb.bottom ? 3 : 1))))
					return false;

				if (!decrease(self_water_level))
					return true;

				//if (grow_debug) DUMP("tr->tr", i, nb.pos.Y, nb.top, nb.bottom, nb.content, content, self_water_level, self_water_level_orig, nb.light);

				map->setNode(nb.pos, {content, 1, nbh[D_SELF].node.getParam2()});
				return true;
			};

			if (tree_grow())
				break;

			if (((!nb.top && !nb.bottom && nb.content == content &&
						 !around_all_is_tree) ||
						nb.is_my_leaves)) {
				auto water_level = nb.content == leaves_content
										   ? get_leaves_water_level(nb.node)
										   : get_tree_water_level(nb.node);
				//DUMP(water_level, nb.is_leaves, allow_grow_up_by_rotation, nb.top, top_is_not_tree);
				if (!nb.is_my_leaves ||
						(nb.is_my_leaves && (nb.top || !self_allow_grow_by_rotation ||
													(!nb.top && top_is_not_tree))))

					if (water_level < (nb.is_my_leaves ? params.leaves_water_max
													   : params.tree_water_max) &&
							self_water_level > water_level
							/* !!!
												n_water_level > wl_dir
						   + (top ? -1 :bottom ? 1 : 0)
						*/
					) {
						if (nb.side && nb.is_tree && self_allow_grow_by_rotation) {
							// DUMP("skip tr side pump", water_level, nb.is_tree, allow_grow_up_by_rotation);
							continue;
						}
						// DUMP(nb.node, self_water_level, water_level);
						//??if (is_tree && dir_allow_grow_up_by_rotation && n_water_level >= params.tree_water_max) continue;

						if (!decrease(self_water_level)) {
							// if (grow_debug) DUMP("pumpfail", n_water_level, n_water_level_orig, wl_dir, top, bottom, c_dir, c);
							break;
						}
						//if (grow_debug)DUMP("tr pump", pos.Y, self_water_level,self_water_level_orig, water_level, nb.top,nb.bottom, nb.content, content);
						++water_level;
						nb.is_my_leaves ? set_leaves_water_level(nb.node, water_level)
										: set_tree_water_level(nb.node, water_level);

						map->setNode(nb.pos, nb.node);
					}
			}

			// Dont grow after top
			//if ((nb.top && nb.is_my_leaves) || nb.content == content) {
			//allow_grow_tree = false;
			//}

			//DUMP(allow_grow_leaves, leaves_c, heat , params.leaves_grow_heat_min, params.leaves_grow_heat_max, n_water_level, light_dir);
			if (
					//allow_grow_leaves
					(nb.allow_grow_by_rotation && nbh[D_TOP].content != content) &&
					leaves_content != CONTENT_IGNORE &&
					heat >= params.leaves_grow_heat_min &&
					heat <= params.leaves_grow_heat_max &&
					(self_water_level >= (nb.top ? params.leaves_grow_water_min_top
												 : params.leaves_grow_water_min_side)) &&
					// can_grow_leaves(n_water_level, top, bottom) &&
					nb.light >= params.leaves_grow_light_min) {
				if (nb.cf->buildable_to && !nb.is_liquid) {
					if (!decrease(self_water_level))
						break;
					//if (grow_debug)DUMP("tr->lv", nb.pos, self_water_level, self_water_level_orig,nb.light_dir);

					map->setNode(nb.pos, {leaves_content, nb.node.param1, 1});

					if (const auto block = map->getBlock(getNodeBlockPos(nb.pos));
							block) {
						block->setLightingExpired(true);
					}
				}
			}
		}

		// up-down distribute of rest

		if (self_allow_grow_by_rotation) {
			int16_t total_level = self_water_level;
			int8_t have_liquid = 1;
			auto &n_bottom = nbh[D_BOTTOM].node;
			if (nbh[D_BOTTOM].content == content) {
				total_level += nbh[D_BOTTOM].water_level;
				++have_liquid;
				//if (grow_debug)DUMP("get bot", nbh[D_BOTTOM].water_level, total_level,(int)have_liquid);
			}

			auto &n_top = nbh[D_TOP].node;
			if (nbh[D_TOP].content == content) {
				total_level += nbh[D_TOP].water_level; // wl_top;
				++have_liquid;
				//if (grow_debug)DUMP("get top", nbh[D_TOP].water_level, total_level,(int)have_liquid);
			}

			/*
tot
   avg/3
         b s t
3  1   = 2 1 0
4  1.3 = 2 1 1
5  1.6 = 2 2 1
6  2   = 3 2 1
7  2.3 = 3 2 2
8  2.6 = 3 3 2
9  3   = 4 3 2
10 3.3 = 4 3 3

bottom = floor(avg + 1)
self   = round(avg) 
top    = ceil(avg - 1)

*/
			const auto fill_bottom = [&](bool prefer = false) {
				if (nbh[D_BOTTOM].content == content) {

					/*
					const auto float_avg_level = (float)total_level / have_liquid;
					//if (grow_debug)DUMP(avg_level_for_bottom, (int)have_liquid, total_level);
					const auto avg_level = prefer ? ceil(float_avg_level + 0.1)
												  : floor(float_avg_level); // -1
					const auto want_level =
							avg_level < params.tree_water_max
									? avg_level + (avg_level >= (total_level ? 0 : 1))
									: params.tree_water_max;
*/
					const auto float_avg_level = (float)total_level / have_liquid;
					const auto avg_level = prefer ? floor(float_avg_level + 1)
												  : ceil(float_avg_level - 1);
					const auto want_level =
							std::min<uint8_t>(avg_level, params.tree_water_max);

					total_level -= want_level;
					--have_liquid;
					if (nbh[D_BOTTOM].water_level != want_level) {
						//if (grow_debug)DUMP("setbot", bottom_level, total_level,avg_level_for_bottom);
						set_tree_water_level(n_bottom, want_level);
						map->setNode(nbh[D_BOTTOM].pos, n_bottom);
					}
				}
			};

			const auto fill_top = [&](bool prefer = false) {
				if (nbh[D_TOP].content == content) {
					/*
					const auto float_avg_level = (float)total_level / have_liquid;
					//const int16_t avg_level_for_top =
					const auto avg_level = prefer ? ceil(float_avg_level + 0.1)
												  : floor(float_avg_level); // -1
					const auto want_level =
							avg_level < params.tree_water_max
									? avg_level + (avg_level >= (total_level ? 0 : 1))
									: params.tree_water_max;
*/
					const auto float_avg_level = (float)total_level / have_liquid;
					const auto avg_level = prefer ? floor(float_avg_level + 1)
												  : ceil(float_avg_level - 1);
					const auto want_level =
							std::min<uint8_t>(avg_level, params.tree_water_max);

					total_level -= want_level;
					--have_liquid;
					if (nbh[D_TOP].water_level != want_level) {
						//if (grow_debug) DUMP("settop", top_level, total_level, avg_level_for_top,around_all_is_tree);
						// if (all_is_tree && n_water_level>= params.tree_water_max) DUMP(top_level, total_level, float_avg_level_for_top, avg_level_for_top);
						set_tree_water_level(n_top, want_level);
						map->setNode(nbh[D_TOP].pos, n_top);
					}
				}
			};

			/*       
		                           1	
                               1   2
    			1  1   1   1   2   3
			  1 2  2   2   2   3   4
            1 2 3  3   3   3   4   5
          1 2 3 4  4   4   4   5   6
        1 2 3 4 5  5   5   5   6   71
      1 2 3 4 5 6  6   6   6   71  82
    1 2 3 4 5 6 7  7   7   71 182 193
  1 2 3 4 5 6 7 8  8   81 182 293 284
1 2 3 4 5 6 7 8 9  91 192 293 384 395
S S S S S S S S S SSS SSS SSS SSS SSS

*/
			// Yggdrasil mode
			if (near_tree >= 4
					//&&((nbh[D_BOTTOM].water_level >= params.tree_get_water_from_humidity / 2) || // params.tree_water_max
					//(nbh[D_SELF].water_level >= params.tree_get_water_from_humidity / 2))
			) {
				//DUMP("prefer top", pos.Y, around_all_is_tree, total_level, nbh[D_BOTTOM].water_level, nbh[D_SELF].water_level,nbh[D_TOP].water_level);
				fill_top(true);
				fill_bottom();
			} else {
				//DUMP("prefer bot", pos.Y, around_all_is_tree, total_level);
				fill_bottom(true);
				fill_top();
			}
			// if (grow_debug) DUMP("total res self:", total_level, (int)have_liquid, (int)near_tree, nbh[D_BOTTOM].water_level, nbh[D_SELF].water_level, nbh[D_TOP].water_level);
			self_water_level = total_level;
		}

		save();
	}
};

class GrowLeaves : public ActiveBlockModifier
{
	std::unordered_map<content_t, content_t> leaves_to_fruit;
	std::unordered_map<content_t, GrowParams> type_params;
	bool grow_debug_fast = false;

	static bool can_grow_leaves(
			GrowParams params, int8_t level, bool is_top, bool is_bottom)
	{
		if (is_top)
			return level >= params.leaves_grow_water_min_top;
		if (is_bottom)
			return level >= params.leaves_grow_water_min_bottom;
		return level >= params.leaves_grow_water_min_side;
	}

public:
	GrowLeaves(ServerEnvironment *env, NodeDefManager *ndef)
	{
		// g_settings->getBoolNoEx("grow_debug", grow_debug);
		g_settings->getBoolNoEx("grow_debug_fast", grow_debug_fast);

		std::vector<content_t> ids;
		ndef->getIds("group:grow_leaves", ids);
		for (const auto &id : ids) {
			const auto &cf = ndef->get(id);
			type_params.emplace(id, GrowParams(cf));
			if (!cf.liquid_alternative_source.empty())
				leaves_to_fruit[id] = ndef->getId(cf.liquid_alternative_source);
		}
	}
	virtual const std::vector<std::string> getTriggerContents() const override
	{
		return {"group:grow_leaves"};
	}
	virtual const std::vector<std::string> getRequiredNeighbors(
			bool activate) const override
	{
		return {};
	}
	u32 getNeighborsRange() override { return 1; }
	virtual float getTriggerInterval() override { return grow_debug_fast ? 0.1 : 10; }
	virtual u32 getTriggerChance() override { return grow_debug_fast ? 1 : 10; }
	bool getSimpleCatchUp() override { return true; }
	virtual pos_t getMinY() override { return -MAX_MAP_GENERATION_LIMIT; };
	virtual pos_t getMaxY() override { return MAX_MAP_GENERATION_LIMIT; };
	virtual void trigger(ServerEnvironment *env, v3pos_t p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider, v3pos_t,
			bool activate) override
	{
		ServerMap *map = &env->getServerMap();
		const auto *ndef = env->getGameDef()->ndef();
		float heat = map->updateBlockHeat(env, p);
		const auto c = n.getContent();
		const auto params = type_params.at(c);

		int n_water_level = get_leaves_water_level(n);
		const auto n_water_level_orig = n_water_level;

		const auto light = getLight(ndef, n);

		uint8_t i = 0;

		bool top_is_full_liquid = false;
		bool have_tree_or_soil = false;
		bool have_air = false;
		bool allow_grow_fruit = leaves_to_fruit.contains(c);
		const content_t c_fruit =
				allow_grow_fruit ? leaves_to_fruit.at(c) : CONTENT_IGNORE;
		// TODO: choose pump and grow direction by leaves type
		for (const auto &dir : leaves_grow_dirs) {
			const auto p_dir = p + dir;
			auto n_dir = map->getNodeTry(p_dir);
			if (!n_dir) {
				have_tree_or_soil = true; // dont remove when map busy
				allow_grow_fruit = false;
				have_air = false;
				continue;
			}
			const auto light_dir = getLight(ndef, n_dir);

			auto c_dir = n_dir.getContent();

			const auto &cf = ndef->get(c_dir);
			bool is_tree = cf.groups.contains("tree");
			bool is_leaves = cf.groups.contains("leaves");
			bool is_liquid = cf.groups.contains("liquid");
			bool top = !i;
			bool bottom = i + 1 == sizeof(leaves_grow_dirs) / sizeof(leaves_grow_dirs[0]);

			top_is_full_liquid =
					top && is_liquid && n_dir.getMaxLevel(ndef) == n_dir.getLevel(ndef);

			/*todo: shapes:
			o    sphere
			|   cypress
			___
			\ /
			*/

			if ((c_dir == c_fruit) || (!top && !bottom && !is_leaves))
				allow_grow_fruit = false;
			if (is_tree)
				allow_grow_fruit = false;

			if (!have_tree_or_soil)
				have_tree_or_soil =
						is_tree || is_leaves || cf.groups.contains("soil") || is_liquid;
			if (!have_air)
				have_air = c_dir == CONTENT_AIR;

			//DUMP("lv?", i, n_water_level, params.leaves_grow_heat_min, heat, params.leaves_grow_heat_min, light_dir, is_liquid, cf.buildable_to);

			if ((!params.leaves_grow_heat_min || heat >= params.leaves_grow_heat_min) &&
					(!params.leaves_grow_heat_max ||
							heat <= params.leaves_grow_heat_max) &&
					can_grow_leaves(params, n_water_level, top, bottom) &&
					light_dir >= params.leaves_grow_light_min && cf.buildable_to &&
					!is_liquid) {
				//if (grow_debug)DUMP("lv->lv  ", p.X, p.Y, p.Z, c_dir, c, l, n_water_level,n_water_level_orig, l, ndef->get(c_dir).name);
				map->setNode(p_dir, {c, n_dir.getParam1(), 1});
				--n_water_level;

				if (!myrand_range(0, 10))
					if (const auto block = map->getBlock(getNodeBlockPos(p_dir)); block) {
						block->setLightingExpired(true);
					}

			} else if (c_dir == c) {
				const auto l_dir = getLight(ndef, n_dir);

				auto wl_dir = get_leaves_water_level(n_dir);
				if (n_water_level > 1 && wl_dir < params.leaves_water_max &&
						l_dir >= light &&
						// todo: all up by type?
						wl_dir < n_water_level - 1 //(top ? -1 :
												   // bottom ? 1 : -2)
				) {
					--n_water_level;
					set_leaves_water_level(n_dir, ++wl_dir);
					map->setNode(p_dir, n_dir);

					//if (grow_debug)DUMP("lv pumpup2", p.Y, n_water_level, n_water_level_orig, wl_dir,top, bottom, c_dir);

					// Prefer pump up
					// todo: its like cypress, by settings
					if (top && params.leaves_grow_prefer_top) {
						break;
					}
				}
			}

			++i;
		}

		// DUMP(allow_grow_fruit, n_water_level, leaves_to_fruit_water_min, heat, leaves_to_fruit_heat_min);
		if (allow_grow_fruit && n_water_level >= params.leaves_to_fruit_water_min &&
				heat >= params.leaves_to_fruit_heat_min &&
				light >= params.leaves_to_fruit_light_min &&
				(grow_debug_fast || !myrand_range(0, params.leaves_to_fruit_chance))) {
			map->setNode(p, {c_fruit});
		} else if (
				(n_water_level >= 1 && // dont touch old static trees
						have_air &&
						((light < params.leaves_die_light_max &&
								 (light > 0 || activate ||
										 !myrand_range(0, params.leaves_die_chance))) ||
								((params.leaves_die_heat_max &&
										 heat < params.leaves_die_heat_max) ||
										(params.leaves_die_heat_min &&
												heat > params.leaves_die_heat_min)))) ||
				((!have_tree_or_soil ||
						 (params.leaves_die_from_liquid && top_is_full_liquid)) &&
						(activate || !myrand_range(0, 10)))) {
			//if (grow_debug) DUMP("lv die", p.X, p.Y, p.Z, have_tree_or_soil, n_water_level, l, heat);
			//if (!grow_debug_no_die)
			map->removeNodeWithEvent(p, false);
		} else if (n_water_level != n_water_level_orig) {
			// save if self level changed
			set_leaves_water_level(n, n_water_level);
			map->setNode(p, n);
		}
	}
};

void add_abm_grow_tree(ServerEnvironment *env, NodeDefManager *nodedef)
{
	bool grow = true;
	g_settings->getBoolNoEx("grow_tree", grow);
	if (grow) {
		env->addActiveBlockModifier(new GrowTree(env, nodedef));
		env->addActiveBlockModifier(new GrowLeaves(env, nodedef));
	}
}
