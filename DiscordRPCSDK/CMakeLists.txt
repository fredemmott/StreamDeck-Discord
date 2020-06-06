set(SOURCES rpc_connection.cpp)
if (WIN32)
  list(APPEND SOURCES connection_win.cpp)
elseif (UNIX)
  list(APPEND SOURCES connection_unix.cpp)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

add_library(
  DiscordRPCSDK
  STATIC
  ${SOURCES}
)
target_link_libraries(DiscordRPCSDK StreamDeckSDK)
