set(
  SOURCES
  DiscordClient.cpp
  DiscordESDAction.cpp
  DiscordESDActionV2.cpp
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
sign_target(sddiscord)
install(TARGETS sddiscord DESTINATION ${CMAKE_INSTALL_PREFIX})
if(WIN32)
  install(FILES "$<TARGET_PDB_FILE:sddiscord>" DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
