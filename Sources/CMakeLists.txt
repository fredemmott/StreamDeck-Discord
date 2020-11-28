set(CMAKE_CXX_STANDARD 17)

set(
  SOURCES
  CallbackTimer.cpp
  DiscordClient.cpp
  DiscordESDAction.cpp
  DiscordESDActionIDs.cpp
  DiscordStreamDeckPlugin.cpp
  main.cpp
)
if (APPLE)
  list(APPEND SOURCES DiscordClientMac.cpp)
elseif (WIN32)
  list(APPEND SOURCES DiscordClientWin.cpp)
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
if (MSVC)
  target_link_libraries(sddiscord wininet Rpcrt4)
endif()
install(TARGETS sddiscord DESTINATION ${CMAKE_INSTALL_PREFIX})
