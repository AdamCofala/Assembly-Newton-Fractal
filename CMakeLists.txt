cmake_minimum_required(VERSION 3.16)

project(NewtonFractal LANGUAGES CXX C ASM_NASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# NASM (Linux ELF64)
# =========================
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

# =========================
# Packages (WSL / Linux)
# =========================
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# =========================
# GLAD (local)
# =========================
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/include/glad/glad.c
)

set_target_properties(glad PROPERTIES
    LINKER_LANGUAGE C
)

# =========================
# IMGUI (local, no CMake)
# =========================
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# =========================
# SOURCES
# =========================
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS
    asm/*.asm
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${ASM_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
)

# =========================
# DEFINES
# =========================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_NONE
)

# =========================
# LINK
# =========================
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        OpenGL::GL
        glad
        imgui
)