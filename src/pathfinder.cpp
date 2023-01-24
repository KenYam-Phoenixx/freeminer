/*
Minetest
Copyright (C) 2013 sapier, sapier at gmx dot net
Copyright (C) 2016 est31, <MTest31@outlook.com>

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

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include "pathfinder.h"
#include "map.h"
#include "nodedef.h"

//#define PATHFINDER_DEBUG
//#define PATHFINDER_CALC_TIME

#ifdef PATHFINDER_DEBUG
	#include <string>
#endif
#ifdef PATHFINDER_DEBUG
	#include <iomanip>
#endif
#ifdef PATHFINDER_CALC_TIME
	#include <sys/time.h>
#endif

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

#define LVL "(" << level << ")" <<

#ifdef PATHFINDER_DEBUG
#define DEBUG_OUT(a)     std::cout << a
#define INFO_TARGET      std::cout
#define VERBOSE_TARGET   std::cout
#define ERROR_TARGET     std::cout
#else
#define DEBUG_OUT(a)     while(0)
#define INFO_TARGET      infostream << "Pathfinder: "
#define VERBOSE_TARGET   verbosestream << "Pathfinder: "
#define ERROR_TARGET     warningstream << "Pathfinder: "
#endif

#define PATHFINDER_MAX_WAYPOINTS 700

/******************************************************************************/
/* Class definitions                                                          */
/******************************************************************************/


/** representation of cost in specific direction */
class PathCost {
public:

	/** default constructor */
	PathCost() = default;

	/** copy constructor */
	PathCost(const PathCost &b);

	/** assignment operator */
	PathCost &operator= (const PathCost &b);

	bool valid = false;              /**< movement is possible         */
	int  value = 0;                  /**< cost of movement             */
	int  y_change = 0;               /**< change of y position of movement */
	bool updated = false;            /**< this cost has ben calculated */

};


/** representation of a mapnode to be used for pathfinding */
class PathGridnode {

public:
	/** default constructor */
	PathGridnode() = default;

	/** copy constructor */
	PathGridnode(const PathGridnode &b);

	/**
	 * assignment operator
	 * @param b node to copy
	 */
	PathGridnode &operator= (const PathGridnode &b);

	/**
	 * read cost in a specific direction
	 * @param dir direction of cost to fetch
	 */
	PathCost getCost(v3pos_t dir);

	/**
	 * set cost value for movement
	 * @param dir direction to set cost for
	 * @cost cost to set
	 */
	void      setCost(v3pos_t dir, const PathCost &cost);

	bool      valid = false;               /**< node is on surface                    */
	bool      target = false;              /**< node is target position               */
	bool      source = false;              /**< node is stating position              */
	int       totalcost = -1;              /**< cost to move here from starting point */
	int       estimated_cost = -1;         /**< totalcost + heuristic cost to end     */
	v3pos_t     sourcedir;                   /**< origin of movement for current cost   */
	v3pos_t     pos;                         /**< real position of node                 */
	PathCost directions[4];                /**< cost in different directions          */
	bool      is_closed = false;           /**< for A* search: if true, is in closed list */
	bool      is_open = false;             /**< for A* search: if true, is in open list */

	/* debug values */
	bool      is_element = false;          /**< node is element of path detected      */
	char      type = 'u';                  /**< Type of pathfinding node.
	                                        * u = unknown
	                                        * i = invalid
	                                        * s = surface (walkable node)
	                                        * - = non-walkable node (e.g. air) above surface
	                                        * g = other non-walkable node
                                                */
};

class Pathfinder;
class PathfinderCompareHeuristic;

/** Abstract class to manage the map data */
class GridNodeContainer {
public:
	virtual PathGridnode &access(v3pos_t p)=0;
	virtual ~GridNodeContainer() = default;

protected:
	Pathfinder *m_pathf;

	void initNode(v3pos_t ipos, PathGridnode *p_node);
};

class ArrayGridNodeContainer : public GridNodeContainer {
public:
	virtual ~ArrayGridNodeContainer() = default;

	ArrayGridNodeContainer(Pathfinder *pathf, v3pos_t dimensions);
	virtual PathGridnode &access(v3pos_t p);

private:
	int m_x_stride;
	int m_y_stride;
	std::vector<PathGridnode> m_nodes_array;
};

class MapGridNodeContainer : public GridNodeContainer {
public:
	virtual ~MapGridNodeContainer() = default;

	MapGridNodeContainer(Pathfinder *pathf);
	virtual PathGridnode &access(v3pos_t p);
private:
	std::map<v3pos_t, PathGridnode> m_nodes;
};

/** class doing pathfinding */
class Pathfinder {

public:
	Pathfinder() = delete;
	Pathfinder(Map *map, const NodeDefManager *ndef) : m_map(map), m_ndef(ndef) {}

	~Pathfinder();

	/**
	 * path evaluation function
	 * @param env environment to look for path
	 * @param source origin of path
	 * @param destination end position of path
	 * @param searchdistance maximum number of nodes to look in each direction
	 * @param max_jump maximum number of blocks a path may jump up
	 * @param max_drop maximum number of blocks a path may drop
	 * @param algo Algorithm to use for finding a path
	 */
	std::vector<v3pos_t> getPath(v3pos_t source,
			v3pos_t destination,
			unsigned int searchdistance,
			unsigned int max_jump,
			unsigned int max_drop,
			PathAlgorithm algo);

private:
	/* helper functions */

