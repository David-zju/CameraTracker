cmake_minimum_required(VERSION 3.12)
# set(CMAKE_BUILD_TYPE "Release") # 或者 "Release"
project(EasyPhotoGPS)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_STANDARD 20)
include(FindEXPAT)
find_package(exiv2 REQUIRED CONFIG NAMES exiv2)
find_package(EXPAT REQUIRED)
# 添加路径
include_directories(${EXPAT_INCLUDE_DIRS})
# include_directories(deps/exiv2/include)
# include_directories(deps/exiv2/build)
# include_directories(deps/exiv2/build/bin)
# include_directories(deps/exiv2/build/lib)


# 获取当前目录下所有的 .cpp 文件
file(GLOB CPP_SOURCES *.cpp)
# file(GLOB EXIV2_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/deps/exiv2/*.cpp")



# 添加所有的 .cpp 文件到可执行文件
add_executable(main main.cpp)
target_link_libraries(main PRIVATE Exiv2::exiv2lib ${EXPAT_LIBRARIES})
# target_link_libraries(main )
