set(SOURCES connection.cpp rpc_connection.cpp)

add_library(
  DiscordRPCSDK
  STATIC
  ${SOURCES}
)
target_link_libraries(DiscordRPCSDK StreamDeckSDK)