	/**
	 * transform index pos to mappos
	 * @param ipos a index position
	 * @return map position
	 */
	v3pos_t          getRealPos(v3pos_t ipos);

	/**
	 * transform mappos to index pos
	 * @param pos a real pos
	 * @return index position
	 */
	v3pos_t          getIndexPos(v3pos_t pos);

	/**
	 * get gridnode at a specific index position
	 * @param ipos index position
	 * @return gridnode for index
	 */
	PathGridnode &getIndexElement(v3pos_t ipos);

	/**
	 * Get gridnode at a specific index position
	 * @return gridnode for index
	 */
	PathGridnode &getIdxElem(pos_t x, pos_t y, pos_t z);

	/**
	 * invert a 3D position (change sign of coordinates)
	 * @param pos 3D position
	 * @return pos *-1
	 */
	v3pos_t          invert(v3pos_t pos);

	/**
	 * check if a index is within current search area
	 * @param index position to validate
	 * @return true/false
	 */
	bool           isValidIndex(v3pos_t index);


	/* algorithm functions */

	/**
	 * calculate 2D Manhattan distance to target
	 * @param pos position to calc distance
	 * @return integer distance
	 */
	int           getXZManhattanDist(v3pos_t pos);

	/**
	 * calculate cost of movement
	 * @param pos real world position to start movement
	 * @param dir direction to move to
	 * @return cost information
	 */
	PathCost     calcCost(v3pos_t pos, v3pos_t dir);

	/**
	 * recursive update whole search areas total cost information
	 * @param ipos position to check next
	 * @param srcdir positionc checked last time
	 * @param total_cost cost of moving to ipos
	 * @param level current recursion depth
	 * @return true/false path to destination has been found
	 */
	bool          updateAllCosts(v3pos_t ipos, v3pos_t srcdir, int current_cost, int level);

	/**
	 * try to find a path to destination using a heuristic function
	 * to estimate distance to target (A* search algorithm)
	 * @param isource start position (index pos)
	 * @param idestination end position (index pos)
	 * @return true/false path to destination has been found
	 */
	bool          updateCostHeuristic(v3pos_t isource, v3pos_t idestination);

	/**
	 * build a vector containing all nodes from destination to source;
	 * to be called after the node costs have been processed
	 * @param path vector to add nodes to
	 * @param ipos initial pos to check (index pos)
	 * @return true/false path has been fully built
	 */
	bool          buildPath(std::vector<v3pos_t> &path, v3pos_t ipos);

	/**
	 * go downwards from a position until some barrier
	 * is hit.
	 * @param pos position from which to go downwards
	 * @param max_down maximum distance to go downwards
	 * @return new position after movement; if too far down,
	 * pos is returned
	 */
	v3pos_t         walkDownwards(v3pos_t pos, unsigned int max_down);

	/* variables */
	int m_max_index_x = 0;            /**< max index of search area in x direction  */
	int m_max_index_y = 0;            /**< max index of search area in y direction  */
	int m_max_index_z = 0;            /**< max index of search area in z direction  */

	int m_maxdrop = 0;                /**< maximum number of blocks a path may drop */
	int m_maxjump = 0;                /**< maximum number of blocks a path may jump */
	int m_min_target_distance = 0;    /**< current smalest path to target           */

	bool m_prefetch = true;              /**< prefetch cost data                       */

	v3pos_t m_start;                /**< source position                          */
	v3pos_t m_destination;          /**< destination position                     */

	core::aabbox3d<pos_t> m_limits; /**< position limits in real map coordinates  */

	/** contains all map data already collected and analyzed.
		Access it via the getIndexElement/getIdxElem methods. */
	friend class GridNodeContainer;
	GridNodeContainer *m_nodes_container = nullptr;

	Map *m_map = nullptr;

	const NodeDefManager *m_ndef = nullptr;

	friend class PathfinderCompareHeuristic;

#ifdef PATHFINDER_DEBUG

	/**
	 * print collected cost information
	 */
	void printCost();

	/**
	 * print collected cost information in a specific direction
	 * @param dir direction to print
	 */
	void printCost(PathDirections dir);

	/**
	 * print type of node as evaluated
	 */
	void printType();

	/**
	 * print pathlenght for all nodes in search area
	 */
	void printPathLen();

	/**
	 * print a path
	 * @param path path to show
	 */
	void printPath(std::vector<v3pos_t> path);

	/**
	 * print y direction for all movements
	 */
	void printYdir();

	/**
	 * print y direction for moving in a specific direction
	 * @param dir direction to show data
	 */
	void printYdir(PathDirections dir);

	/**
	 * helper function to translate a direction to speaking text
	 * @param dir direction to translate
	 * @return textual name of direction
	 */
	std::string dirToName(PathDirections dir);
#endif
};

/** Helper class for the open list priority queue in the A* pathfinder
 *  to sort the pathfinder nodes by cost.
 */
