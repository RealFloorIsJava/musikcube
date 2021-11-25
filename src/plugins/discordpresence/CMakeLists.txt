set (discordpresence_SOURCES
  discordpresence_plugin.cpp
  DiscordPlaybackRemote.cpp
  DiscordPresence.cpp
  discord/achievement_manager.cpp
  discord/activity_manager.cpp
  discord/application_manager.cpp
  discord/core.cpp
  discord/image_manager.cpp
  discord/lobby_manager.cpp
  discord/network_manager.cpp
  discord/overlay_manager.cpp
  discord/relationship_manager.cpp
  discord/storage_manager.cpp
  discord/store_manager.cpp
  discord/types.cpp
  discord/user_manager.cpp
  discord/voice_manager.cpp
)

add_library(discordpresence SHARED ${discordpresence_SOURCES})

find_library(LIBDISCORD_GAME_SDK
             NAMES libdiscord_game_sdk discord_game_sdk
             PATHS "${CMAKE_CURRENT_SOURCE_DIR}/discord-lib"
             NO_DEFAULT_PATH REQUIRED)
if ("${LIBDISCORD_GAME_SDK}" STREQUAL "LIBDISCORD_GAME_SDK-NOTFOUND")
  message(STATUS "[discord_game_sdk] *not* found! plugin will not be built. make sure to add the libraries to discord-lib/!")
endif()

file(GLOB LIBDISCORD_GAME_SDK_LIBS LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/discord-lib/*discord_game_sdk*)
file(COPY ${LIBDISCORD_GAME_SDK_LIBS} DESTINATION ${musikcube_SOURCE_DIR}/bin FOLLOW_SYMLINK_CHAIN)
target_link_libraries(discordpresence ${LIBDISCORD_GAME_SDK})