cmake_minimum_required(VERSION 3.14)

project(Lupus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_CLANG_TIDY /usr/lib/llvm/9/bin/clang-tidy; # TODO: not portable
        -header-filter="./include/*.h$";
        -checks=*;
        -warnings-as-errors=*;)

set(SRCS main.c src/lupus_application.c src/lupus_profilechooser.c src/resources.c src/lupus_main.c src/lupus_mainheaderbar.c src/lupus_editablelabel.c src/lupus_mainfriend.c src/lupus_wrapper.c src/lupus_wrapperfriend.c src/lupus_mainchat.c src/lupus_mainmessage.c)
set(INCLUDES include/lupus_application.h include/lupus.h include/lupus_profilechooser.h include/utils.h include/lupus_main.h include/lupus_editablelabel.h include/lupus_mainfriend.h include/lupus_mainheaderbar.h include/lupus_wrapper.h include/lupus_wrapperfriend.h include/lupus_mainchat.h include/lupus_mainmessage.h)

add_executable(Lupus main.c ${SRCS} ${INCLUDES})

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