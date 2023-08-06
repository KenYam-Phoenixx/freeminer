/*
defaultsettings.cpp
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
*/

/*
This file is part of Freeminer.

Freeminer is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "network/fm_connection_use.h"
#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "config.h"
#include "constants.h"
#include "porting.h"
#include "mapgen/mapgen.h" // Mapgen::setDefaultSettings
#include "util/string.h"


// freeminer part:
#include "network/connection.h" // ENET_IPV6
#ifndef SERVER // Only on client
#include "hud.h"
#endif


const bool debug =
#ifdef NDEBUG
    false
#else
    true
#endif
    ;

const bool win32 =
#if defined(_WIN32) && !defined(_WIN64)
    true
#else
    false
#endif
    ;

const bool win64 =
#if defined(_WIN64)
    true
#else
    false
#endif
    ;

const bool win = win32 || win64;


const bool android =
#if defined(__ANDROID__)
    true
#else
    false
#endif
    ;

const bool arm =
#if defined(__ARM_ARCH)
    true
#else
    false
#endif
    ;

const bool threads =
#if ENABLE_THREADS
    true
#else
    false
#endif
    ;


void fm_set_default_settings(Settings *settings) {

	// Screen
#if __ANDROID__ || __ARM_ARCH
	//settings->setDefault("enable_shaders", "0");
#endif
//	settings->setDefault("chat_buffer_size", "6"); // todo re-enable
	settings->setDefault("timelapse", "0");

	// Paths
	settings->setDefault("screenshot_path", "screenshots"); // "."
	settings->setDefault("serverlist_file", "favoriteservers.json"); // "favoriteservers.txt"
	settings->setDefault("serverlist_cache", porting::path_user + DIR_DELIM + "client" + DIR_DELIM + "servers_public.json");
	settings->setDefault("serverlist_lan", "1");

	// Main menu
	settings->setDefault("main_menu_tab", "multiplayer");
	settings->setDefault("public_serverlist", "1");
	settings->setDefault("password_save", "1");
	settings->setDefault("enable_split_login_register", "false");

	// Game Speed
	settings->setDefault("pause_fps_max", "10"); // "20"
	settings->setDefault("static_viewing_range", "false");

	// Debugging stuff
	settings->setDefault("show_debug", debug ? "true" : "false"); // "true"
	settings->setDefault("deprecated_lua_api_handling", debug ? "log" : "legacy"); // "log"
	settings->setDefault("profiler_print_interval", debug ? "10" : "0"); // "0"
	settings->setDefault("time_taker_enabled", debug ? "5" : "0");

	// Keymaps
	settings->setDefault("keymap_zoom", "KEY_KEY_Z");
	settings->setDefault("keymap_msg", "@");
	settings->setDefault("keymap_toggle_update_camera", debug ? "KEY_F4" : "");
	settings->setDefault("keymap_toggle_block_boundaries", "KEY_F4");
	settings->setDefault("keymap_playerlist", "KEY_TAB");
#if IRRLICHT_VERSION_10000  >= 10703
	settings->setDefault("keymap_console", "KEY_OEM_3");
#else
	settings->setDefault("keymap_console", "KEY_F10");
#endif


	// Fonts
	settings->setDefault("freetype", "true"); // "false"
	settings->setDefault("font_path", porting::getDataPath("fonts" DIR_DELIM "liberationsans.ttf")); // porting::getDataPath("fonts" DIR_DELIM "lucida_sans")
	settings->setDefault("mono_font_path", porting::getDataPath("fonts" DIR_DELIM "liberationmono.ttf")); // porting::getDataPath("fonts" DIR_DELIM "mono_dejavu_sans")

	settings->setDefault("reconnects", win ? "1" : "10"); // TODO: wix windows

	// Map generation
	settings->setDefault("mg_name", "indev"); // "v6"
	//settings->setDefault("mg_flags", "trees, caves, dungeons"); // "dungeons"
	//settings->setDefault("mgv6_spflags", "jungles, biome_blend, snowbiomes"); // "jungles, snowbiomes"
	settings->setDefault("mg_math", ""); // configuration in json struct
	settings->setDefault("mg_params", ""); // configuration in json struct
	settings->setDefault("static_spawnpoint_find", "0");

	// Filters
	settings->setDefault("anisotropic_filter", "true"); // "false"

	// Waving
	settings->setDefault("enable_waving_leaves", "true"); // "false"
	settings->setDefault("enable_waving_plants", "true"); // "false"
	settings->setDefault("enable_waving_water", "true"); // "false"

	// Shaders
	//settings->setDefault("enable_bumpmapping", "true"); // "false"
	settings->setDefault("enable_parallax_occlusion", "true"); // "false"
	settings->setDefault("disable_wieldlight", "false");
	settings->setDefault("mip_map", "true"); // "false"
	// Clouds, water, glass, leaves, fog
	settings->setDefault("cloud_height", "300"); // "120"
	settings->setDefault("enable_zoom_cinematic", "true");
	settings->setDefault("wanted_fps", android ? "25" : "30");
	settings->setDefault("viewing_range_max", (win32 || android) ? "300" : "10000" /*itos(MAX_MAP_GENERATION_LIMIT)*/); // "240"
	settings->setDefault("shadows", "0");
	settings->setDefault("farmesh", android ? "2" : "4");
	settings->setDefault("farmesh_step", android ? "2" : "4");
	settings->setDefault("farmesh_wanted", android ? "100" :"500");
	settings->setDefault("headless_optimize", "false");
	//settings->setDefault("node_highlighting", "halo");
	//settings->setDefault("enable_vbo", win ? "false" : "true");
	settings->setDefault("light_ambient", "false");
	settings->setDefault("enable_dynamic_shadows", "1");

	// Liquid
	settings->setDefault("liquid_real", "true");
	settings->setDefault("liquid_send", android ? "3.0" : "1.0");
	settings->setDefault("liquid_relax", android ? "1" : "2");
	settings->setDefault("liquid_fast_flood", "-200");

	// Weather
	settings->setDefault("weather", threads ? "true" : "false");
	settings->setDefault("weather_biome", "false");
	settings->setDefault("weather_heat_season", "20");
	settings->setDefault("weather_heat_daily", "8");
	settings->setDefault("weather_heat_width", "3000");
	settings->setDefault("weather_hot_core", "1000");
	settings->setDefault("weather_heat_height", "-333");
	settings->setDefault("year_days", "30");
	settings->setDefault("weather_humidity_season", "30");
	settings->setDefault("weather_humidity_daily", "-12");
	settings->setDefault("weather_humidity_width", "300");
	settings->setDefault("weather_humidity_height", "-250");
	settings->setDefault("weather_humidity_days", "2");

	settings->setDefault("respawn_auto", "false");
	settings->setDefault("autojump", android ? "1" : "0");
	settings->setDefault("hotbar_cycling", "false");

