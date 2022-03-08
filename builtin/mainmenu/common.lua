--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

-- Global menu data
menudata = {}

-- Local cached values
local min_supp_proto, max_supp_proto

function common_update_cached_supp_proto()
	min_supp_proto = core.get_min_supp_proto()
	max_supp_proto = core.get_max_supp_proto()
end
common_update_cached_supp_proto()

-- Menu helper functions

local function render_client_count(n)
<<<<<<< HEAD
	n = n + 0
	if     n > 99 then return '99+'
	elseif n >= 0 then return tostring(n)
=======
	if     n > 999 then return '99+'
	elseif n >= 0  then return tostring(n)
>>>>>>> 5.5.0
	else return '?' end
end

local function configure_selected_world_params(idx)
	local worldconfig = pkgmgr.get_worldconfig(menudata.worldlist:get_list()[idx].path)
	if worldconfig.creative_mode then
		core.settings:set("creative_mode", worldconfig.creative_mode)
	end
	if worldconfig.enable_damage then
		core.settings:set("enable_damage", worldconfig.enable_damage)
	end
end

<<<<<<< HEAD
--------------------------------------------------------------------------------
function image_column(tooltip, flagname)
	return "image,tooltip=" .. core.formspec_escape(tooltip) .. "," ..
		"0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") .. "," ..
		"1=" .. core.formspec_escape(defaulttexturedir .. "server_flags_" .. flagname .. ".png")
end

