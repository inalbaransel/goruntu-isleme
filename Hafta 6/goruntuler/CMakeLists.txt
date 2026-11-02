cmake_minimum_required(VERSION 3.16)
project(Hafta6Goruntuler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)

add_executable(uret uret.cpp)
target_link_libraries(uret ${OpenCV_LIBS})
