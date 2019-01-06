cmake_minimum_required(VERSION 3.0.0)
project(lockable VERSION 0.1.0)

include(CTest)
enable_testing()

set (CMAKE_CXX_STANDARD 17)

add_executable(lockable main.cpp)
target_link_libraries(lockable -lpthread)
# enable this instead if you're building with boost.
# target_link_libraries(lockable -lpthread -lboost_system -lboost_thread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
