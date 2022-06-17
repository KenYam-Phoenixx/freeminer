/*
map.h
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#pragma once

#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include "util/unordered_map_hash.h"
#include "threading/concurrent_unordered_map.h"
#include <list>

#include "irrlichttypes_bloated.h"
#include "mapnode.h"
#include "constants.h"
#include "voxel.h"
#include "modifiedstate.h"
#include "util/container.h"
#include "util/metricsbackend.h"
#include "nodetimer.h"
#include "map_settings_manager.h"
#include "debug.h"

#include "mapblock.h"
#include <unordered_set>
#include "config.h"

class Settings;
class MapDatabase;
class ClientMap;
class MapSector;
class ServerMapSector;
class MapBlock;
class NodeMetadata;
class IGameDef;
class IRollbackManager;
class EmergeManager;
class MetricsBackend;
class ServerEnvironment;
struct BlockMakeData;
class Server;

/*
	MapEditEvent
*/

#define MAPTYPE_BASE 0
#define MAPTYPE_SERVER 1
#define MAPTYPE_CLIENT 2

enum MapEditEventType{
	// Node added (changed from air or something else to something)
	MEET_ADDNODE,
	// Node removed (changed to air)
	MEET_REMOVENODE,
	// Node swapped (changed without metadata change)
	MEET_SWAPNODE,
	// Node metadata changed
	MEET_BLOCK_NODE_METADATA_CHANGED,
	// Anything else (modified_blocks are set unsent)
	MEET_OTHER
};

struct MapEditEvent
{
	MapEditEventType type = MEET_OTHER;
	v3s16 p;
	MapNode n = CONTENT_AIR;
	std::set<v3s16> modified_blocks;
	bool is_private_change = false;

	MapEditEvent() = default;

	VoxelArea getArea() const
	{
		switch(type){
		case MEET_ADDNODE:
			return VoxelArea(p);
		case MEET_REMOVENODE:
			return VoxelArea(p);
		case MEET_SWAPNODE:
			return VoxelArea(p);
		case MEET_BLOCK_NODE_METADATA_CHANGED:
		{
			v3s16 np1 = p*MAP_BLOCKSIZE;
			v3s16 np2 = np1 + v3s16(1,1,1)*MAP_BLOCKSIZE - v3s16(1,1,1);
			return VoxelArea(np1, np2);
		}
		case MEET_OTHER:
		{
			VoxelArea a;
			for (v3s16 p : modified_blocks) {
				v3s16 np1 = p*MAP_BLOCKSIZE;
				v3s16 np2 = np1 + v3s16(1,1,1)*MAP_BLOCKSIZE - v3s16(1,1,1);
				a.addPoint(np1);
				a.addPoint(np2);
			}
			return a;
		}
		}
		return VoxelArea();
	}
};

class MapEventReceiver
{
public:
	// event shall be deleted by caller after the call.
	virtual void onMapEditEvent(const MapEditEvent &event) = 0;
};

class Map /*: public NodeContainer*/
{
public:
	Map(IGameDef *gamedef);
	virtual ~Map();
	DISABLE_CLASS_COPY(Map);

	virtual s32 mapType() const
	{
		return MAPTYPE_BASE;
	}

	/*
		Drop (client) or delete (server) the map.
	*/
	virtual void drop()
	{
		delete this;
	}

	void addEventReceiver(MapEventReceiver *event_receiver);
	void removeEventReceiver(MapEventReceiver *event_receiver);
	// event shall be deleted by caller after the call.
	void dispatchEvent(const MapEditEvent &event);

	// Returns InvalidPositionException if not found
	MapBlock * getBlockNoCreate(v3s16 p);
	// Returns NULL if not found
	MapBlock * getBlockNoCreateNoEx(v3POS p, bool trylock = false, bool nocache = false);
	MapBlockP getBlock(v3POS p, bool trylock = false, bool nocache = false);
	void getBlockCacheFlush();

	/* Server overrides */
	virtual MapBlock * emergeBlock(v3s16 p, bool create_blank=false)
	{ return getBlockNoCreateNoEx(p); }

	inline const NodeDefManager * getNodeDefManager() { return m_nodedef; }

	bool isValidPosition(v3s16 p);

	// throws InvalidPositionException if not found
	void setNode(v3s16 p, MapNode & n, bool no_light_check = 0);

