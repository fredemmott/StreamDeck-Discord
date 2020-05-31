cmake_minimum_required(VERSION 3.10)

project(com.fredemmott.discord.sdPlugin VERSION 2.4)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(Vendor)
add_subdirectory(Common)
add_subdirectory(Discord)

set(
  SOURCES
  CallbackTimer.cpp
  DiscordClient.cpp
  MyStreamDeckPlugin.cpp
)
if (APPLE)
  list(APPEND SOURCES macOS/DiscordClient_mac.cpp)
elseif (WIN32)
  list(APPEND SOURCES Windows/DiscordClient_win.cpp)
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
