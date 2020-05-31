cmake_minimum_required(VERSION 3.10)

project(com.fredemmott.discord.sdPlugin VERSION 2.4)

set(CMAKE_CXX_STANDARD 17)
include_directories("${CMAKE_SOURCE_DIR}")

add_subdirectory(Vendor)
add_subdirectory(StreamDeckSDK)
add_subdirectory(Discord)

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
target_link_libraries(sddiscord StreamDeckSDK DiscordRPC)
if (APPLE)
  target_link_libraries(sddiscord curl)
endif()