class PathfinderCompareHeuristic
{
	private:
		Pathfinder *myPathfinder;
	public:
		PathfinderCompareHeuristic(Pathfinder *pf)
		{
			myPathfinder = pf;
		}
		bool operator() (v3pos_t pos1, v3pos_t pos2) {
			v3pos_t ipos1 = myPathfinder->getIndexPos(pos1);
			v3pos_t ipos2 = myPathfinder->getIndexPos(pos2);
			PathGridnode &g_pos1 = myPathfinder->getIndexElement(ipos1);
			PathGridnode &g_pos2 = myPathfinder->getIndexElement(ipos2);
			if (!g_pos1.valid)
				return false;
			if (!g_pos2.valid)
				return false;
			return g_pos1.estimated_cost > g_pos2.estimated_cost;
		}
};

/******************************************************************************/
/* implementation                                                             */
/******************************************************************************/

std::vector<v3pos_t> get_path(Map* map, const NodeDefManager *ndef,
		v3pos_t source,
		v3pos_t destination,
		unsigned int searchdistance,
		unsigned int max_jump,
		unsigned int max_drop,
		PathAlgorithm algo)
{
	return Pathfinder(map, ndef).getPath(source, destination,
				searchdistance, max_jump, max_drop, algo);
}

/******************************************************************************/
PathCost::PathCost(const PathCost &b)
{
	valid     = b.valid;
	y_change  = b.y_change;
	value     = b.value;
	updated   = b.updated;
}

/******************************************************************************/
PathCost &PathCost::operator= (const PathCost &b)
{
	valid     = b.valid;
	y_change  = b.y_change;
	value     = b.value;
	updated   = b.updated;

	return *this;
}

/******************************************************************************/
PathGridnode::PathGridnode(const PathGridnode &b)
:	valid(b.valid),
	target(b.target),
	source(b.source),
	totalcost(b.totalcost),
	sourcedir(b.sourcedir),
	pos(b.pos),
	is_element(b.is_element),
	type(b.type)
{

	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];
}

/******************************************************************************/
PathGridnode &PathGridnode::operator= (const PathGridnode &b)
{
	valid      = b.valid;
	target     = b.target;
	source     = b.source;
	is_element = b.is_element;
	totalcost  = b.totalcost;
	sourcedir  = b.sourcedir;
	pos        = b.pos;
	type       = b.type;

	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];

	return *this;
}

/******************************************************************************/
PathCost PathGridnode::getCost(v3pos_t dir)
{
	if (dir.X > 0) {
		return directions[DIR_XP];
	}
	if (dir.X < 0) {
		return directions[DIR_XM];
	}
	if (dir.Z > 0) {
		return directions[DIR_ZP];
	}
	if (dir.Z < 0) {
		return directions[DIR_ZM];
	}
	PathCost retval;
	return retval;
}

/******************************************************************************/
void PathGridnode::setCost(v3pos_t dir, const PathCost &cost)
{
	if (dir.X > 0) {
		directions[DIR_XP] = cost;
	}
	if (dir.X < 0) {
		directions[DIR_XM] = cost;
	}
	if (dir.Z > 0) {
		directions[DIR_ZP] = cost;
	}
	if (dir.Z < 0) {
		directions[DIR_ZM] = cost;
	}
}

void GridNodeContainer::initNode(v3pos_t ipos, PathGridnode *p_node)
{
	const NodeDefManager *ndef = m_pathf->m_ndef;
	PathGridnode &elem = *p_node;

	v3pos_t realpos = m_pathf->getRealPos(ipos);

	MapNode current = m_pathf->m_map->getNode(realpos);
	MapNode below   = m_pathf->m_map->getNode(realpos + v3pos_t(0, -1, 0));


	if ((current.param0 == CONTENT_IGNORE) ||
			(below.param0 == CONTENT_IGNORE)) {
		DEBUG_OUT("Pathfinder: " << PP(realpos) <<
			" current or below is invalid element" << std::endl);
		if (current.param0 == CONTENT_IGNORE) {
			elem.type = 'i';
			DEBUG_OUT(PP(ipos) << ": " << 'i' << std::endl);
		}
		return;
	}

	//don't add anything if it isn't an air node
	if (ndef->get(current).walkable || !ndef->get(below).walkable) {
			DEBUG_OUT("Pathfinder: " << PP(realpos)
				<< " not on surface" << std::endl);
			if (ndef->get(current).walkable) {
				elem.type = 's';
				DEBUG_OUT(PP(ipos) << ": " << 's' << std::endl);
			} else {
				elem.type = '-';
				DEBUG_OUT(PP(ipos) << ": " << '-' << std::endl);
			}
			return;
	}

	elem.valid = true;
	elem.pos   = realpos;
	elem.type  = 'g';
	DEBUG_OUT(PP(ipos) << ": " << 'a' << std::endl);

	if (m_pathf->m_prefetch) {
		elem.directions[DIR_XP] = m_pathf->calcCost(realpos, v3pos_t( 1, 0, 0));
		elem.directions[DIR_XM] = m_pathf->calcCost(realpos, v3pos_t(-1, 0, 0));
		elem.directions[DIR_ZP] = m_pathf->calcCost(realpos, v3pos_t( 0, 0, 1));
		elem.directions[DIR_ZM] = m_pathf->calcCost(realpos, v3pos_t( 0, 0,-1));
	}
}