// TODO: refactor and resolve client/server dependencies
#ifndef SERVER // Only on client
	settings->setDefault("minimap_default_mode", itos(MINIMAP_TYPE_SURFACE));
#endif

#if !MINETEST_PROTO || !MINETEST_TRANSPORT
	settings->setDefault("serverlist_url", "servers.freeminer.org");
//#elif USE_SCTP
//	settings->setDefault("serverlist_url", "servers2.freeminer.org");
#endif
	settings->setDefault("server_proto", server_proto);
	settings->setDefault("remote_proto", "");
	settings->setDefault("timeout_mul", android ? "5" : "1");
	settings->setDefault("default_game", "default"); // "minetest"
	settings->setDefault("max_users", "100"); // "15"
	settings->setDefault("enable_any_name", "0"); // WARNING! SETTING TO "1" COULD CAUSE SECURITY RISKS WITH MODULES WITH PLAYER DATA IN FILES CONTAINS PLAYER NAME IN FILENAME
	settings->setDefault("default_privs_creative", "interact, shout, fly, fast");
	settings->setDefault("vertical_spawn_range", "50"); // "16"
	settings->setDefault("cache_block_before_spawn", "true");
	settings->setDefault("abm_random", (android || win) ? "false" : "true");
	settings->setDefault("active_block_range", android ? "1" : threads ? "4" : "2");
	settings->setDefault("abm_neighbors_range_max", (threads && !win32 && !android) ? "16" : "1");
	settings->setDefault("enable_force_load", "true");
#if !MINETEST_PROTO
	settings->setDefault("max_simultaneous_block_sends_per_client", "50"); // "10"
#endif
	settings->setDefault("max_block_send_distance", "30"); // "9"
	settings->setDefault("server_unload_unused_data_timeout", "65"); // "29"
	settings->setDefault("max_objects_per_block", "100"); // "49"
	settings->setDefault("server_occlusion", "true");
	settings->setDefault("ignore_world_load_errors", "true"); // "false"
	settings->setDefault("emergequeue_limit_diskonly", ""); // autodetect from number of cpus
	settings->setDefault("emergequeue_limit_generate", ""); // autodetect from number of cpus
	settings->setDefault("emergequeue_limit_total", ""); // autodetect from number of cpus
	settings->setDefault("num_emerge_threads", ""); // "1"
	settings->setDefault("server_map_save_interval", "300"); // "5.3"
	settings->setDefault("sqlite_synchronous", "1"); // "2"
	settings->setDefault("save_generated_block", "true");
	settings->setDefault("save_changed_block", "true");
	settings->setDefault("block_delete_time", threads && arm ? "60" : threads ? "30" : "10");

#if (ENET_IPV6 || MINETEST_TRANSPORT || USE_SCTP)
	//settings->setDefault("enable_ipv6", "true");
#else
	settings->setDefault("enable_ipv6", "false");
#endif

#if !USE_IPV4_DEFAULT && (ENET_IPV6 || MINETEST_TRANSPORT || USE_SCTP)
	settings->setDefault("ipv6_server", "true"); // problems on all windows versions (unable to play in local game)
#else
	//settings->setDefault("ipv6_server", "false");
#endif

#if !MINETEST_PROTO
	settings->setDefault("send_pre_v25_init", "1");
