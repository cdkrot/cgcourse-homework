cmake_minimum_required(VERSION 3.0)
project(task4 CXX)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else()
    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# CONFIG option is important so that CMake doesnt search for modules into the default modules directory
find_package(imgui CONFIG)
find_package(glfw CONFIG)
find_package(glew CONFIG)
find_package(fmt CONFIG)
find_package(glm CONFIG)
find_package(stb CONFIG)

add_executable( task4
                src/main.cpp
                src/opengl_shader.cpp
                src/opengl_shader.h
                src/miniconfig.cpp
                src/miniconfig.h
                src/stb_image_impl.cpp
                src/external/tiny_obj_loader.h
                src/external/tiny_obj_loader_impl.cpp
                bindings/imgui_impl_glfw.cpp
                bindings/imgui_impl_opengl3.cpp
                bindings/imgui_impl_glfw.h
                bindings/imgui_impl_opengl3.h )

target_include_directories(task4 PRIVATE . src src/external)
target_compile_definitions(task4 PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_link_libraries(task4 imgui::imgui GLEW::glew_s glfw::glfw fmt::fmt glm::glm stb::stb)
