cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(com.fredemmott.discord.sdPlugin VERSION 4.0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
  add_definitions("/Zc:__cplusplus")
endif()
if(WIN32)
  # Windows 10
  add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
endif()
include_directories("${CMAKE_SOURCE_DIR}")
add_compile_definitions(JSON_USE_IMPLICIT_CONVERSIONS=0)

include("StreamDeckSDK.cmake")
set_default_install_dir_to_streamdeck_plugin_dir()

include("sign_target.cmake")


add_subdirectory(DiscordRPCSDK)
add_subdirectory(Sources)
add_subdirectory(sdplugin)
