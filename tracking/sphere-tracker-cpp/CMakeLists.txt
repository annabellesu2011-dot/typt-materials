cmake_minimum_required(VERSION 3.14)
project(sphere_tracker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc highgui videoio video
             OPTIONAL_COMPONENTS tracking)

add_executable(sphere_tracker src/main.cpp src/tracker.cpp src/analysis.cpp)
target_include_directories(sphere_tracker PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sphere_tracker PRIVATE ${OpenCV_LIBS})

if(OpenCV_tracking_FOUND OR TARGET opencv_tracking)
  target_compile_definitions(sphere_tracker PRIVATE HAVE_OPENCV_CSRT=1)
  message(STATUS "opencv_tracking found: --tracker csrt enabled")
else()
  message(STATUS "opencv_tracking not found: --tracker csrt disabled")
endif()

if(MSVC)
  target_compile_options(sphere_tracker PRIVATE /W3 /permissive-)
else()
  target_compile_options(sphere_tracker PRIVATE -Wall -Wextra)
endif()
