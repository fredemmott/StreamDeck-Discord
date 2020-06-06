include(ExternalProject)

ExternalProject_Add(
  StreamDeckSDK_build
  URL https://github.com/fredemmott/StreamDeck-CPPSDK/releases/download/v0.5.7/StreamDeckSDK-v0.5.7.zip
  URL_HASH SHA512=bb1e22d047892827bf0ebb48afc3be9585342f343e0bf193213bad33bd603e6052e63587d2fa94ce686b5b21925e612493f9aeefc662d900f4c64c75ae21d761
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
