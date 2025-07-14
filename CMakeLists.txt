cmake_minimum_required(VERSION 3.18)
project(main2)

# 设置CPP版本
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找指定目录及其子目录中的所有.cpp文件
function(find_all_files dir result pattern)
    file(GLOB_RECURSE new_files "${dir}/*.${pattern}")
    set(files ${${result}})
    list(APPEND files ${new_files})
    set(${result} ${files} PARENT_SCOPE)
endfunction()

if (1)
    set(ALL_CPP_FILES main.cpp
        vinteger_adder.cpp
        vinteger_bit_operation.cpp 
        vinteger_cast_for_string.cpp
        vinteger_compare.cpp
        vinteger_multiplier.cpp
        vinteger_divider.cpp
        vinteger.cpp
        )
else()
    set(ALL_CPP_FILES omain.cpp)
endif()

# 生成应用程序
add_executable(${PROJECT_NAME} ${ALL_CPP_FILES})
