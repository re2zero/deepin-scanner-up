cmake_minimum_required(VERSION 3.15)

# 启用测试
enable_testing()

# 查找Qt Test模块
find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Test)

# 添加测试可执行文件
add_executable(tests_shared_ptr
    test_shared_ptr.cpp
)

# 包含主项目头文件
target_include_directories(tests_shared_ptr PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/device
    ${CMAKE_SOURCE_DIR}/src/ui
)

# 链接Qt Test库和主项目
target_link_libraries(tests_shared_ptr PRIVATE
    Qt${QT_DESIRED_VERSION}::Test
    ${PROJECT_NAME}
)

# 启用自动moc处理
set_target_properties(tests_shared_ptr PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# 添加测试
add_test(NAME test_shared_ptr COMMAND tests_shared_ptr)