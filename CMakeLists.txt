cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(com.fredemmott.discord.sdPlugin VERSION 4.0.3 LANGUAGES CXX)

if (APPLE)
  set(
    STREAMDECK_PLUGIN_DIR
    "$ENV{HOME}/Library/ApplicationSupport/com.elgato.StreamDeck/Plugins"
  )
endif()
set(
  CMAKE_INSTALL_PREFIX
  "${STREAMDECK_PLUGIN_DIR}/${CMAKE_PROJECT_NAME}"
  CACHE STRING "See cmake documentation"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
  add_definitions("/Zc:__cplusplus")
endif()
if(WIN32)
  # Windows 10
  add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()
include_directories("${CMAKE_SOURCE_DIR}")
add_compile_definitions(JSON_USE_IMPLICIT_CONVERSIONS=0)

include("StreamDeckSDK.cmake")
include("sign_target.cmake")


add_subdirectory(DiscordRPCSDK)
add_subdirectory(Sources)
add_subdirectory(sdplugin)
