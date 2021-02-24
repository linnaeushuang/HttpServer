cmake_minimum_required(VERSION 3.0)

project(httpServer)

set(SRCS
	Event.cpp
    Epoll.cpp
    EventLoop.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    HttpData.cpp
    Main.cpp
    Server.cpp
    Timer.cpp
	./utils/SocketUtils.cpp
)
set( CMAKE_CXX_FLAGS "-std=c++11" )
include_directories(${PROJECT_SOURCE_DIR}/utils)
add_executable(httpServer ${SRCS})
target_link_libraries(httpServer libutils_base)
add_subdirectory(utils)
