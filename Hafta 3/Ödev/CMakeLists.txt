cmake_minimum_required(VERSION 3.18)
project(Hafta3Odev CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

# CPU ve GPU sonuçlarının bit bit aynı çıkması için çarp-topla birleştirme kapalı
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>)

add_executable(odev_mt main_mt.cpp)
target_link_libraries(odev_mt ${OpenCV_LIBS} Threads::Threads)

# CUDA derleyicisi varsa (ör. Google Colab) GPU sürümü de derlenir
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 17)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
  endif()
  add_executable(odev_cuda main_cuda.cu)
  target_compile_options(odev_cuda PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-fmad=false>)
  target_link_libraries(odev_cuda ${OpenCV_LIBS} Threads::Threads)
else()
  message(STATUS "CUDA bulunamadi: sadece odev_mt derlenecek")
endif()
