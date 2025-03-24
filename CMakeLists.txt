cmake_minimum_required(VERSION 3.25.0)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type: Debug, Release" FORCE)
endif()

# Set compiler flags for Release and Debug
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")

project(CHIP8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# CPM
file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

set(LIBS)


## Dependencies

# SDL
CPMAddPackage("gh:libsdl-org/SDL#4c70aa3")
list(APPEND LIBS SDL3::SDL3)

# glm
CPMAddPackage("gh:g-truc/glm#3c18b0f")
list(APPEND LIBS glm::glm-header-only)

add_executable(CHIP8 src/main.cpp src/Chip8.cpp  "src/Chip8.h" src/Display.cpp src/Display.h)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_SOURCE_DIR}/include)


add_custom_command(
        TARGET CHIP8 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/roms"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/roms"
           COMMENT "Copying roms to output directory."
)
# imgui
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    CPMADDPACKAGE("gh:ocornut/imgui#v1.91.7")

    if(imgui_ADDED)
        target_include_directories(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR})
        target_sources(${PROJECT_NAME} PRIVATE
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        )
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})