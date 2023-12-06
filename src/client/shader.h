/*
shader.h
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>
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

#include "irrlichttypes_bloated.h"
#include <IMaterialRendererServices.h>
#include <string>
#include "tile.h"
#include "nodedef.h"

class IGameDef;

/*
	shader.{h,cpp}: Shader handling stuff.
*/

/*
	Gets the path to a shader by first checking if the file
	  name_of_shader/filename
	exists in shader_path and if not, using the data path.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getShaderPath(const std::string &name_of_shader,
		const std::string &filename);

struct ShaderInfo {
	std::string name = "";
	video::E_MATERIAL_TYPE base_material = video::EMT_SOLID;
	video::E_MATERIAL_TYPE material = video::EMT_SOLID;
	NodeDrawType drawtype = NDT_NORMAL;
	MaterialType material_type = TILE_MATERIAL_BASIC;

	ShaderInfo() = default;
	virtual ~ShaderInfo() = default;
};

/*
	Setter of constants for shaders
*/

namespace irr { namespace video {
	class IMaterialRendererServices;
} }


class IShaderConstantSetter {
public:
	virtual ~IShaderConstantSetter() = default;
	virtual void onSetConstants(video::IMaterialRendererServices *services) = 0;
	virtual void onSetMaterial(const video::SMaterial& material)
	{ }
};


class IShaderConstantSetterFactory {
public:
	virtual ~IShaderConstantSetterFactory() = default;
	virtual IShaderConstantSetter* create() = 0;
};


template <typename T, std::size_t count, bool cache>
class CachedShaderSetting {
	const char *m_name;
	T m_sent[count];
	bool has_been_set = false;
	bool is_pixel;
protected:
	CachedShaderSetting(const char *name, bool is_pixel) :
		m_name(name), is_pixel(is_pixel)
	{}
public:
	void set(const T value[count], video::IMaterialRendererServices *services)
	{
		if (cache && has_been_set && std::equal(m_sent, m_sent + count, value))
			return;
		if (is_pixel)
			services->setPixelShaderConstant(services->getPixelShaderConstantID(m_name), value, count);
		else
			services->setVertexShaderConstant(services->getVertexShaderConstantID(m_name), value, count);

		if (cache) {
			std::copy(value, value + count, m_sent);
			has_been_set = true;
		}
	}
};

template <typename T, std::size_t count = 1, bool cache=true>
class CachedPixelShaderSetting : public CachedShaderSetting<T, count, cache> {
public:
	CachedPixelShaderSetting(const char *name) :
		CachedShaderSetting<T, count, cache>(name, true){}
};

template <typename T, std::size_t count = 1, bool cache=true>
class CachedVertexShaderSetting : public CachedShaderSetting<T, count, cache> {
public:
	CachedVertexShaderSetting(const char *name) :
		CachedShaderSetting<T, count, cache>(name, false){}
};

template <typename T, std::size_t count, bool cache, bool is_pixel>
class CachedStructShaderSetting {
	const char *m_name;
	T m_sent[count];
	bool has_been_set = false;
	std::array<const char*, count> m_fields;
public:
	CachedStructShaderSetting(const char *name, std::array<const char*, count> &&fields) :
		m_name(name), m_fields(std::move(fields))
	{}

	void set(const T value[count], video::IMaterialRendererServices *services)
	{
		if (cache && has_been_set && std::equal(m_sent, m_sent + count, value))
			return;

		for (std::size_t i = 0; i < count; i++) {
			std::string uniform_name = std::string(m_name) + "." + m_fields[i];

			if (is_pixel)
				services->setPixelShaderConstant(services->getPixelShaderConstantID(uniform_name.c_str()), value + i, 1);
			else
				services->setVertexShaderConstant(services->getVertexShaderConstantID(uniform_name.c_str()), value + i, 1);
		}

		if (cache) {
			std::copy(value, value + count, m_sent);
			has_been_set = true;
		}
	}
};

template<typename T, std::size_t count, bool cache = true>
using CachedStructVertexShaderSetting = CachedStructShaderSetting<T, count, cache, false>;

template<typename T, std::size_t count, bool cache = true>
using CachedStructPixelShaderSetting = CachedStructShaderSetting<T, count, cache, true>;

/*
	ShaderSource creates and caches shaders.
*/

class IShaderSource {
public:
	IShaderSource() = default;
	virtual ~IShaderSource() = default;

	virtual u32 getShaderIdDirect(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype = NDT_NORMAL){return 0;}
	virtual ShaderInfo getShaderInfo(u32 id){return ShaderInfo();}
	virtual u32 getShader(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype = NDT_NORMAL){return 0;}
};

class IWritableShaderSource : public IShaderSource {
public:
	IWritableShaderSource() = default;
	virtual ~IWritableShaderSource() = default;

	virtual void processQueue()=0;
	virtual void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program)=0;
	virtual void rebuildShaders()=0;

	/// @note Takes ownership of @p setter.
	virtual void addShaderConstantSetterFactory(IShaderConstantSetterFactory *setter) = 0;
};

IWritableShaderSource *createShaderSource();

void dumpShaderProgram(std::ostream &output_stream,
	const std::string &program_type, const std::string &program);
