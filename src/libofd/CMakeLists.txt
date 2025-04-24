cmake_minimum_required(VERSION 3.10)
project(ofdcpp VERSION 0.1.0 LANGUAGES CXX)

# Build options
option(BUILD_EXAMPLES "Build example applications" OFF)
option(BUILD_TESTS "Build test applications" OFF)

# C++ standard settings
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Source files
file(GLOB SOURCES
    "src/ofd_reader.cpp"
    "src/ofd_writer.cpp"
)

file(GLOB HEADERS
    "include/ofd/*.h"
)

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Core Gui Xml)

# 使用 pkg-config 查找 QuaZip
find_package(PkgConfig REQUIRED)

if(QT_DESIRED_VERSION MATCHES 6)
    pkg_check_modules(QUAZIP REQUIRED quazip1-qt6)
else()
    pkg_check_modules(QUAZIP quazip1-qt5 QUIET)

    if(NOT QUAZIP_FOUND)
        # find the old quazip package
        message(STATUS "quazip1-qt5 not found, trying to find the old package libquazip5-dev")
        find_package(QuaZip5 REQUIRED)
    endif()
endif()

message(STATUS "QuaZip Include Dirs: ${QUAZIP_INCLUDE_DIRS}")
message(STATUS "QuaZip Libraries: ${QUAZIP_LIBRARIES}")

add_library(ofdcpp STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QUAZIP_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Qt${QT_DESIRED_VERSION}::Core
    Qt${QT_DESIRED_VERSION}::Gui
    Qt${QT_DESIRED_VERSION}::Xml
    ${QUAZIP_LIBRARIES}
)

# # 安装头文件
# install(DIRECTORY include/ofd
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )

# 然后再构建示例
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