	// Returns a CONTENT_IGNORE node if not found
	MapNode getNodeTry(v3s16 p);
	//MapNode getNodeNoLock(v3s16 p); // dont use
	// If is_valid_position is not NULL then this will be set to true if the
	// position is valid, otherwise false
	MapNode getNode(v3s16 p, bool *is_valid_position = NULL);

	/*
		These handle lighting but not faces.
	*/
	void addNodeAndUpdate(v3s16 p, MapNode n,
			std::map<v3s16, MapBlock*> &modified_blocks,
			bool remove_metadata = true,
			int fast = 0
			);
	void removeNodeAndUpdate(v3s16 p,
			std::map<v3s16, MapBlock*> &modified_blocks, int fast = 0);

	/*
		Wrappers for the latter ones.
		These emit events.
		Return true if succeeded, false if not.
	*/
	bool addNodeWithEvent(v3s16 p, MapNode n, bool remove_metadata = true);
	bool removeNodeWithEvent(v3s16 p);

	// Call these before and after saving of many blocks
	virtual void beginSave() {}
	virtual void endSave() {}

	virtual s32 save(ModifiedState save_level, float dedicated_server_step, bool breakable){ FATAL_ERROR("FIXME"); return 0;};

	// Server implements these.
	// Client leaves them as no-op.
	virtual bool saveBlock(MapBlock *block) { return false; }
	virtual bool deleteBlock(v3s16 blockpos) { return false; }

	/*
		Updates usage timers and unloads unused blocks and sectors.
		Saves modified blocks before unloading on MAPTYPE_SERVER.
	*/
	u32 timerUpdate(float uptime, float unload_timeout, u32 max_loaded_blocks, unsigned int max_cycle_ms = 100,
			std::vector<v3s16> *unloaded_blocks=NULL);

	/*
		Unloads all blocks with a zero refCount().
		Saves modified blocks before unloading on MAPTYPE_SERVER.
	*/
	void unloadUnreferencedBlocks(std::vector<v3s16> *unloaded_blocks=NULL);

	// For debug printing. Prints "Map: ", "ServerMap: " or "ClientMap: "
	virtual void PrintInfo(std::ostream &out);

	void transformLiquidsReal(Server *m_server, unsigned int max_cycle_ms);
	void transformLiquids(std::map<v3s16, MapBlock*> & modified_blocks,
			ServerEnvironment *env, 
			Server *m_server, unsigned int max_cycle_ms);
	/*
		Node metadata
		These are basically coordinate wrappers to MapBlock
	*/

	std::vector<v3s16> findNodesWithMetadata(v3s16 p1, v3s16 p2);
	NodeMetadata *getNodeMetadata(v3s16 p);

	/**
	 * Sets metadata for a node.
	 * This method sets the metadata for a given node.
	 * On success, it returns @c true and the object pointed to
	 * by @p meta is then managed by the system and should
	 * not be deleted by the caller.
	 *
	 * In case of failure, the method returns @c false and the
	 * caller is still responsible for deleting the object!
	 *
	 * @param p node coordinates
	 * @param meta pointer to @c NodeMetadata object
	 * @return @c true on success, false on failure
	 */
	bool setNodeMetadata(v3s16 p, NodeMetadata *meta);
	void removeNodeMetadata(v3s16 p);

	/*
		Node Timers
		These are basically coordinate wrappers to MapBlock
	*/

	NodeTimer getNodeTimer(v3s16 p);
	void setNodeTimer(const NodeTimer &t);
	void removeNodeTimer(v3s16 p);

	/*
		Variables
	*/

	u32 transforming_liquid_size();

//freeminer:
	void transforming_liquid_add(v3s16 p);
	v3s16 transforming_liquid_pop();
	std::atomic_uint m_liquid_step_flow;

	virtual s16 getHeat(v3s16 p, bool no_random = 0);
	virtual s16 getHumidity(v3s16 p, bool no_random = 0);