ArrayGridNodeContainer::ArrayGridNodeContainer(Pathfinder *pathf, v3pos_t dimensions) :
	m_x_stride(dimensions.Y * dimensions.Z),
	m_y_stride(dimensions.Z)
{
	m_pathf = pathf;

	m_nodes_array.resize(dimensions.X * dimensions.Y * dimensions.Z);
	INFO_TARGET << "Pathfinder ArrayGridNodeContainer constructor." << std::endl;
	for (int x = 0; x < dimensions.X; x++) {
		for (int y = 0; y < dimensions.Y; y++) {
			for (int z= 0; z < dimensions.Z; z++) {
				v3pos_t ipos(x, y, z);
				initNode(ipos, &access(ipos));
			}
		}
	}
}

PathGridnode &ArrayGridNodeContainer::access(v3pos_t p)
{
	return m_nodes_array[p.X * m_x_stride + p.Y * m_y_stride + p.Z];
}

MapGridNodeContainer::MapGridNodeContainer(Pathfinder *pathf)
{
	m_pathf = pathf;
}

PathGridnode &MapGridNodeContainer::access(v3pos_t p)
{
	std::map<v3pos_t, PathGridnode>::iterator it = m_nodes.find(p);
	if (it != m_nodes.end()) {
		return it->second;
	}
	PathGridnode &n = m_nodes[p];
	initNode(p, &n);
	return n;
}



/******************************************************************************/
std::vector<v3pos_t> Pathfinder::getPath(v3pos_t source,
							v3pos_t destination,
							unsigned int searchdistance,
							unsigned int max_jump,
							unsigned int max_drop,
							PathAlgorithm algo)
{
#ifdef PATHFINDER_CALC_TIME
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	std::vector<v3pos_t> retval;

	//initialization
	m_maxjump = max_jump;
	m_maxdrop = max_drop;
	m_start       = source;
	m_destination = destination;
	m_min_target_distance = -1;
	m_prefetch = true;

	if (algo == PA_PLAIN_NP) {
		m_prefetch = false;
	}

	//calculate boundaries within we're allowed to search
	int min_x = MYMIN(source.X, destination.X);
	int max_x = MYMAX(source.X, destination.X);

	int min_y = MYMIN(source.Y, destination.Y);
	int max_y = MYMAX(source.Y, destination.Y);

	int min_z = MYMIN(source.Z, destination.Z);
	int max_z = MYMAX(source.Z, destination.Z);

	m_limits.MinEdge.X = min_x - searchdistance;
	m_limits.MinEdge.Y = min_y - searchdistance;
	m_limits.MinEdge.Z = min_z - searchdistance;

	m_limits.MaxEdge.X = max_x + searchdistance;
	m_limits.MaxEdge.Y = max_y + searchdistance;
	m_limits.MaxEdge.Z = max_z + searchdistance;

	v3pos_t diff = m_limits.MaxEdge - m_limits.MinEdge;

	m_max_index_x = diff.X;
	m_max_index_y = diff.Y;
	m_max_index_z = diff.Z;

	delete m_nodes_container;
	if (diff.getLength() > 5) {
		m_nodes_container = new MapGridNodeContainer(this);
	} else {
		m_nodes_container = new ArrayGridNodeContainer(this, diff);
	}
#ifdef PATHFINDER_DEBUG
	printType();
	printCost();
	printYdir();
#endif

	//fail if source or destination is walkable
	MapNode node_at_pos = m_map->getNode(destination);
	if (m_ndef->get(node_at_pos).walkable) {
		VERBOSE_TARGET << "Destination is walkable. " <<
				"Pos: " << PP(destination) << std::endl;
		return retval;
	}
	node_at_pos = m_map->getNode(source);
	if (m_ndef->get(node_at_pos).walkable) {
		VERBOSE_TARGET << "Source is walkable. " <<
				"Pos: " << PP(source) << std::endl;
		return retval;
	}

	//If source pos is hovering above air, drop
	//to the first walkable node (up to m_maxdrop).
	//All algorithms expect the source pos to be *directly* above
	//a walkable node.
	v3pos_t true_source = v3pos_t(source);
	source = walkDownwards(source, m_maxdrop);

	//If destination pos is hovering above air, go downwards
	//to the first walkable node (up to m_maxjump).
	//This means a hovering destination pos could be reached
	//by a final upwards jump.
	v3pos_t true_destination = v3pos_t(destination);
	destination = walkDownwards(destination, m_maxjump);

	//validate and mark start and end pos

	v3pos_t StartIndex  = getIndexPos(source);
	v3pos_t EndIndex    = getIndexPos(destination);

	PathGridnode &startpos = getIndexElement(StartIndex);
	PathGridnode &endpos   = getIndexElement(EndIndex);

	if (!startpos.valid) {
		VERBOSE_TARGET << "Invalid startpos " <<
				"Index: " << PP(StartIndex) <<
				"Realpos: " << PP(getRealPos(StartIndex)) << std::endl;
		return retval;
	}
	if (!endpos.valid) {
		VERBOSE_TARGET << "Invalid stoppos " <<
				"Index: " << PP(EndIndex) <<
				"Realpos: " << PP(getRealPos(EndIndex)) << std::endl;
		return retval;
	}

	endpos.target      = true;
	startpos.source    = true;
	startpos.totalcost = 0;

	bool update_cost_retval = false;

	//calculate node costs
	switch (algo) {
		case PA_DIJKSTRA:
			update_cost_retval = updateAllCosts(StartIndex, v3pos_t(0, 0, 0), 0, 0);
			break;
		case PA_PLAIN_NP:
		case PA_PLAIN:
			update_cost_retval = updateCostHeuristic(StartIndex, EndIndex);
			break;
		default:
			ERROR_TARGET << "Missing PathAlgorithm" << std::endl;
			break;
	}

	if (update_cost_retval) {

#ifdef PATHFINDER_DEBUG
		std::cout << "Path to target found!" << std::endl;
		printPathLen();
#endif

		//find path
		std::vector<v3pos_t> index_path;
		buildPath(index_path, EndIndex);
		//Now we have a path of index positions,
		//and it's in reverse.
		//The "true" start or end position might be missing
		//since those have been given special treatment.

#ifdef PATHFINDER_DEBUG
		std::cout << "Index path:" << std::endl;
		printPath(index_path);
#endif
		//from here we'll make the final changes to the path
		std::vector<v3pos_t> full_path;

		//calculate required size
		int full_path_size = index_path.size();
		if (source != true_source) {
			full_path_size++;
		}
		if (destination != true_destination) {
			full_path_size++;
		}
		full_path.reserve(full_path_size);

		//manually add true_source to start of path, if needed
		if (source != true_source) {
			full_path.push_back(true_source);
		}
		//convert all index positions to "normal" positions and insert
		//them into full_path in reverse
		std::vector<v3pos_t>::reverse_iterator rit = index_path.rbegin();
		for (; rit != index_path.rend(); ++rit) {
			full_path.push_back(getIndexElement(*rit).pos);
		}
		//manually add true_destination to end of path, if needed
		if (destination != true_destination) {
			full_path.push_back(true_destination);
		}

		//Done! We now have a complete path of normal positions.


#ifdef PATHFINDER_DEBUG
		std::cout << "Full path:" << std::endl;
		printPath(full_path);
#endif
#ifdef PATHFINDER_CALC_TIME
		timespec ts2;
		clock_gettime(CLOCK_REALTIME, &ts2);

		int ms = (ts2.tv_nsec - ts.tv_nsec)/(1000*1000);
		int us = ((ts2.tv_nsec - ts.tv_nsec) - (ms*1000*1000))/1000;
		int ns = ((ts2.tv_nsec - ts.tv_nsec) - ( (ms*1000*1000) + (us*1000)));


		std::cout << "Calculating path took: " << (ts2.tv_sec - ts.tv_sec) <<
				"s " << ms << "ms " << us << "us " << ns << "ns " << std::endl;
#endif
		return full_path;
	}
	else {
#ifdef PATHFINDER_DEBUG
		printPathLen();
#endif
		INFO_TARGET << "No path found" << std::endl;
	}


	//return
	return retval;
}

