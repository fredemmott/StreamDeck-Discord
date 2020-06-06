include(ExternalProject)

ExternalProject_Add(
  StreamDeckSDK_build
  GIT_REPOSITORY
  https://github.com/fredemmott/StreamDeck-CPPSDK
  CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
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

if (APPLE)
  find_library(CORE_FOUNDATION_LIBRARY CoreFoundation REQUIRED)
  target_link_libraries(
    StreamDeckSDK
    INTERFACE
    ${CORE_FOUNDATION_LIBRARY}
  )
endif()
