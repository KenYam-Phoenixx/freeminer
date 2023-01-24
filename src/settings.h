/*
settings.h
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

#include "irr_v3d.h"
#include "irrlichttypes_bloated.h"
#include "util/string.h"
#include "util/basic_macros.h"
#include <memory>
#include <string>
#include <list>
#include <set>
#include <mutex>

#include "porting.h"
#include "json/json.h" // for json config values
#include "msgpack_fix.h"
#include <stdint.h>

class Settings;
using SettingsPtr = std::shared_ptr<Settings>;
struct NoiseParams;

// Global objects
extern Settings *g_settings; // Same as Settings::getLayer(SL_GLOBAL);
extern std::string g_settings_path;

// Type for a settings changed callback function
typedef void (*SettingsChangedCallback)(const std::string &name, void *data);

typedef std::vector<
	std::pair<
		SettingsChangedCallback,
		void *
	>
> SettingsCallbackList;

typedef std::unordered_map<std::string, SettingsCallbackList> SettingsCallbackMap;

enum ValueType {
	VALUETYPE_STRING,
	VALUETYPE_FLAG // Doesn't take any arguments
};

enum SettingsParseEvent {
	SPE_NONE,
	SPE_INVALID,
	SPE_COMMENT,
	SPE_KVPAIR,
	SPE_END,
	SPE_GROUP,
	SPE_MULTILINE,
};

// Describes the global setting layers, SL_GLOBAL is where settings are read from
enum SettingsLayer {
	SL_DEFAULTS,
	SL_GAME,
	SL_GLOBAL,
	SL_TOTAL_COUNT
};

// Implements the hierarchy a settings object may be part of
class SettingsHierarchy {
public:
	/*
	 * A settings object that may be part of another hierarchy can
	 * occupy the index 0 as a fallback. If not set you can use 0 on your own.
	 */
	SettingsHierarchy(Settings *fallback = nullptr);

	DISABLE_CLASS_COPY(SettingsHierarchy)

	Settings *getLayer(int layer) const;

private:
	friend class Settings;
	Settings *getParent(int layer) const;
	void onLayerCreated(int layer, Settings *obj);
	void onLayerRemoved(int layer);

	std::vector<Settings*> layers;
};

struct ValueSpec {
	ValueSpec(ValueType a_type, const char *a_help=NULL)
	{
		type = a_type;
		help = a_help;
	}

	ValueType type;
	const char *help;
};

struct SettingsEntry {
	SettingsEntry() = default;

	SettingsEntry(const std::string &value_) :
		value(value_)
	{}

	SettingsEntry(const SettingsPtr & group_) :
		group(group_),
		is_group(true)
	{}

	std::string value = "";
	SettingsPtr group = nullptr;
	bool is_group = false;
};

typedef std::unordered_map<std::string, SettingsEntry> SettingEntries;

class Settings {
public:
	/* These functions operate on the global hierarchy! */
	static Settings *createLayer(SettingsLayer sl, const std::string &end_tag = "");
	static Settings *getLayer(SettingsLayer sl);
	/**/

	Settings(const std::string &end_tag = "") :
		m_end_tag(end_tag)
	{}
	Settings(const std::string &end_tag, SettingsHierarchy *h, int settings_layer);
	~Settings();

	//Settings & operator += (const Settings &other);
	Settings & operator = (const Settings &other);

	/***********************
	 * Reading and writing *
	 ***********************/

	// Read configuration file.  Returns success.
	bool readConfigFile(const std::string &filename);
	//Updates configuration file.  Returns success.
	bool updateConfigFile(const std::string &filename);
	// NOTE: Types of allowed_options are ignored.  Returns success.
	bool parseCommandLine(int argc, char *argv[],
			std::map<std::string, ValueSpec> &allowed_options);
	bool parseConfigLines(std::istream &is);
	void writeLines(std::ostream &os, u32 tab_depth=0) const;

	/***********
	 * Getters *
	 ***********/

	SettingsPtr getGroup(const std::string &name) const;
	const std::string &get(const std::string &name) const;
	bool getBool(const std::string &name) const;
	u16 getU16(const std::string &name) const;
	s16 getS16(const std::string &name) const;
	u32 getU32(const std::string &name) const;
	s32 getS32(const std::string &name) const;
	u64 getU64(const std::string &name) const;
	pos_t getPos(const std::string &name) const;
	float getFloat(const std::string &name) const;
	float getFloat(const std::string &name, float min, float max) const;
	v2f getV2F(const std::string &name) const;
	v3f getV3F(const std::string &name) const;
#if USE_OPOS64
	v3opos_t getV3O(const std::string &name) const;
#endif
	u32 getFlagStr(const std::string &name, const FlagDesc *flagdesc,
			u32 *flagmask) const;
	bool getNoiseParams(const std::string &name, NoiseParams &np) const;
	bool getNoiseParamsFromValue(const std::string &name, NoiseParams &np) const;
	bool getNoiseParamsFromGroup(const std::string &name, NoiseParams &np) const;

	// return all keys used in this object
	std::vector<std::string> getNames() const;
	// check if setting exists anywhere in the hierarchy
	bool exists(const std::string &name) const;
	// check if setting exists in this object ("locally")
	bool existsLocal(const std::string &name) const;


	/***************************************
	 * Getters that don't throw exceptions *
	 ***************************************/

