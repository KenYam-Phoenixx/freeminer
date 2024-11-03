/*
nodemetadata.h
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

#include <unordered_set>
#include <map>
#include "metadata.h"

/*
	NodeMetadata stores arbitary amounts of data for special blocks.
	Used for furnaces, chests and signs.

	There are two interaction methods: inventory menu and text input.
	Only one can be used for a single metadata, thus only inventory OR
	text input should exist in a metadata.
*/

class Inventory;
class IItemDefManager;

class NodeMetadata : public SimpleMetadata
{
public:
	NodeMetadata(IItemDefManager *item_def_mgr);
	~NodeMetadata();

	void serialize(std::ostream &os, u8 version, bool disk=true) const;
	void deSerialize(std::istream &is, u8 version);

	void clear();
	bool empty() const;

	// The inventory
	Inventory *getInventory()
	{
		return m_inventory;
	}

	inline bool isPrivate(const std::string &name) const
	{
		return m_privatevars.count(name) != 0;
	}
	void markPrivate(const std::string &name, bool set);

private:
	int countNonPrivate() const;

	Inventory *m_inventory;
	std::unordered_set<std::string> m_privatevars;
};


/*
	List of metadata of all the nodes of a block
*/

typedef std::map<v3s16, NodeMetadata *> NodeMetadataMap;

class NodeMetadataList
{
public:
	NodeMetadataList(bool is_metadata_owner = true) :
		m_is_metadata_owner(is_metadata_owner)
	{}

	~NodeMetadataList();

	void serialize(std::ostream &os, u8 blockver, bool disk = true,
		bool absolute_pos = false, bool include_empty = false) const;
	void deSerialize(std::istream &is, IItemDefManager *item_def_mgr,
		bool absolute_pos = false);

	// Add all keys in this list to the vector keys
	std::vector<v3s16> getAllKeys();
	// Get pointer to data
	NodeMetadata *get(v3s16 p);
	// Deletes data
	void remove(v3s16 p);
	// Deletes old data and sets a new one
	void set(v3s16 p, NodeMetadata *d);
	// Deletes all
	void clear();

	size_t size() const { return m_data.size(); }

	NodeMetadataMap::const_iterator begin()
	{
		return m_data.begin();
	}

	NodeMetadataMap::const_iterator end()
	{
		return m_data.end();
	}

private:
	int countNonEmpty() const;

	bool m_is_metadata_owner;
	NodeMetadataMap m_data;
};