#endif

	settings->setDefault("movement_fov", "true");
	settings->setDefault("movement_acceleration_default", "4"); // "3"
	settings->setDefault("movement_acceleration_air", "4"); // "2"
	settings->setDefault("movement_speed_walk", "6"); // "4"
	settings->setDefault("movement_speed_crouch", "2"); // "1.35"
	settings->setDefault("movement_speed_fast", "20.5"); // "20"
	//settings->setDefault("movement_fall_aerodynamics", "110");

/*
	settings->setDefault("animation_default_start", "0");
	settings->setDefault("animation_default_stop", "79");
	settings->setDefault("animation_walk_start", "168");
	settings->setDefault("animation_walk_stop", "187");
	settings->setDefault("animation_dig_start", "189");
	settings->setDefault("animation_dig_stop", "198");
	settings->setDefault("animation_wd_start", "200");
	settings->setDefault("animation_wd_stop", "219");
*/
	settings->setDefault("more_threads", "true");
	settings->setDefault("console_enabled", debug ? "true" : "false");

	if (win32) {
		settings->setDefault("client_unload_unused_data_timeout", "30");
		settings->setDefault("server_unload_unused_data_timeout", "45");
	}

	settings->setDefault("minimap_shape_round", "false");
	settings->setDefault("mainmenu_last_selected_world", "1");


#ifdef __ANDROID__
	settings->setDefault("TMPFolder", porting::path_user + "/tmp/");

	//check for device with small screen
	float x_inches = porting::getDisplaySize().X / porting::get_dpi();

	settings->setDefault("smooth_lighting", "false");
	//settings->setDefault("enable_3d_clouds", "false");

	settings->setDefault("fps_max", "30");
	settings->setDefault("mouse_sensitivity", "0.1");

	settings->setDefault("sound_volume", "1");
	settings->setDefault("doubletap_jump", "1");

	/*
	settings->setDefault("max_simultaneous_block_sends_per_client", "3");
	settings->setDefault("emergequeue_limit_diskonly", "8");
	settings->setDefault("emergequeue_limit_generate", "8");
	*/
	//settings->setDefault("viewing_range", "25");
	settings->setDefault("num_emerge_threads", "1"); // too unstable when > 1
	settings->setDefault("inventory_image_hack", "false");
	if (x_inches  < 7) {
		settings->setDefault("enable_minimap", "false");
	}

	if (x_inches  < 3.5) {
		settings->setDefault("hud_scaling", "0.6");
	} else if (x_inches < 4.5) {
		settings->setDefault("hud_scaling", "0.7");
	} else if (x_inches < 7) {
		settings->setDefault("hud_scaling", "0.8");
	}

	settings->setDefault("curl_verify_cert", "false");

	settings->setDefault("chunksize", "3");
	settings->setDefault("server_map_save_interval", "60");
	settings->setDefault("server_unload_unused_data_timeout", "65");
	settings->setDefault("client_unload_unused_data_timeout", "60");
	//settings->setDefault("max_objects_per_block", "20");

	settings->setDefault("leaves_style", "opaque");
	//settings->setDefault("mg_name", "v7");

	char lang[3] = {};
	AConfiguration_getLanguage(porting::app_global->config, lang);
	settings->setDefault("language", lang);
	settings->setDefault("android_keyboard", "0");
	settings->setDefault("texture_min_size", "16");
	settings->setDefault("cloud_radius", "6");


	{
	std::stringstream fontsize;
	auto density = porting::getDisplayDensity();
	if (density > 1.6 && porting::getDisplaySize().X > 1024)
		density = 1.6;
	float font_size = 10 * density;

	fontsize << (int)font_size;

	settings->setDefault("font_size", fontsize.str());
	settings->setDefault("mono_font_size", fontsize.str());
	settings->setDefault("fallback_font_size", fontsize.str());

	actionstream << "Autoconfig: "" displayX=" << porting::getDisplaySize().X 
		<< " density=" << porting::getDisplayDensity() 
		<< " dpi=" << porting::get_dpi()
		//<< " densityDpi=" << porting::get_densityDpi()
		<< " x_inches=" << x_inches 
		<< " font=" << font_size 
		<< " lang=" << lang <<std::endl;
	}

#endif

#ifdef HAVE_TOUCHSCREENGUI
	settings->setDefault("touchscreen", android ? "true" : "false");
	settings->setDefault("touchtarget", "true");
	settings->setDefault("touchscreen_threshold","20");
#endif

}


// End of freeminer ======