	virtual int getSurface(v3s16 basepos, int searchup, bool walkable_only) {
        return basepos.Y -1;
	}


// from old mapsector:
	typedef maybe_concurrent_unordered_map<v3POS, MapBlockP, v3POSHash, v3POSEqual> m_blocks_type;
	m_blocks_type m_blocks;
	//MapBlock * getBlockNoCreateNoEx(v3s16 & p);
	MapBlock * createBlankBlockNoInsert(v3s16 & p);
	MapBlock * createBlankBlock(v3s16 & p);
	bool insertBlock(MapBlock *block);
	void deleteBlock(MapBlockP block);
	std::unordered_map<MapBlockP, int> * m_blocks_delete;
	std::unordered_map<MapBlockP, int> m_blocks_delete_1, m_blocks_delete_2;
	unsigned int m_blocks_delete_time = 0;
	//void getBlocks(std::list<MapBlock*> &dest);
	concurrent_unordered_map<v3POS, int, v3POSHash, v3POSEqual> m_db_miss;

#if !ENABLE_THREADS
	locker<> m_nothread_locker;
#endif
#if ENABLE_THREADS && !HAVE_THREAD_LOCAL
	try_shared_mutex m_block_cache_mutex;
#endif
#if !HAVE_THREAD_LOCAL
	MapBlockP m_block_cache;
	v3POS m_block_cache_p;
#endif
	void copy_27_blocks_to_vm(MapBlock * block, VoxelManipulator & vmanip);

	bool propagateSunlight(v3POS pos, std::set<v3POS> & light_sources, bool remove_light=false);



// freeminer:
	bool isBlockOccluded(MapBlock *block, v3s16 cam_pos_nodes);
protected:
	friend class LuaVoxelManip;

	IGameDef *m_gamedef;
	std::set<MapEventReceiver*> m_event_receivers;

	// Queued transforming water nodes

	// This stores the properties of the nodes on the map.
	const NodeDefManager *m_nodedef;


	// freminer:
protected:
	u32 m_blocks_update_last;
	u32 m_blocks_save_last;

public:
	//concurrent_unordered_map<v3POS, bool, v3POSHash, v3POSEqual> m_transforming_liquid;
	std::mutex m_transforming_liquid_mutex;
	UniqueQueue<v3POS> m_transforming_liquid;
	typedef unordered_map_v3POS<int> lighting_map_t;
	std::mutex m_lighting_modified_mutex;
	std::map<v3POS, int> m_lighting_modified_blocks;
	std::map<unsigned int, lighting_map_t> m_lighting_modified_blocks_range;
	void lighting_modified_add(v3POS pos, int range = 5);
	std::atomic_uint time_life;
	u32 updateLighting(lighting_map_t & a_blocks, unordered_map_v3POS<int> & processed, unsigned int max_cycle_ms = 0);
	unsigned int updateLightingQueue(unsigned int max_cycle_ms, int & loopcount);
private:




	bool determineAdditionalOcclusionCheck(const v3s16 &pos_camera,
		const core::aabbox3d<s16> &block_bounds, v3s16 &check);
	bool isOccluded(const v3s16 &pos_camera, const v3s16 &pos_target,
		float step, float stepfac, float start_offset, float end_offset,
		u32 needed_count);

private:
	f32 m_transforming_liquid_loop_count_multiplier = 1.0f;
	u32 m_unprocessed_count = 0;
	u64 m_inc_trending_up_start_time = 0; // milliseconds
	bool m_queue_size_timer_started = false;
};

/*
	ServerMap

	This is the only map class that is able to generate map.
*/

class ServerMap : public Map
{
public:
	/*
		savedir: directory to which map data should be saved
	*/
	ServerMap(const std::string &savedir, IGameDef *gamedef, EmergeManager *emerge, MetricsBackend *mb);
	~ServerMap();

	s32 mapType() const
	{
		return MAPTYPE_SERVER;
	}

	/*
		Blocks are generated by using these and makeBlock().
	*/
	bool blockpos_over_mapgen_limit(v3s16 p);
	bool initBlockMake(v3s16 blockpos, BlockMakeData *data);
	void finishBlockMake(BlockMakeData *data,
		std::map<v3s16, MapBlock*> *changed_blocks);

	/*
		Get a block from somewhere.
		- Memory
		- Create blank
	*/
	MapBlock *createBlock(v3s16 p);

	/*
		Forcefully get a block from somewhere.
		- Memory
		- Load from disk
		- Create blank filled with CONTENT_IGNORE

	*/
	MapBlock *emergeBlock(v3s16 p, bool create_blank=false);

