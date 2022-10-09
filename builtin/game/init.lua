
local scriptpath = core.get_builtin_path()
local commonpath = scriptpath .. "common" .. DIR_DELIM
local gamepath   = scriptpath .. "game".. DIR_DELIM

-- Shared between builtin files, but
-- not exposed to outer context
local builtin_shared = {}

dofile(gamepath .. "constants.lua")
dofile(gamepath .. "item_s.lua")
assert(loadfile(gamepath .. "item.lua"))(builtin_shared)
dofile(gamepath .. "register.lua")

if core.settings:get_bool("profiler.load") then
	profiler = dofile(scriptpath .. "profiler" .. DIR_DELIM .. "init.lua")
end

dofile(commonpath .. "after.lua")
dofile(commonpath .. "mod_storage.lua")
dofile(gamepath .. "item_entity.lua")
dofile(gamepath .. "deprecated.lua")
dofile(gamepath .. "misc_s.lua")
dofile(gamepath .. "misc.lua")
dofile(gamepath .. "privileges.lua")
if core.settings:get_bool("auth_kv") then
	dofile(gamepath .. "fm_auth.lua")
else
dofile(gamepath .. "auth.lua")
end
dofile(commonpath .. "chatcommands.lua")
dofile(gamepath .. "chat.lua")
dofile(commonpath .. "information_formspecs.lua")
-- internal is better
--dofile(gamepath .. "static_spawn.lua")
dofile(gamepath .. "detached_inventory.lua")
assert(loadfile(gamepath .. "falling.lua"))(builtin_shared)
dofile(gamepath .. "features.lua")
dofile(gamepath .. "voxelarea.lua")
dofile(gamepath .. "forceloading.lua")
dofile(gamepath .. "statbars.lua")
dofile(gamepath .. "knockback.lua")
dofile(gamepath .. "async.lua")

if core.setting_getbool("mod_debugging") then
	dofile(gamepath.."mod_debugging.lua")
end

profiler = nil