Pathfinder::~Pathfinder()
{
	delete m_nodes_container;
}
/******************************************************************************/
v3pos_t Pathfinder::getRealPos(v3pos_t ipos)
{
	return m_limits.MinEdge + ipos;
}

/******************************************************************************/
PathCost Pathfinder::calcCost(v3pos_t pos, v3pos_t dir)
{
	PathCost retval;

	retval.updated = true;

	v3pos_t pos2 = pos + dir;

	//check limits
	if (!m_limits.isPointInside(pos2)) {
		DEBUG_OUT("Pathfinder: " << PP(pos2) <<
				" no cost -> out of limits" << std::endl);
		return retval;
	}

	MapNode node_at_pos2 = m_map->getNode(pos2);

	//did we get information about node?
	if (node_at_pos2.param0 == CONTENT_IGNORE ) {
			VERBOSE_TARGET << "Pathfinder: (1) area at pos: "
					<< PP(pos2) << " not loaded";
			return retval;
	}

	if (!m_ndef->get(node_at_pos2).walkable) {
		MapNode node_below_pos2 =
			m_map->getNode(pos2 + v3pos_t(0, -1, 0));

		//did we get information about node?
		if (node_below_pos2.param0 == CONTENT_IGNORE ) {
				VERBOSE_TARGET << "Pathfinder: (2) area at pos: "
					<< PP((pos2 + v3pos_t(0, -1, 0))) << " not loaded";
				return retval;
		}

		//test if the same-height neighbor is suitable
		if (m_ndef->get(node_below_pos2).walkable) {
			//SUCCESS!
			retval.valid = true;
			retval.value = 1;
			retval.y_change = 0;
			DEBUG_OUT("Pathfinder: "<< PP(pos)
					<< " cost same height found" << std::endl);
		}
		else {
			//test if we can fall a couple of nodes (m_maxdrop)
			v3pos_t testpos = pos2 + v3pos_t(0, -1, 0);
			MapNode node_at_pos = m_map->getNode(testpos);

			while ((node_at_pos.param0 != CONTENT_IGNORE) &&
					(!m_ndef->get(node_at_pos).walkable) &&
					(testpos.Y > m_limits.MinEdge.Y)) {
				testpos += v3pos_t(0, -1, 0);
				node_at_pos = m_map->getNode(testpos);
			}

			//did we find surface?
			if ((testpos.Y >= m_limits.MinEdge.Y) &&
					(node_at_pos.param0 != CONTENT_IGNORE) &&
					(m_ndef->get(node_at_pos).walkable)) {
				if ((pos2.Y - testpos.Y - 1) <= m_maxdrop) {
					//SUCCESS!
					retval.valid = true;
					retval.value = 2;
					//difference of y-pos +1 (target node is ABOVE solid node)
					retval.y_change = ((testpos.Y - pos2.Y) +1);
					DEBUG_OUT("Pathfinder cost below height found" << std::endl);
				}
				else {
					INFO_TARGET << "Pathfinder:"
							" distance to surface below too big: "
							<< (testpos.Y - pos2.Y) << " max: " << m_maxdrop
							<< std::endl;
				}
			}
			else {
				DEBUG_OUT("Pathfinder: no surface below found" << std::endl);
			}
		}
	}
	else {
		//test if we can jump upwards (m_maxjump)

		v3pos_t targetpos = pos2; // position for jump target
		v3pos_t jumppos = pos; // position for checking if jumping space is free
		MapNode node_target = m_map->getNode(targetpos);
		MapNode node_jump = m_map->getNode(jumppos);
		bool headbanger = false; // true if anything blocks jumppath

		while ((node_target.param0 != CONTENT_IGNORE) &&
				(m_ndef->get(node_target).walkable) &&
				(targetpos.Y < m_limits.MaxEdge.Y)) {
			//if the jump would hit any solid node, discard
			if ((node_jump.param0 == CONTENT_IGNORE) ||
					(m_ndef->get(node_jump).walkable)) {
					headbanger = true;
				break;
			}
			targetpos += v3pos_t(0, 1, 0);
			jumppos   += v3pos_t(0, 1, 0);
			node_target = m_map->getNode(targetpos);
			node_jump   = m_map->getNode(jumppos);

		}
		//check headbanger one last time
		if ((node_jump.param0 == CONTENT_IGNORE) ||
			(m_ndef->get(node_jump).walkable)) {
			headbanger = true;
		}

		//did we find surface without banging our head?
		if ((!headbanger) && (targetpos.Y <= m_limits.MaxEdge.Y) &&
				(!m_ndef->get(node_target).walkable)) {

			if (targetpos.Y - pos2.Y <= m_maxjump) {
				//SUCCESS!
				retval.valid = true;
				retval.value = 2;
				retval.y_change = (targetpos.Y - pos2.Y);
				DEBUG_OUT("Pathfinder cost above found" << std::endl);
			}
			else {
				DEBUG_OUT("Pathfinder: distance to surface above too big: "
						<< (targetpos.Y - pos2.Y) << " max: " << m_maxjump
						<< std::endl);
			}
		}
		else {
			DEBUG_OUT("Pathfinder: no surface above found" << std::endl);
		}
	}
	return retval;
}