	/*
		Try to get a block.
		If it does not exist in memory, add it to the emerge queue.
		- Memory
		- Emerge Queue (deferred disk or generate)
	*/
	MapBlock *getBlockOrEmerge(v3s16 p3d);

	// Carries out any initialization necessary before block is sent
	void prepareBlock(MapBlock *block);

	// Helper for placing objects on ground level
	s16 findGroundLevel(v2POS p2d, bool cacheBlocks);

	bool isBlockInQueue(v3s16 pos);

	/*
		Database functions
	*/
	static MapDatabase *createDatabase(const std::string &name, const std::string &savedir, Settings &conf);

	// Call these before and after saving of blocks
	void beginSave();
	void endSave();

	s32 save(ModifiedState save_level, float dedicated_server_step = 0.1, bool breakable = 0);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	void listAllLoadedBlocks(std::vector<v3s16> &dst);

	MapgenParams *getMapgenParams();

	bool saveBlock(MapBlock *block);
	static bool saveBlock(MapBlock *block, MapDatabase *db, int compression_level = -1);
	MapBlock* loadBlock(v3s16 p);

	bool deleteBlock(v3s16 blockpos);

	void updateVManip(v3s16 pos);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);

	bool isSavingEnabled(){ return m_map_saving_enabled; }

	u64 getSeed();

	/*!
	 * Fixes lighting in one map block.
	 * May modify other blocks as well, as light can spread
	 * out of the specified block.
	 * Returns false if the block is not generated (so nothing
	 * changed), true otherwise.
	 */
	bool repairBlockLight(v3s16 blockpos,
		std::map<v3s16, MapBlock *> *modified_blocks);

//freeminer:
	virtual s16 updateBlockHeat(ServerEnvironment *env, v3POS p, MapBlock *block = nullptr, unordered_map_v3POS<s16> *cache = nullptr);
	virtual s16 updateBlockHumidity(ServerEnvironment *env, v3POS p, MapBlock *block = nullptr, unordered_map_v3POS<s16> *cache = nullptr);

	//getSurface level starting on basepos.y up to basepos.y + searchup
	//returns basepos.y -1 if no surface has been found
	// (due to limited data range of basepos.y this will always give a unique
	// return value as long as minetest is compiled at least on 32bit architecture)
	int getSurface(v3s16 basepos, int searchup, bool walkable_only);
//end of freeminer

	MapSettingsManager settings_mgr;

private:
	// Emerge manager
	EmergeManager *m_emerge;

public:
	std::string m_savedir;
	bool m_map_saving_enabled;
	bool m_map_loading_enabled;
	concurrent_unordered_map<v3POS, unsigned int, v3POSHash, v3POSEqual> m_mapgen_process;
private:

	int m_map_compression_level;

	std::set<v3s16> m_chunks_in_progress;

	/*
		Metadata is re-written on disk only if this is true.
		This is reset to false when written on disk.
	*/
	bool m_map_metadata_changed = true;
public:
	MapDatabase *dbase = nullptr;
private:
	MapDatabase *dbase_ro = nullptr;

	MetricCounterPtr m_save_time_counter;
};

#if !ENABLE_THREADS
	#define MAP_NOTHREAD_LOCK(map) auto lock_map = map->m_nothread_locker.lock_unique_rec();
#else
	#define MAP_NOTHREAD_LOCK(map) ;
#endif

#define VMANIP_BLOCK_DATA_INEXIST     1
#define VMANIP_BLOCK_CONTAINS_CIGNORE 2

class MMVManip : public VoxelManipulator
{
public:
	MMVManip(Map *map);
	virtual ~MMVManip() = default;

	virtual void clear()
	{
		VoxelManipulator::clear();
		m_loaded_blocks.clear();
	}

	void initialEmerge(v3s16 blockpos_min, v3s16 blockpos_max,
		bool load_if_inexistent = true);

	// This is much faster with big chunks of generated data
	void blitBackAll(std::map<v3s16, MapBlock*> * modified_blocks,
		bool overwrite_generated = true);

	bool m_is_dirty = false;

protected:
public:
	Map *m_map;
protected:
	/*
		key = blockpos
		value = flags describing the block
	*/
	std::map<v3s16, u8> m_loaded_blocks;
};