	bool getGroupNoEx(const std::string &name, SettingsPtr &val) const;
	bool getNoEx(const std::string &name, std::string &val) const;
	bool getFlag(const std::string &name) const;
	bool getBoolNoEx(const std::string &name, bool &val) const;
	bool getU16NoEx(const std::string &name, u16 &val) const;
	bool getS16NoEx(const std::string &name, s16 &val) const;
	bool getU32NoEx(const std::string &name, u32 &val) const;
	bool getS32NoEx(const std::string &name, s32 &val) const;
	bool getPosNoEx(const std::string &name, pos_t &val) const;
	bool getU64NoEx(const std::string &name, u64 &val) const;
	bool getFloatNoEx(const std::string &name, float &val) const;
	bool getV2FNoEx(const std::string &name, v2f &val) const;
	bool getV3FNoEx(const std::string &name, v3f &val) const;
#if USE_OPOS64
	bool getV3FNoEx(const std::string &name, v3opos_t &val) const;
#endif

	// Like other getters, but handling each flag individualy:
	// 1) Read default flags (or 0)
	// 2) Override using user-defined flags
	bool getFlagStrNoEx(const std::string &name, u32 &val,
		const FlagDesc *flagdesc) const;


	/***********
	 * Setters *
	 ***********/

	// N.B. Groups not allocated with new must be set to NULL in the settings
	// tree before object destruction.
/*
	bool setEntry(const std::string &name, const void *entry,
		bool set_group);
*/	
	bool set(const std::string &name, const std::string &value);
	bool setDefault(const std::string &name, const std::string &value);
	bool setGroup(const std::string &name, const Settings &group);
	bool setBool(const std::string &name, bool value);
	bool setS16(const std::string &name, s16 value);
	bool setU16(const std::string &name, u16 value);
	bool setS32(const std::string &name, s32 value);
	bool setU64(const std::string &name, u64 value);
	bool setPos(const std::string &name, pos_t value);
	bool setFloat(const std::string &name, float value);
	bool setV2F(const std::string &name, v2f value);
	bool setV3F(const std::string &name, v3f value);
	bool setFlagStr(const std::string &name, u32 flags,
		const FlagDesc *flagdesc = nullptr, u32 flagmask = U32_MAX);
	bool setNoiseParams(const std::string &name, const NoiseParams &np);

	// remove a setting
	bool remove(const std::string &name);

	/*****************
	 * Miscellaneous *
	 *****************/

	void setDefault(const std::string &name, const FlagDesc *flagdesc, u32 flags);
	const FlagDesc *getFlagDescFallback(const std::string &name) const;

	//freeminer:
	bool setEntry(const std::string &name, const std::string &entry, bool set_group);
	bool setEntry(const std::string &name, const SettingsPtr & entry, bool set_group);
	Json::Value getJson(const std::string & name, const Json::Value & def = Json::Value()) const;
	void setJson(const std::string & name, const Json::Value & value);

	Json::Value m_json;
	bool toJson(Json::Value &json) const;
	bool fromJson(const Json::Value &json);
	bool writeJsonFile(const std::string &filename);
	bool readJsonFile(const std::string &filename);
	void msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const;
	void msgpack_unpack(msgpack::object o);
	friend std::ostream & operator<<(std::ostream & os, Settings & settings) {
		Json::Value json;
		settings.toJson(json);
		os << json;
		return os;
	}
	//=========

	void registerChangedCallback(const std::string &name,
		SettingsChangedCallback cbf, void *userdata = NULL);
	void deregisterChangedCallback(const std::string &name,
		SettingsChangedCallback cbf, void *userdata = NULL);

	void removeSecureSettings();

	// Returns the settings layer this object is.
	// If within the global hierarchy you can cast this to enum SettingsLayer
	inline int getLayer() const { return m_settingslayer; }

private:
	/***********************
	 * Reading and writing *
	 ***********************/

	SettingsParseEvent parseConfigObject(const std::string &line,
		std::string &name, std::string &value);
	bool updateConfigObject(std::istream &is, std::ostream &os,
		u32 tab_depth=0);

	static bool checkNameValid(const std::string &name);
	static bool checkValueValid(const std::string &value);
	static std::string getMultiline(std::istream &is, size_t *num_lines=NULL);
	static void printEntry(std::ostream &os, const std::string &name,
		const SettingsEntry &entry, u32 tab_depth=0);

	/***********
	 * Getters *
	 ***********/
	Settings *getParent() const;

	const SettingsEntry &getEntry(const std::string &name) const;

	// Allow TestSettings to run sanity checks using private functions.
	friend class TestSettings;
	// For sane mutex locking when iterating
	friend class LuaSettings;

	void updateNoLock(const Settings &other);
	void clearNoLock();
	void clearDefaultsNoLock();

	void doCallbacks(const std::string &name) const;

	SettingEntries m_settings;
	SettingsCallbackMap m_callbacks;
	std::string m_end_tag;

	mutable std::mutex m_callback_mutex;

	// All methods that access m_settings/m_defaults directly should lock this.
	mutable std::mutex m_mutex;

	SettingsHierarchy *m_hierarchy = nullptr;
	int m_settingslayer = -1;

	static std::unordered_map<std::string, const FlagDesc *> s_flags;
};

extern Settings *g_settings;
extern std::string g_settings_path;