/******************************************************************************/
v3pos_t Pathfinder::getIndexPos(v3pos_t pos)
{
	return pos - m_limits.MinEdge;
}

/******************************************************************************/
PathGridnode &Pathfinder::getIndexElement(v3pos_t ipos)
{
	return m_nodes_container->access(ipos);
}

/******************************************************************************/
inline PathGridnode &Pathfinder::getIdxElem(pos_t x, pos_t y, pos_t z)
{
	return m_nodes_container->access(v3pos_t(x,y,z));
}

/******************************************************************************/
bool Pathfinder::isValidIndex(v3pos_t index)
{
	if (	(index.X < m_max_index_x) &&
			(index.Y < m_max_index_y) &&
			(index.Z < m_max_index_z) &&
			(index.X >= 0) &&
			(index.Y >= 0) &&
			(index.Z >= 0))
		return true;

	return false;
}

/******************************************************************************/
v3pos_t Pathfinder::invert(v3pos_t pos)
{
	v3pos_t retval = pos;

	retval.X *=-1;
	retval.Y *=-1;
	retval.Z *=-1;

	return retval;
}

/******************************************************************************/
bool Pathfinder::updateAllCosts(v3pos_t ipos,
								v3pos_t srcdir,
								int current_cost,
								int level)
{
	PathGridnode &g_pos = getIndexElement(ipos);
	g_pos.totalcost = current_cost;
	g_pos.sourcedir = srcdir;

	level ++;

	//check if target has been found
	if (g_pos.target) {
		m_min_target_distance = current_cost;
		DEBUG_OUT(LVL " Pathfinder: target found!" << std::endl);
		return true;
	}

	bool retval = false;

	// the 4 cardinal directions
	const static v3pos_t directions[4] = {
		v3pos_t(1,0, 0),
		v3pos_t(-1,0, 0),
		v3pos_t(0,0, 1),
		v3pos_t(0,0,-1)
	};

	for (v3pos_t direction : directions) {
		if (direction != srcdir) {
			PathCost cost = g_pos.getCost(direction);

			if (cost.valid) {
				direction.Y = cost.y_change;

				v3pos_t ipos2 = ipos + direction;

				if (!isValidIndex(ipos2)) {
					DEBUG_OUT(LVL " Pathfinder: " << PP(ipos2) <<
						" out of range, max=" << PP(m_limits.MaxEdge) << std::endl);
					continue;
				}

				PathGridnode &g_pos2 = getIndexElement(ipos2);

				if (!g_pos2.valid) {
					VERBOSE_TARGET << LVL "Pathfinder: no data for new position: "
												<< PP(ipos2) << std::endl;
					continue;
				}

				assert(cost.value > 0);

				int new_cost = current_cost + cost.value;

				// check if there already is a smaller path
				if ((m_min_target_distance > 0) &&
						(m_min_target_distance < new_cost)) {
					return false;
				}

				if ((g_pos2.totalcost < 0) ||
						(g_pos2.totalcost > new_cost)) {
					DEBUG_OUT(LVL "Pathfinder: updating path at: "<<
							PP(ipos2) << " from: " << g_pos2.totalcost << " to "<<
							new_cost << std::endl);
					if (updateAllCosts(ipos2, invert(direction),
											new_cost, level)) {
						retval = true;
						}
					}
				else {
					DEBUG_OUT(LVL "Pathfinder:"
							" already found shorter path to: "
							<< PP(ipos2) << std::endl);
				}
			}
			else {
				DEBUG_OUT(LVL "Pathfinder:"
						" not moving to invalid direction: "
						<< PP(directions[i]) << std::endl);
			}
		}
	}
	return retval;
}

