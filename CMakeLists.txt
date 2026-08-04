cmake_minimum_required(VERSION 3.16)

project(Proteus_OOP_Sections_1_to_6_Complete LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project currently supports Windows only.")
endif()

if(NOT MINGW)
    message(FATAL_ERROR "Please select the MSYS2 UCRT64/MinGW 64-bit toolchain in CLion.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME}
    OOP_project.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SDL_MAIN_HANDLED
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# Detect the MSYS2/MinGW prefix from the compiler path.
# Example compiler:
# C:/msys64/ucrt64/bin/c++.exe
# Detected prefix:
# C:/msys64/ucrt64
get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(MINGW_PREFIX "${MINGW_BIN_DIR}" DIRECTORY)
file(TO_CMAKE_PATH "${MINGW_PREFIX}" MINGW_PREFIX)

set(SDL2_INCLUDE_ROOT "${MINGW_PREFIX}/include")
set(SDL2_LIBRARY_DIR "${MINGW_PREFIX}/lib")
set(SDL2_RUNTIME_DIR "${MINGW_PREFIX}/bin")

find_path(SDL2_INCLUDE_DIR
    NAMES SDL2/SDL.h
    PATHS "${SDL2_INCLUDE_ROOT}"
    NO_DEFAULT_PATH
)

find_path(SDL2_TTF_INCLUDE_DIR
    NAMES SDL2/SDL_ttf.h
    PATHS "${SDL2_INCLUDE_ROOT}"
    NO_DEFAULT_PATH
)

find_library(SDL2_LIBRARY
    NAMES SDL2
    PATHS "${SDL2_LIBRARY_DIR}"
    NO_DEFAULT_PATH
)

find_library(SDL2_TTF_LIBRARY
    NAMES SDL2_ttf
    PATHS "${SDL2_LIBRARY_DIR}"
    NO_DEFAULT_PATH
)

if(NOT SDL2_INCLUDE_DIR OR NOT SDL2_LIBRARY)
    message(FATAL_ERROR
        "SDL2 was not found under ${MINGW_PREFIX}.\n"
        "Open the MSYS2 UCRT64 terminal and install SDL2, then reload CMake."
    )
endif()

if(NOT SDL2_TTF_INCLUDE_DIR OR NOT SDL2_TTF_LIBRARY)
    message(FATAL_ERROR
        "SDL2_ttf was not found under ${MINGW_PREFIX}.\n"
        "Open the MSYS2 UCRT64 terminal and install SDL2_ttf, then reload CMake."
    )
endif()

message(STATUS "MinGW prefix: ${MINGW_PREFIX}")
message(STATUS "SDL2 library: ${SDL2_LIBRARY}")
message(STATUS "SDL2_ttf library: ${SDL2_TTF_LIBRARY}")

target_include_directories(${PROJECT_NAME} PRIVATE
    "${SDL2_INCLUDE_DIR}"
    "${SDL2_TTF_INCLUDE_DIR}"
)

# Keep SDL2_ttf before SDL2 in the link order.
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${SDL2_TTF_LIBRARY}"
    "${SDL2_LIBRARY}"
    comdlg32
)

if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)

# Copy SDL2, SDL2_ttf and their exact runtime dependencies beside the EXE.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DEXECUTABLE_PATH=$<TARGET_FILE:${PROJECT_NAME}>"
        "-DDESTINATION_DIR=$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        "-DRUNTIME_SEARCH_DIR=${SDL2_RUNTIME_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake"
    VERBATIM
    COMMENT "Copying runtime DLL dependencies beside the executable"
)
