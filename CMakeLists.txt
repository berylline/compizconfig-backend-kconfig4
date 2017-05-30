if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message (SEND_ERROR "Building in the source directory is not supported.")
    message (FATAL_ERROR "Please remove the created \"CMakeCache.txt\" file, the \"CMakeFiles\" directory and create a build directory and call \"${CMAKE_COMMAND} <path to the sources>\".")
endif ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

if (CMAKE_MAJOR_VERSION GREATER 2 OR CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION GREATER 5)
cmake_policy (VERSION 2.4)
cmake_policy(SET CMP0000 OLD)
cmake_policy(SET CMP0005 OLD)
endif (CMAKE_MAJOR_VERSION GREATER 2 OR CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION GREATER 5)

project (compizconfig-backend-kconfig4)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/addons.cmake)

get_version ()

set (CMAKE_BUILD_TYPE "Debug")
add_subdirectory(src)

cf_print_configure_header ()

cf_add_package_generation ("CompizConfig KDE 4 storage backend")