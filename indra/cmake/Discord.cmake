# -*- cmake -*-
include(Prebuilt)
include(Variables)

if(INSTALL_PROPRIETARY)
	set(USE_DISCORD_RPC ON)
endif(INSTALL_PROPRIETARY)

if(USE_DISCORD_RPC)
	if(NOT USESYSTEMLIBS)
		use_prebuilt_binary(discord_rpc)
		# TODO: Finish Autobuild packages
		if(WINDOWS)
			message("Discord RPC Support for Windows is not implemented yet.")
			set(USE_DISCORD_RPC OFF)
		elseif(DARWIN)
			message("Discord RPC Support for MacOS is not implemented yet.")
			set(USE_DISCORD_RPC OFF)
		elseif(LINUX)
			set(DISCORDRPC_LIBRARY libdiscord-rpc.a)
			set(DISCORDRPC_INCLUDE_DIR "${LIBS_PREBUILT_DIR}/include/discord_rpc")
		endif(WINDOWS)
	endif(NOT USESYSTEMLIBS)
	if (USE_DISCORD_RPC AND NOT DISCORD_APP_ID)
		message(FATAL_ERROR "You must pass an application ID to use Discord RPC!")
	else(USE_DISCORD_RPC AND DISCORD_APP_ID)
		add_definitions(
			-DUSE_DISCORD_RPC
			-DDISCORD_APP_ID=${DISCORD_APP_ID})
	endif(USE_DISCORD_RPC AND NOT DISCORD_APP_ID)
endif(USE_DISCORD_RPC)