void set_default_settings()
{
	Settings *settings = Settings::createLayer(SL_DEFAULTS);

	// Client and server
	settings->setDefault("language", "");
	settings->setDefault("name", "");
	settings->setDefault("bind_address", "");
	settings->setDefault("serverlist_url", "servers.minetest.net");

	// Client
	settings->setDefault("address", "");
	settings->setDefault("enable_sound", "true");
	settings->setDefault("sound_volume", "0.8");
	settings->setDefault("mute_sound", "false");
	settings->setDefault("enable_mesh_cache", "false");
	settings->setDefault("mesh_generation_interval", "0");
	settings->setDefault("mesh_generation_threads", "0");
	settings->setDefault("meshgen_block_cache_size", "20");
	settings->setDefault("enable_vbo", "true");
	settings->setDefault("free_move", "false");
	settings->setDefault("pitch_move", "false");
	settings->setDefault("fast_move", "false");
	settings->setDefault("noclip", "false");
	settings->setDefault("screenshot_path", "screenshots");
	settings->setDefault("screenshot_format", "png");
	settings->setDefault("screenshot_quality", "0");
	settings->setDefault("client_unload_unused_data_timeout", "300");
	settings->setDefault("client_mapblock_limit", "7500");
	settings->setDefault("enable_build_where_you_stand", "false");
	settings->setDefault("curl_timeout", "20000");
	settings->setDefault("curl_parallel_limit", "8");
	settings->setDefault("curl_file_download_timeout", "300000");
	settings->setDefault("curl_verify_cert", "true");
	settings->setDefault("enable_remote_media_server", "true");
	settings->setDefault("enable_client_modding", "false");
	settings->setDefault("max_out_chat_queue_size", "20");
	settings->setDefault("pause_on_lost_focus", "false");
	settings->setDefault("enable_split_login_register", "true");
	settings->setDefault("occlusion_culler", "bfs");
	settings->setDefault("enable_raytraced_culling", "true");
	settings->setDefault("chat_weblink_color", "#8888FF");

	// Keymap
	settings->setDefault("remote_port", "30000");
	settings->setDefault("keymap_forward", "KEY_KEY_W");
	settings->setDefault("keymap_autoforward", "");
	settings->setDefault("keymap_backward", "KEY_KEY_S");
	settings->setDefault("keymap_left", "KEY_KEY_A");
	settings->setDefault("keymap_right", "KEY_KEY_D");
	settings->setDefault("keymap_jump", "KEY_SPACE");
	settings->setDefault("keymap_sneak", "KEY_LSHIFT");
	settings->setDefault("keymap_dig", "KEY_LBUTTON");
	settings->setDefault("keymap_place", "KEY_RBUTTON");
	settings->setDefault("keymap_drop", "KEY_KEY_Q");
	settings->setDefault("keymap_zoom", "KEY_KEY_Z");
	settings->setDefault("keymap_inventory", "KEY_KEY_I");
	settings->setDefault("keymap_aux1", "KEY_KEY_E");
	settings->setDefault("keymap_chat", "KEY_KEY_T");
	settings->setDefault("keymap_cmd", "/");
	settings->setDefault("keymap_cmd_local", ".");
	settings->setDefault("keymap_minimap", "KEY_KEY_V");
	settings->setDefault("keymap_console", "KEY_F10");
#if HAVE_TOUCHSCREENGUI
	// See https://github.com/minetest/minetest/issues/12792
	settings->setDefault("keymap_rangeselect", "KEY_KEY_R");
#else
	settings->setDefault("keymap_rangeselect", "");
#endif
	settings->setDefault("keymap_freemove", "KEY_KEY_K");
	settings->setDefault("keymap_pitchmove", "");
	settings->setDefault("keymap_fastmove", "KEY_KEY_J");
	settings->setDefault("keymap_noclip", "KEY_KEY_H");
	settings->setDefault("keymap_hotbar_next", "KEY_KEY_N");
	settings->setDefault("keymap_hotbar_previous", "KEY_KEY_B");
	settings->setDefault("keymap_mute", "KEY_KEY_M");
	settings->setDefault("keymap_increase_volume", "");
	settings->setDefault("keymap_decrease_volume", "");
	settings->setDefault("keymap_cinematic", "");
	settings->setDefault("keymap_toggle_block_bounds", "");
	settings->setDefault("keymap_toggle_hud", "KEY_F1");
	settings->setDefault("keymap_toggle_chat", "KEY_F2");
	settings->setDefault("keymap_toggle_fog", "KEY_F3");
#if DEBUG
	settings->setDefault("keymap_toggle_update_camera", "KEY_F4");
#else
	settings->setDefault("keymap_toggle_update_camera", "");
#endif
	settings->setDefault("keymap_toggle_debug", "KEY_F5");
	settings->setDefault("keymap_toggle_profiler", "KEY_F6");
	settings->setDefault("keymap_camera_mode", "KEY_KEY_C");
	settings->setDefault("keymap_screenshot", "KEY_F12");
	settings->setDefault("keymap_increase_viewing_range_min", "+");
	settings->setDefault("keymap_decrease_viewing_range_min", "-");
	settings->setDefault("keymap_slot1", "KEY_KEY_1");
	settings->setDefault("keymap_slot2", "KEY_KEY_2");
	settings->setDefault("keymap_slot3", "KEY_KEY_3");
	settings->setDefault("keymap_slot4", "KEY_KEY_4");
	settings->setDefault("keymap_slot5", "KEY_KEY_5");
	settings->setDefault("keymap_slot6", "KEY_KEY_6");
	settings->setDefault("keymap_slot7", "KEY_KEY_7");
	settings->setDefault("keymap_slot8", "KEY_KEY_8");
	settings->setDefault("keymap_slot9", "KEY_KEY_9");
	settings->setDefault("keymap_slot10", "KEY_KEY_0");
	settings->setDefault("keymap_slot11", "");
	settings->setDefault("keymap_slot12", "");
	settings->setDefault("keymap_slot13", "");
	settings->setDefault("keymap_slot14", "");
	settings->setDefault("keymap_slot15", "");
	settings->setDefault("keymap_slot16", "");
	settings->setDefault("keymap_slot17", "");
	settings->setDefault("keymap_slot18", "");
	settings->setDefault("keymap_slot19", "");
	settings->setDefault("keymap_slot20", "");
	settings->setDefault("keymap_slot21", "");
	settings->setDefault("keymap_slot22", "");
	settings->setDefault("keymap_slot23", "");
	settings->setDefault("keymap_slot24", "");
	settings->setDefault("keymap_slot25", "");
	settings->setDefault("keymap_slot26", "");
	settings->setDefault("keymap_slot27", "");
	settings->setDefault("keymap_slot28", "");
	settings->setDefault("keymap_slot29", "");
	settings->setDefault("keymap_slot30", "");
	settings->setDefault("keymap_slot31", "");
	settings->setDefault("keymap_slot32", "");

#ifndef NDEBUG
	// Default keybinds for quicktune in debug builds
	settings->setDefault("keymap_quicktune_prev", "KEY_HOME");
	settings->setDefault("keymap_quicktune_next", "KEY_END");
	settings->setDefault("keymap_quicktune_dec", "KEY_NEXT");
	settings->setDefault("keymap_quicktune_inc", "KEY_PRIOR");
#else
	settings->setDefault("keymap_quicktune_prev", "");
	settings->setDefault("keymap_quicktune_next", "");
	settings->setDefault("keymap_quicktune_dec", "");
	settings->setDefault("keymap_quicktune_inc", "");
#endif

	// Visuals
#ifdef NDEBUG
	settings->setDefault("show_debug", "false");
#else
	settings->setDefault("show_debug", "true");
#endif
	settings->setDefault("fsaa", "0");
	settings->setDefault("undersampling", "0");
	settings->setDefault("world_aligned_mode", "enable");
	settings->setDefault("autoscale_mode", "disable");
	settings->setDefault("enable_fog", "true");
	settings->setDefault("fog_start", "0.4");
	settings->setDefault("3d_mode", "none");
	settings->setDefault("3d_paralax_strength", "0.025");
	settings->setDefault("tooltip_show_delay", "400");
	settings->setDefault("tooltip_append_itemname", "false");
	settings->setDefault("fps_max", "60");
	settings->setDefault("fps_max_unfocused", "20");
	settings->setDefault("viewing_range", "190");
	settings->setDefault("client_mesh_chunk", "1");
#if ENABLE_GLES
	settings->setDefault("near_plane", "0.1");
#endif
	settings->setDefault("screen_w", "1024");
	settings->setDefault("screen_h", "600");
	settings->setDefault("autosave_screensize", "true");
	settings->setDefault("fullscreen", "false");
	settings->setDefault("vsync", "false");
	settings->setDefault("fov", "72");
	settings->setDefault("leaves_style", "fancy");
	settings->setDefault("connected_glass", "false");
	settings->setDefault("smooth_lighting", "true");
	settings->setDefault("performance_tradeoffs", "false");
	settings->setDefault("lighting_alpha", "0.0");
	settings->setDefault("lighting_beta", "1.5");
	settings->setDefault("display_gamma", "1.0");
	settings->setDefault("lighting_boost", "0.2");
	settings->setDefault("lighting_boost_center", "0.5");
	settings->setDefault("lighting_boost_spread", "0.2");
	settings->setDefault("texture_path", "");
	settings->setDefault("shader_path", "");
#if ENABLE_GLES
	settings->setDefault("video_driver", "ogles2");
#else
	settings->setDefault("video_driver", "opengl");
#endif
	settings->setDefault("cinematic", "false");
	settings->setDefault("camera_smoothing", "0");
	settings->setDefault("cinematic_camera_smoothing", "0.7");
	settings->setDefault("enable_clouds", "true");
	settings->setDefault("view_bobbing_amount", "1.0");
	settings->setDefault("fall_bobbing_amount", "0.03");
	settings->setDefault("enable_3d_clouds", "true");
	settings->setDefault("cloud_radius", "12");
	settings->setDefault("menu_clouds", "true");
	settings->setDefault("opaque_water", "false");
	settings->setDefault("console_height", "0.6");
	settings->setDefault("console_color", "(0,0,0)");
	settings->setDefault("console_alpha", "200");
	settings->setDefault("formspec_fullscreen_bg_color", "(0,0,0)");
	settings->setDefault("formspec_fullscreen_bg_opacity", "140");
	settings->setDefault("formspec_default_bg_color", "(0,0,0)");
	settings->setDefault("formspec_default_bg_opacity", "140");
	settings->setDefault("selectionbox_color", "(0,0,0)");
	settings->setDefault("selectionbox_width", "2");
	settings->setDefault("node_highlighting", "box");
	settings->setDefault("crosshair_color", "(255,255,255)");
	settings->setDefault("crosshair_alpha", "255");
	settings->setDefault("recent_chat_messages", "6");
	settings->setDefault("hud_scaling", "1.0");
	settings->setDefault("gui_scaling", "1.0");
	settings->setDefault("gui_scaling_filter", "false");
	settings->setDefault("gui_scaling_filter_txr2img", "true");
	settings->setDefault("desynchronize_mapblock_texture_animation", "true");
	settings->setDefault("hud_hotbar_max_width", "1.0");
	settings->setDefault("enable_local_map_saving", "false");
	settings->setDefault("show_entity_selectionbox", "false");
	settings->setDefault("texture_clean_transparent", "false");
	settings->setDefault("texture_min_size", "64");
	settings->setDefault("ambient_occlusion_gamma", "1.8");
	settings->setDefault("enable_shaders", "true");
	settings->setDefault("enable_particles", "true");
	settings->setDefault("arm_inertia", "true");
	settings->setDefault("show_nametag_backgrounds", "true");
	settings->setDefault("transparency_sorting_distance", "16");

	settings->setDefault("enable_minimap", "true");
	settings->setDefault("minimap_shape_round", "true");
	settings->setDefault("minimap_double_scan_height", "true");

	// Effects
	settings->setDefault("directional_colored_fog", "true");
	settings->setDefault("inventory_items_animations", "false");
	settings->setDefault("mip_map", "false");
	settings->setDefault("anisotropic_filter", "false");
	settings->setDefault("bilinear_filter", "false");
	settings->setDefault("trilinear_filter", "false");
	settings->setDefault("tone_mapping", "false");
	settings->setDefault("enable_waving_water", "false");
	settings->setDefault("water_wave_height", "1.0");
	settings->setDefault("water_wave_length", "20.0");
	settings->setDefault("water_wave_speed", "5.0");
	settings->setDefault("enable_waving_leaves", "false");
	settings->setDefault("enable_waving_plants", "false");
	settings->setDefault("exposure_compensation", "0.0");
	settings->setDefault("enable_auto_exposure", "false");
	settings->setDefault("enable_bloom", "false");
	settings->setDefault("enable_bloom_debug", "false");
	settings->setDefault("bloom_strength_factor", "1.0");
	settings->setDefault("bloom_intensity", "0.05");
	settings->setDefault("bloom_radius", "1");

	// Effects Shadows
	settings->setDefault("enable_dynamic_shadows", "false");
	settings->setDefault("shadow_strength_gamma", "1.0");
	settings->setDefault("shadow_map_max_distance", "200.0");
	settings->setDefault("shadow_map_texture_size", "2048");
	settings->setDefault("shadow_map_texture_32bit", "true");
	settings->setDefault("shadow_map_color", "false");
	settings->setDefault("shadow_filters", "1");
	settings->setDefault("shadow_poisson_filter", "true");
	settings->setDefault("shadow_update_frames", "8");
	settings->setDefault("shadow_soft_radius", "5.0");
	settings->setDefault("shadow_sky_body_orbit_tilt", "0.0");

	// Input
	settings->setDefault("invert_mouse", "false");
	settings->setDefault("mouse_sensitivity", "0.2");
	settings->setDefault("repeat_place_time", "0.25");
	settings->setDefault("safe_dig_and_place", "false");
	settings->setDefault("random_input", "false");
	settings->setDefault("aux1_descends", "false");
	settings->setDefault("doubletap_jump", "false");
	settings->setDefault("always_fly_fast", "true");
#ifdef HAVE_TOUCHSCREENGUI
	settings->setDefault("autojump", "true");
#else
	settings->setDefault("autojump", "false");
#endif
	settings->setDefault("continuous_forward", "false");
	settings->setDefault("enable_joysticks", "false");
	settings->setDefault("joystick_id", "0");
	settings->setDefault("joystick_type", "");
	settings->setDefault("repeat_joystick_button_time", "0.17");
	settings->setDefault("joystick_frustum_sensitivity", "170");
	settings->setDefault("joystick_deadzone", "2048");

	// Main menu
	settings->setDefault("main_menu_path", "");
	settings->setDefault("serverlist_file", "favoriteservers.json");

	// General font settings
	settings->setDefault("font_path", porting::getDataPath("fonts" DIR_DELIM "Arimo-Regular.ttf"));
	settings->setDefault("font_path_italic", porting::getDataPath("fonts" DIR_DELIM "Arimo-Italic.ttf"));
	settings->setDefault("font_path_bold", porting::getDataPath("fonts" DIR_DELIM "Arimo-Bold.ttf"));
	settings->setDefault("font_path_bold_italic", porting::getDataPath("fonts" DIR_DELIM "Arimo-BoldItalic.ttf"));
	settings->setDefault("font_bold", "false");
	settings->setDefault("font_italic", "false");
	settings->setDefault("font_shadow", "1");
	settings->setDefault("font_shadow_alpha", "127");
	settings->setDefault("font_size_divisible_by", "1");
	settings->setDefault("mono_font_path", porting::getDataPath("fonts" DIR_DELIM "Cousine-Regular.ttf"));
	settings->setDefault("mono_font_path_italic", porting::getDataPath("fonts" DIR_DELIM "Cousine-Italic.ttf"));
	settings->setDefault("mono_font_path_bold", porting::getDataPath("fonts" DIR_DELIM "Cousine-Bold.ttf"));
	settings->setDefault("mono_font_path_bold_italic", porting::getDataPath("fonts" DIR_DELIM "Cousine-BoldItalic.ttf"));
	settings->setDefault("mono_font_size_divisible_by", "1");
	settings->setDefault("fallback_font_path", porting::getDataPath("fonts" DIR_DELIM "DroidSansFallbackFull.ttf"));

	std::string font_size_str = std::to_string(TTF_DEFAULT_FONT_SIZE);
	settings->setDefault("font_size", font_size_str);
	settings->setDefault("mono_font_size", font_size_str);
	settings->setDefault("chat_font_size", "0"); // Default "font_size"

	// ContentDB
	settings->setDefault("contentdb_url", "https://content.minetest.net");
	settings->setDefault("contentdb_max_concurrent_downloads", "3");

#ifdef __ANDROID__
	settings->setDefault("contentdb_flag_blacklist", "nonfree, android_default");
#else
	settings->setDefault("contentdb_flag_blacklist", "nonfree, desktop_default");
#endif

	settings->setDefault("update_information_url", "https://freeminer.org/release_info.json");
#if ENABLE_UPDATE_CHECKER
	settings->setDefault("update_last_checked", "");
#else
	settings->setDefault("update_last_checked", "disabled");
#endif

	// Server
	settings->setDefault("disable_escape_sequences", "false");
	settings->setDefault("strip_color_codes", "false");
#if USE_PROMETHEUS
	settings->setDefault("prometheus_listener_address", "127.0.0.1:30000");
#endif

	// Network
	settings->setDefault("enable_ipv6", "true");
	settings->setDefault("ipv6_server", "false");
	settings->setDefault("max_packets_per_iteration","1024");
	settings->setDefault("port", "30000");
	settings->setDefault("strict_protocol_version_checking", "false");
	settings->setDefault("player_transfer_distance", "0");
	settings->setDefault("max_simultaneous_block_sends_per_client", "40");
	settings->setDefault("time_send_interval", "5");

	settings->setDefault("default_game", "minetest");
	settings->setDefault("motd", "");
	settings->setDefault("max_users", "15");
	settings->setDefault("creative_mode", "false");
	settings->setDefault("enable_damage", "true");
	settings->setDefault("default_password", "");
	settings->setDefault("default_privs", "interact, shout");
	settings->setDefault("enable_pvp", "true");
	settings->setDefault("enable_mod_channels", "false");
	settings->setDefault("disallow_empty_password", "false");
	settings->setDefault("disable_anticheat", "false");
	settings->setDefault("enable_rollback_recording", "false");
	settings->setDefault("deprecated_lua_api_handling", "log");

	settings->setDefault("kick_msg_shutdown", "Server shutting down.");
	settings->setDefault("kick_msg_crash", "This server has experienced an internal error. You will now be disconnected.");
	settings->setDefault("ask_reconnect_on_crash", "false");

	settings->setDefault("chat_message_format", "<@name> @message");
	settings->setDefault("profiler_print_interval", "0");
	settings->setDefault("active_object_send_range_blocks", "8");
	settings->setDefault("active_block_range", "4");
	//settings->setDefault("max_simultaneous_block_sends_per_client", "1");
	// This causes frametime jitter on client side, or does it?
	settings->setDefault("max_block_send_distance", "12");
	settings->setDefault("block_send_optimize_distance", "4");
	settings->setDefault("server_side_occlusion_culling", "true");
	settings->setDefault("csm_restriction_flags", "62");
	settings->setDefault("csm_restriction_noderange", "0");
	settings->setDefault("max_clearobjects_extra_loaded_blocks", "4096");
	settings->setDefault("time_speed", "72");
	settings->setDefault("world_start_time", "6125");
	settings->setDefault("server_unload_unused_data_timeout", "29");
	settings->setDefault("max_objects_per_block", "256");
	settings->setDefault("server_map_save_interval", "5.3");
	settings->setDefault("chat_message_max_size", "500");
	settings->setDefault("chat_message_limit_per_10sec", "8.0");
	settings->setDefault("chat_message_limit_trigger_kick", "50");
	settings->setDefault("sqlite_synchronous", "2");
	settings->setDefault("map_compression_level_disk", "-1");
	settings->setDefault("map_compression_level_net", "-1");
	settings->setDefault("full_block_send_enable_min_time_from_building", "2.0");
	settings->setDefault("dedicated_server_step", "0.09");
	settings->setDefault("active_block_mgmt_interval", "2.0");
	settings->setDefault("abm_interval", "1.0");
	settings->setDefault("abm_time_budget", "0.2");
	settings->setDefault("nodetimer_interval", "0.2");
	settings->setDefault("ignore_world_load_errors", "false");
	settings->setDefault("remote_media", "");
	settings->setDefault("debug_log_level", "action");
	settings->setDefault("debug_log_size_max", "50");
	settings->setDefault("chat_log_level", "error");
	settings->setDefault("emergequeue_limit_total", "1024");
	settings->setDefault("emergequeue_limit_diskonly", "128");
	settings->setDefault("emergequeue_limit_generate", "128");
	settings->setDefault("num_emerge_threads", "1");
	settings->setDefault("secure.enable_security", "true");
	settings->setDefault("secure.trusted_mods", "");
	settings->setDefault("secure.http_mods", "");

	// Physics
	settings->setDefault("movement_acceleration_default", "3");
	settings->setDefault("movement_acceleration_air", "2");
	settings->setDefault("movement_acceleration_fast", "10");
	settings->setDefault("movement_speed_walk", "4");
	settings->setDefault("movement_speed_crouch", "1.35");
	settings->setDefault("movement_speed_fast", "20");
	settings->setDefault("movement_speed_climb", "3");
	settings->setDefault("movement_speed_jump", "6.5");
	settings->setDefault("movement_liquid_fluidity", "1");
	settings->setDefault("movement_liquid_fluidity_smooth", "0.5");
	settings->setDefault("movement_liquid_sink", "10");
	settings->setDefault("movement_gravity", "9.81");

	// Liquids
	settings->setDefault("liquid_loop_max", "100000");
	settings->setDefault("liquid_queue_purge_time", "0");
	settings->setDefault("liquid_update", "1.0");

	// Mapgen
	settings->setDefault("mg_name", "v7");
	settings->setDefault("water_level", "1");
	settings->setDefault("mapgen_limit", std::to_string(MAX_MAP_GENERATION_LIMIT));
	settings->setDefault("chunksize", "5");
	settings->setDefault("fixed_map_seed", "");
	settings->setDefault("max_block_generate_distance", "10");
	settings->setDefault("enable_mapgen_debug_info", "false");
	Mapgen::setDefaultSettings(settings);

	// Server list announcing
	settings->setDefault("server_announce", "false");
	settings->setDefault("server_url", "");
	settings->setDefault("server_address", "");
	settings->setDefault("server_name", "");
	settings->setDefault("server_description", "");

	settings->setDefault("enable_console", "false");
	settings->setDefault("screen_dpi", "72");
	settings->setDefault("display_density_factor", "1");

	// Altered settings for macOS
#if defined(__MACH__) && defined(__APPLE__)
	settings->setDefault("keymap_sneak", "KEY_SHIFT");
#endif

#ifdef HAVE_TOUCHSCREENGUI
	settings->setDefault("touchscreen_threshold","20");
	settings->setDefault("touch_use_crosshair", "false");
	settings->setDefault("fixed_virtual_joystick", "false");
	settings->setDefault("virtual_joystick_triggers_aux1", "false");
	settings->setDefault("clickable_chat_weblinks", "false");
#else
	settings->setDefault("clickable_chat_weblinks", "true");
#endif
	// Altered settings for Android
#ifdef __ANDROID__
	settings->setDefault("screen_w", "0");
	settings->setDefault("screen_h", "0");
	settings->setDefault("fullscreen", "true");
	settings->setDefault("smooth_lighting", "false");
	settings->setDefault("performance_tradeoffs", "true");
	settings->setDefault("max_simultaneous_block_sends_per_client", "10");
	settings->setDefault("emergequeue_limit_diskonly", "16");
	settings->setDefault("emergequeue_limit_generate", "16");
	settings->setDefault("max_block_generate_distance", "5");
	settings->setDefault("enable_3d_clouds", "false");
	settings->setDefault("fps_max_unfocused", "10");
	settings->setDefault("sqlite_synchronous", "1");
	settings->setDefault("map_compression_level_disk", "-1");
	settings->setDefault("map_compression_level_net", "-1");
	settings->setDefault("server_map_save_interval", "15");
	settings->setDefault("client_mapblock_limit", "1000");
	settings->setDefault("active_block_range", "2");
	settings->setDefault("viewing_range", "50");
	settings->setDefault("leaves_style", "simple");
	settings->setDefault("curl_verify_cert","false");

	// Apply settings according to screen size
	float x_inches = (float) porting::getDisplaySize().X /
			(160.f * porting::getDisplayDensity());

	if (x_inches < 3.7f) {
		settings->setDefault("hud_scaling", "0.6");
		settings->setDefault("font_size", "14");
		settings->setDefault("mono_font_size", "14");
	} else if (x_inches < 4.5f) {
		settings->setDefault("hud_scaling", "0.7");
		settings->setDefault("font_size", "14");
		settings->setDefault("mono_font_size", "14");
	} else if (x_inches < 6.0f) {
		settings->setDefault("hud_scaling", "0.85");
		settings->setDefault("font_size", "14");
		settings->setDefault("mono_font_size", "14");
	}
	// Tablets >= 6.0 use non-Android defaults for these settings
#endif

	fm_set_default_settings(settings);
}
