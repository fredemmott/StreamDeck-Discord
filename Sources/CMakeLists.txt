cmake_minimum_required(VERSION 3.10)

project(com.fredemmott.discord.sdPlugin VERSION 2.4)

if (APPLE)
  set(
    STREAMDECK_PLUGIN_DIR
    "$ENV{HOME}/Library/ApplicationSupport/com.elgato.StreamDeck/Plugins"
  )
endif()
set(
  CMAKE_INSTALL_PREFIX
  "${STREAMDECK_PLUGIN_DIR}/${CMAKE_PROJECT_NAME}"
)

set(CMAKE_CXX_STANDARD 17)
include_directories("${CMAKE_SOURCE_DIR}")

add_subdirectory(Vendor)
add_subdirectory(StreamDeckSDK)
add_subdirectory(DiscordRPCSDK)
add_subdirectory(sdplugin)

set(
  SOURCES
  CallbackTimer.cpp
  DiscordClient.cpp
  MyStreamDeckPlugin.cpp
)
if (APPLE)
  list(APPEND SOURCES DiscordClientMac.cpp)
elseif (WIN32)
  list(APPEND SOURCES DiscordClientWindows.cpp)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
add_executable(
  sddiscord
  ${SOURCES}
)
target_link_libraries(sddiscord StreamDeckSDK DiscordRPCSDK)
if (APPLE)
  target_link_libraries(sddiscord curl)
endif()
install(TARGETS sddiscord DESTINATION ${CMAKE_INSTALL_PREFIX})