--------------------------------------------------------------------------------
function order_favorite_list(list)
	local res = {}
	if not list then list = {} end
	--orders the favorite list after support
	for i = 1, #list do
		local fav = list[i]
		if is_server_protocol_compat(fav.proto_min, fav.proto_max, fav.proto) then
			res[#res + 1] = fav
		end
	end
	for i = 1, #list do
		local fav = list[i]
		if not is_server_protocol_compat(fav.proto_min, fav.proto_max, fav.proto) then
			res[#res + 1] = fav
		end
	end
	return res
end

--------------------------------------------------------------------------------
function render_favorite(spec, is_favorite)
=======
function render_serverlist_row(spec)
>>>>>>> 5.5.0
	local text = ""
	if not spec then return "" end
	if spec.name and spec.name ~= "" then
		text = text .. core.formspec_escape(spec.name:trim())
	elseif spec.address then
<<<<<<< HEAD
		text = text .. spec.address:trim()
		if spec.port and tostring(spec.port) ~= "30000" then
=======
		text = text .. core.formspec_escape(spec.address:trim())
		if spec.port then
>>>>>>> 5.5.0
			text = text .. ":" .. spec.port
		end
	end

<<<<<<< HEAD
	local details = ""
	local grey_out = not is_server_protocol_compat(spec.proto_min, spec.proto_max, spec.proto)
=======
	local grey_out = not spec.is_compatible
>>>>>>> 5.5.0

	local details = {}

	if spec.lag or spec.ping then
		local lag = (spec.lag or 0) * 1000 + (spec.ping or 0) * 250
		if lag <= 125 then
			table.insert(details, "1")
		elseif lag <= 175 then
			table.insert(details, "2")
		elseif lag <= 250 then
			table.insert(details, "3")
		else
			table.insert(details, "4")
		end
	else
		table.insert(details, "0")
	end

	table.insert(details, ",")

	local color = (grey_out and "#aaaaaa") or ((spec.is_favorite and "#ddddaa") or "#ffffff")
	if spec.clients and (spec.clients_max or 0) > 0 then
		local clients_percent = 100 * spec.clients / spec.clients_max

		-- Choose a color depending on how many clients are connected
		-- (relatively to clients_max)
		local clients_color
		if     grey_out		      then clients_color = '#aaaaaa'
		elseif spec.clients == 0      then clients_color = ''        -- 0 players: default/white
		elseif clients_percent <= 60  then clients_color = '#a1e587' -- 0-60%: green
		elseif clients_percent <= 90  then clients_color = '#ffdc97' -- 60-90%: yellow
		elseif clients_percent == 100 then clients_color = '#dd5b5b' -- full server: red (darker)
		else                               clients_color = '#ffba97' -- 90-100%: orange
		end

		table.insert(details, clients_color)
		table.insert(details, render_client_count(spec.clients) .. " / " ..
			render_client_count(spec.clients_max))
	else
		table.insert(details, color)
		table.insert(details, "?")
	end

	if spec.creative then
		table.insert(details, "1") -- creative icon
	else
		table.insert(details, "0")
	end

	if spec.pvp then
		table.insert(details, "2") -- pvp icon
	elseif spec.damage then
		table.insert(details, "1") -- heart icon
	else
		table.insert(details, "0")
	end

	table.insert(details, color)
	table.insert(details, text)

	return table.concat(details, ",")
end
<<<<<<< HEAD

--------------------------------------------------------------------------------
os.tempfolder = function()
	if core.settings:get("TMPFolder") then
		return core.settings:get("TMPFolder") .. DIR_DELIM .. "MT_" .. math.random(0,10000)
	end

	local filetocheck = os.tmpname()
	os.remove(filetocheck)

	local randname = "MTTempModFolder_" .. math.random(0,10000)
	if DIR_DELIM == "\\" then
		local tempfolder = os.getenv("TEMP")
		return tempfolder .. filetocheck
	else
		local backstring = filetocheck:reverse()
		return filetocheck:sub(0,filetocheck:len()-backstring:find(DIR_DELIM)+1) ..randname
	end

=======
---------------------------------------------------------------------------------
os.tmpname = function()
	error('do not use') -- instead use core.get_temp_path()
>>>>>>> 5.5.0
end
--------------------------------------------------------------------------------

function menu_render_worldlist(show_gameid)
	local retval = {}
	local current_worldlist = menudata.worldlist:get_list()

	local row
	for i, v in ipairs(current_worldlist) do
		row = v.name
		if show_gameid == nil or show_gameid == true then
			row = row .. " [" .. v.gameid .. "]"
		end
		retval[#retval+1] = core.formspec_escape(row)

	end

	return table.concat(retval, ",")
end

function menu_handle_key_up_down(fields, textlist, settingname)
	local oldidx, newidx = core.get_textlist_index(textlist), 1
	if fields.key_up or fields.key_down then
		if fields.key_up and oldidx and oldidx > 1 then
			newidx = oldidx - 1
		elseif fields.key_down and oldidx and
				oldidx < menudata.worldlist:size() then
			newidx = oldidx + 1
		end
		core.settings:set(settingname, menudata.worldlist:get_raw_index(newidx))
		configure_selected_world_params(newidx)
		return true
	end
	return false
end

<<<<<<< HEAD
--------------------------------------------------------------------------------
function asyncOnlineFavourites()
	if not menudata.public_known then
		local file = io.open( core.setting_get("serverlist_cache"), "r" )
		if file then
			local data = file:read("*all")
			menudata.public_known = core.parse_json( data )
			file:close()
		end
	end

	if not menudata.public_known then
	menudata.public_known = {{
			name = fgettext("Loading..."),
			description = fgettext_ne("Try reenabling public serverlist and check your internet connection.")
		}}
	end
	menudata.favorites = menudata.public_known
	menudata.favorites_is_public = true

	if not menudata.public_downloading then
		menudata.public_downloading = true
	else
		return
	end

	core.handle_async(
		function(param)
			return core.get_favorites("online")
		end,
		nil,
		function(result)
			menudata.public_downloading = nil
			local favs = order_favorite_list(result)
			if favs[1] then
				menudata.public_known = favs
				menudata.favorites = menudata.public_known
				menudata.favorites_is_public = true

					local file = io.open( core.setting_get("serverlist_cache"), "w" )
					if file then
						file:write( core.write_json( favs ) )
						file:close()
					end

			end
			core.event_handler("Refresh")
		end
	)
end

function updater_init()
	local updater_req =  function(param) return core.get_favorites("sleep_cache") end
	local updater_res;
	updater_res = function(result)
			if core.setting_getbool("public_serverlist") and result[1] then
				local favs = order_favorite_list(result)
					menudata.public_known = favs
					menudata.favorites = menudata.public_known
				core.event_handler("Refresh")
			end
	core.handle_async(updater_req, nil, updater_res)
	end
	core.handle_async(updater_req, nil, updater_res)
end

--------------------------------------------------------------------------------
=======
>>>>>>> 5.5.0
function text2textlist(xpos, ypos, width, height, tl_name, textlen, text, transparency)
	local textlines = core.wrap_text(text, textlen, true)
	local retval = "textlist[" .. xpos .. "," .. ypos .. ";" .. width ..
			"," .. height .. ";" .. tl_name .. ";"

	for i = 1, #textlines do
		textlines[i] = textlines[i]:gsub("\r", "")
		retval = retval .. core.formspec_escape(textlines[i]) .. ","
	end

	retval = retval .. ";0;"
	if transparency then retval = retval .. "true" end
	retval = retval .. "]"

	return retval
end

<<<<<<< HEAD
--------------------------------------------------------------------------------
function is_server_protocol_compat(server_proto_min, server_proto_max, proto)
	if proto and core.setting_get("server_proto") ~= proto then return false end
=======
function is_server_protocol_compat(server_proto_min, server_proto_max)
>>>>>>> 5.5.0
	if (not server_proto_min) or (not server_proto_max) then
		-- There is no info. Assume the best and act as if we would be compatible.
		return true
	end
	return tonumber(min_supp_proto) <= tonumber(server_proto_max) and tonumber(max_supp_proto) >= tonumber(server_proto_min)
end
<<<<<<< HEAD
--------------------------------------------------------------------------------
function is_server_protocol_compat_or_error(server_proto_min, server_proto_max, proto)
	if not is_server_protocol_compat(server_proto_min, server_proto_max, proto) then
=======

function is_server_protocol_compat_or_error(server_proto_min, server_proto_max)
	if not is_server_protocol_compat(server_proto_min, server_proto_max) then
>>>>>>> 5.5.0
		local server_prot_ver_info, client_prot_ver_info

		if proto and core.setting_get("server_proto") ~= proto then 
			server_prot_ver_info = fgettext_ne("Server supports protocol $1, but we can connect only to $2 ",
				proto or '?', core.setting_get("server_proto") )
		end

		local s_p_min = server_proto_min
		local s_p_max = server_proto_max

		if s_p_min ~= s_p_max then
			server_prot_ver_info = fgettext_ne("Server supports protocol versions between $1 and $2. ",
				s_p_min, s_p_max)
		else
			server_prot_ver_info = fgettext_ne("Server enforces protocol version $1. ",
				s_p_min)
		end
		if min_supp_proto ~= max_supp_proto then
			client_prot_ver_info= fgettext_ne("We support protocol versions between version $1 and $2.",
				min_supp_proto, max_supp_proto)
		else
			client_prot_ver_info = fgettext_ne("We only support protocol version $1.", min_supp_proto)
		end
		gamedata.errormessage = fgettext_ne("Protocol version mismatch. ")
			.. server_prot_ver_info
			.. client_prot_ver_info
		return false
	end

	return true
end

function menu_worldmt(selected, setting, value)
	local world = menudata.worldlist:get_list()[selected]
	if world then
		local filename = world.path .. DIR_DELIM .. "world.mt"
		local world_conf = Settings(filename)

		if value then
			if not world_conf:write() then
				core.log("error", "Failed to write world config file")
			end
			world_conf:set(setting, value)
			world_conf:write()
		else
			return world_conf:get(setting)
		end
	else
		return nil
	end
end

function menu_worldmt_legacy(selected)
	local modes_names = {"creative_mode", "enable_damage", "server_announce"}
	for _, mode_name in pairs(modes_names) do
		local mode_val = menu_worldmt(selected, mode_name)
		if mode_val then
			core.settings:set(mode_name, mode_val)
		else
			menu_worldmt(selected, mode_name, core.settings:get(mode_name))
		end
	end
end