/******************************************************************************/
int Pathfinder::getXZManhattanDist(v3pos_t pos)
{
	int min_x = MYMIN(pos.X, m_destination.X);
	int max_x = MYMAX(pos.X, m_destination.X);
	int min_z = MYMIN(pos.Z, m_destination.Z);
	int max_z = MYMAX(pos.Z, m_destination.Z);

	return (max_x - min_x) + (max_z - min_z);
}



/******************************************************************************/
bool Pathfinder::updateCostHeuristic(v3pos_t isource, v3pos_t idestination)
{
	// A* search algorithm.

	// The open list contains the pathfinder nodes that still need to be
	// checked. The priority queue sorts the pathfinder nodes by
	// estimated cost, with lowest cost on the top.
	std::priority_queue<v3pos_t, std::vector<v3pos_t>, PathfinderCompareHeuristic>
			openList(PathfinderCompareHeuristic(this));

	v3pos_t source = getRealPos(isource);
	v3pos_t destination = getRealPos(idestination);

	// initial position
	openList.push(source);

	// the 4 cardinal directions
	const static v3pos_t directions[4] = {
		v3pos_t(1,0, 0),
		v3pos_t(-1,0, 0),
		v3pos_t(0,0, 1),
		v3pos_t(0,0,-1)
	};

	v3pos_t current_pos;
	PathGridnode& s_pos = getIndexElement(isource);
	s_pos.source = true;
	s_pos.totalcost = 0;

	// estimated cost from start to finish
	int cur_manhattan = getXZManhattanDist(destination);
	s_pos.estimated_cost = cur_manhattan;

	while (!openList.empty()) {
		// Pick node with lowest total cost estimate.
		// The "cheapest" node is always on top.
		current_pos = openList.top();
		openList.pop();
		v3pos_t ipos = getIndexPos(current_pos);

		// check if node is inside searchdistance and valid
		if (!isValidIndex(ipos)) {
			DEBUG_OUT(LVL " Pathfinder: " << PP(current_pos) <<
				" out of search distance, max=" << PP(m_limits.MaxEdge) << std::endl);
			continue;
		}

		PathGridnode& g_pos = getIndexElement(ipos);
		g_pos.is_closed = true;
		g_pos.is_open = false;
		if (!g_pos.valid) {
			continue;
		}

		if (current_pos == destination) {
			// destination found, terminate
			g_pos.target = true;
			return true;
		}

		// for this node, check the 4 cardinal directions
		for (v3pos_t direction_flat : directions) {
			int current_totalcost = g_pos.totalcost;

			// get cost from current node to currently checked direction
			PathCost cost = g_pos.getCost(direction_flat);
			if (!cost.updated) {
				cost = calcCost(current_pos, direction_flat);
				g_pos.setCost(direction_flat, cost);
			}
			// update Y component of direction if neighbor requires jump or fall
			v3pos_t direction_3d = v3pos_t(direction_flat);
			direction_3d.Y = cost.y_change;

			// get position of true neighbor
			v3pos_t neighbor = current_pos + direction_3d;
			v3pos_t ineighbor = getIndexPos(neighbor);
			PathGridnode &n_pos = getIndexElement(ineighbor);

			if (cost.valid && !n_pos.is_closed && !n_pos.is_open) {
				// heuristic function; estimate cost from neighbor to destination
				cur_manhattan = getXZManhattanDist(neighbor);

				// add neighbor to open list
				n_pos.sourcedir = invert(direction_3d);
				n_pos.totalcost = current_totalcost + cost.value;
				n_pos.estimated_cost = current_totalcost + cost.value + cur_manhattan;
				n_pos.is_open = true;
				openList.push(neighbor);
			}
		}
	}
	// no path found; all possible nodes within searchdistance have been exhausted
	return false;
}

