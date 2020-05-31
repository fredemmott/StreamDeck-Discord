set(
  SOURCES
  CallbackTimer.cpp
  DiscordClient.cpp
  MyStreamDeckPlugin.cpp
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
install(TARGETS sddiscord DESTINATION ${CMAKE_INSTALL_PREFIX})
