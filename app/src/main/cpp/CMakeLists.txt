cmake_minimum_required(VERSION 3.4.1)

file(GLOB SOURCE *.cpp)
add_library(
        native-lib
        SHARED
        ${SOURCE})

find_library(
        log-lib
        log)

# 引入头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cpp/include)
set(lib_path ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${ANDROID_ABI})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${lib_path}")

target_link_libraries(
        native-lib
        avfilter avformat avcodec avutil swresample swscale
        ${log-lib})