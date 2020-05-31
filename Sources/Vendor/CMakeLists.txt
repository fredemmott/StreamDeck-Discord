#### asio ####

add_library(asio INTERFACE)
target_compile_definitions(
  asio
  INTERFACE 
  ASIO_STANDALONE=1
)
target_include_directories(
  asio
  INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/asio/include"
)

#### websocketpp ####

add_library(websocketpp INTERFACE)
target_include_directories(
  websocketpp
  INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/websocketpp"
)
target_link_libraries(
  websocketpp
  INTERFACE
  asio
)
target_compile_definitions(
  websocketpp
  INTERFACE
  ASIO_STANDALONE=1
)
