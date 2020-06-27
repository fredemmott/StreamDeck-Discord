include(ExternalProject)

ExternalProject_Add(
  StreamDeckSDK_build
  GIT_REPOSITORY ${CMAKE_CURRENT_SOURCE_DIR}/../StreamDeck-CPPSDK/.git
  GIT_TAG HEAD
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
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
  "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
target_include_directories(StreamDeckSDK INTERFACE ${INSTALL_DIR}/include)
target_compile_definitions(StreamDeckSDK INTERFACE -DASIO_STANDALONE=1)
