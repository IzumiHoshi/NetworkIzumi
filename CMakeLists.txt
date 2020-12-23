
# file(GLOB LIBS "${CMAKE_CURRENT_SOURCE_DIR}/../Build/libFanucComm.so" ABSOLUTE)
# set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)

add_executable(TestServer TestServer.cpp)
add_executable(TestClient TestClient.cpp)
# target_include_directories(TestServer PRIVATE ${INCLUDE_DIR})
# target_link_libraries(TestServer PRIVATE FanucComm)