/******************************************************************************/
bool Pathfinder::buildPath(std::vector<v3pos_t> &path, v3pos_t ipos)
{
	// The cost calculation should have set a source direction for all relevant nodes.
	// To build the path, we go backwards from the destination until we reach the start.
	for(u32 waypoints = 1; waypoints++; ) {
		if (waypoints > PATHFINDER_MAX_WAYPOINTS) {
			ERROR_TARGET << "Pathfinder: buildPath: path is too long (too many waypoints), aborting" << std::endl;
			return false;
		}
		// Insert node into path
		PathGridnode &g_pos = getIndexElement(ipos);
		if (!g_pos.valid) {
			ERROR_TARGET << "Pathfinder: buildPath: invalid next pos detected, aborting" << std::endl;
			return false;
		}

		g_pos.is_element = true;
		path.push_back(ipos);
		if (g_pos.source)
			// start node found, terminate
			return true;

		// go to the node from which the pathfinder came
		ipos += g_pos.sourcedir;
	}

	ERROR_TARGET << "Pathfinder: buildPath: no source node found" << std::endl;
	return false;
}

/******************************************************************************/
v3pos_t Pathfinder::walkDownwards(v3pos_t pos, unsigned int max_down) {
	if (max_down == 0)
		return pos;
	v3pos_t testpos = v3pos_t(pos);
	MapNode node_at_pos = m_map->getNode(testpos);
	unsigned int down = 0;
	while ((node_at_pos.param0 != CONTENT_IGNORE) &&
			(!m_ndef->get(node_at_pos).walkable) &&
			(testpos.Y > m_limits.MinEdge.Y) &&
			(down <= max_down)) {
		testpos += v3pos_t(0, -1, 0);
		down++;
		node_at_pos = m_map->getNode(testpos);
	}
	//did we find surface?
	if ((testpos.Y >= m_limits.MinEdge.Y) &&
			(node_at_pos.param0 != CONTENT_IGNORE) &&
			(m_ndef->get(node_at_pos).walkable)) {
		if (down == 0) {
			pos = testpos;
		} else if ((down - 1) <= max_down) {
			//difference of y-pos +1 (target node is ABOVE solid node)
			testpos += v3pos_t(0, 1, 0);
			pos = testpos;
		}
		else {
			VERBOSE_TARGET << "Pos too far above ground: " <<
				"Index: " << PP(getIndexPos(pos)) <<
				"Realpos: " << PP(getRealPos(getIndexPos(pos))) << std::endl;
		}
	} else {
		DEBUG_OUT("Pathfinder: no surface found below pos" << std::endl);
	}
	return pos;
}

#ifdef PATHFINDER_DEBUG

/******************************************************************************/
void Pathfinder::printCost()
{
	printCost(DIR_XP);
	printCost(DIR_XM);
	printCost(DIR_ZP);
	printCost(DIR_ZM);
}

/******************************************************************************/
void Pathfinder::printYdir()
{
	printYdir(DIR_XP);
	printYdir(DIR_XM);
	printYdir(DIR_ZP);
	printYdir(DIR_ZM);
}

/******************************************************************************/
void Pathfinder::printCost(PathDirections dir)
{
	std::cout << "Cost in direction: " << dirToName(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				if (getIdxElem(x, y, z).directions[dir].valid)
					std::cout << std::setw(4)
						<< getIdxElem(x, y, z).directions[dir].value;
				else
					std::cout << std::setw(4) << "-";
				}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/******************************************************************************/
void Pathfinder::printYdir(PathDirections dir)
{
	std::cout << "Height difference in direction: " << dirToName(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				if (getIdxElem(x, y, z).directions[dir].valid)
					std::cout << std::setw(4)
						<< getIdxElem(x, y, z).directions[dir].y_change;
				else
					std::cout << std::setw(4) << "-";
				}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/******************************************************************************/
void Pathfinder::printType()
{
	std::cout << "Type of node:" << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(3) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(3) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(3) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				char toshow = getIdxElem(x, y, z).type;
				std::cout << std::setw(3) << toshow;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

/******************************************************************************/
void Pathfinder::printPathLen()
{
	std::cout << "Pathlen:" << std::endl;
		std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
		std::cout << std::setfill(' ');
		for (int y = 0; y < m_max_index_y; y++) {

			std::cout << "Level: " << y << std::endl;

			std::cout << std::setw(3) << " " << "  ";
			for (int x = 0; x < m_max_index_x; x++) {
				std::cout << std::setw(3) << x;
			}
			std::cout << std::endl;

			for (int z = 0; z < m_max_index_z; z++) {
				std::cout << std::setw(3) << z <<": ";
				for (int x = 0; x < m_max_index_x; x++) {
					std::cout << std::setw(3) << getIdxElem(x, y, z).totalcost;
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
}

/******************************************************************************/
std::string Pathfinder::dirToName(PathDirections dir)
{
	switch (dir) {
	case DIR_XP:
		return "XP";
		break;
	case DIR_XM:
		return "XM";
		break;
	case DIR_ZP:
		return "ZP";
		break;
	case DIR_ZM:
		return "ZM";
		break;
	default:
		return "UKN";
	}
}

/******************************************************************************/
void Pathfinder::printPath(const std::vector<v3pos_t> &path)
{
	unsigned int current = 0;
	for (std::vector<v3pos_t>::iterator i = path.begin();
			i != path.end(); ++i) {
		std::cout << std::setw(3) << current << ":" << PP((*i)) << std::endl;
		current++;
	}
}

#endif
