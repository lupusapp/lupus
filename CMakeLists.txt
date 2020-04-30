cmake_minimum_required(VERSION 3.14)

project(Lupus C)

set(CMAKE_C_STANDARD 11)

option(CLANG_TIDY_CHECK "run clang-tidy" OFF)
find_program(CLANG_TIDY_EXE NAMES "clang-tidy" PATHS "/usr/lib/llvm/*/bin")
if (CLANG_TIDY_CHECK AND CLANG_TIDY_EXE)
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}" -header-filter="./include/*.h$" -checks=* -warnings-as-errors=*)
endif ()

file(GLOB SRCS src/*.c)
file(GLOB INCS include/*.h)

add_executable(Lupus main.c ${SRCS} ${INCS})

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(LIBSODIUM libsodium REQUIRED)
pkg_check_modules(TOXCORE toxcore REQUIRED)

include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${LIBSODIUM_INCLUDE_DIRS})
include_directories(${TOXCORE_INCLUDE_DIRS})

link_directories(${GTK3_LIBRARY_DIRS})
link_directories(${LIBSODIUM_LIBRARY_DIRS})
link_directories(${TOXCORE_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})
add_definitions(${LIBSODIUM_CFLAGS_OTHER})
add_definitions(${TOXCORE_CFLAGS_OTHER})

target_link_libraries(Lupus ${GTK3_LIBRARIES} ${LIBSODIUM_LIBRARIES} ${TOXCORE_LIBRARIES})

add_custom_command(
        TARGET Lupus
        PRE_BUILD
        COMMAND glib-compile-resources --target=src/resources.c --sourcedir=res/ --generate-source res/lupus.gresource.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)