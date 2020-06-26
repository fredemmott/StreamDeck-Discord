include(ExternalProject)

ExternalProject_Add(
  StreamDeckSDK_build
  URL https://github.com/fredemmott/StreamDeck-CPPSDK/releases/download/v2.0-rc1/StreamDeckSDK-v2.0-rc1.zip
  URL_HASH SHA512=5df0c575ec851ddbdd85e8cd52de63e315ba0db758e628f03e4193ea2c46c4ed45d96e1e2a354ba7f60eeb92632192cdbabc911ca64467c68eab04e2ccf0d27b
  CMAKE_ARGS
    -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property(
  StreamDeckSDK_build
  INSTALL_DIR
)
add_library(StreamDeckSDK INTERFACE)
add_dependencies(StreamDeckSDK StreamDeckSDK_build)
target_link_libraries(
  StreamDeckSDK
  INTERFACE
  ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}StreamDeckSDK${CMAKE_STATIC_LIBRARY_SUFFIX}
)
target_include_directories(StreamDeckSDK INTERFACE ${INSTALL_DIR}/include)
target_compile_definitions(StreamDeckSDK INTERFACE -DASIO_STANDALONE=1)
