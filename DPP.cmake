include(ExternalProject)

ExternalProject_Add(
  DPP_Build
  URL https://github.com/brainboxdotcc/DPP/archive/refs/tags/v9.0.16.zip
  URL_HASH SHA512=435d7abd7a4e172518ef67f31892a0a215b50694cbf2308255891d44db4ba08dff75c7594a3b7925ebfb8b3bb36b89d3e459fb5e0cd29dcee91f1ec78a3edd92
  CMAKE_ARGS
    -DBUILD_LIB_ONLY=ON
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property(
  DPP_Build
  INSTALL_DIR
)
add_library(DPP INTERFACE)
add_dependencies(DPP DPP_Build)
target_link_libraries(
  DPP
  INTERFACE
  ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}DPP${CMAKE_STATIC_LIBRARY_SUFFIX}
  "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
target_include_directories(DPP INTERFACE ${INSTALL_DIR}/include/dpp-9.0/)
target_compile_definitions(DPP INTERFACE -DASIO_STANDALONE=1)

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
  target_link_libraries(DPP INTERFACE pthread)
endif()
set(
  STREAMDECK_PLUGIN_DIR
  ${STREAMDECK_PLUGIN_DIR}
  CACHE PATH "Path to this system's streamdeck plugin directory"
)

function(set_default_install_dir_to_streamdeck_plugin_dir)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(
      CMAKE_INSTALL_PREFIX
      "${STREAMDECK_PLUGIN_DIR}/${CMAKE_PROJECT_NAME}"
      CACHE PATH "See cmake documentation"
      FORCE
    )
  endif()
endfunction()
