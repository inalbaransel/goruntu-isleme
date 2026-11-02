cmake_minimum_required(VERSION 3.16)
project(Hafta1Gorev1 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(resize main.cpp)
target_link_libraries(resize ${OpenCV_LIBS})
