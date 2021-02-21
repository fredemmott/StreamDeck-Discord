include(ExternalProject)

ExternalProject_Add(
  StreamDeckSDK_build
  URL https://github.com/fredemmott/StreamDeck-CPPSDK/releases/download/v2.0.1/StreamDeckSDK-v2.0.1.zip
  URL_HASH SHA512=1d83d072d3c8be63394150aab3c42de6716d8d22acd4c696ffbe86a7da2e3b38a9cbbc968b3f021d9385d36c95d72b58cff78e30dafc44a68bb22300a8861290
  CMAKE_ARGS
    -DBUILD_LIB_ONLY=ON
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
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
  "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
target_include_directories(StreamDeckSDK INTERFACE ${INSTALL_DIR}/include)
target_compile_definitions(StreamDeckSDK INTERFACE -DASIO_STANDALONE=1)

if(APPLE)
  set(
    STREAMDECK_PLUGIN_DIR
    "$ENV{HOME}/Library/ApplicationSupport/com.elgato.StreamDeck/Plugins"
  )
elseif(WIN32)
  string(
    REPLACE
    "\\"
    "/"
    STREAMDECK_PLUGIN_DIR
    "$ENV{appdata}/Elgato/StreamDeck/Plugins"
  )
elseif(UNIX AND NOT APPLE)
  target_link_libraries(StreamDeckSDK INTERFACE pthread)
endif()
set(
  STREAMDECK_PLUGIN_DIR
  ${STREAMDECK_PLUGIN_DIR}
  CACHE PATH "Path to this system's streamdeck plugin directory"
)
