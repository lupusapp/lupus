cmake_minimum_required(VERSION 3.14)

project(Lupus C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(SRCS src/lupus_application.c src/lupus_headerbar.c src/lupus_profile.c src/lupus_profile_chooser.c src/resources.c src/utils.c)
set(INCLUDES include/lupus.h include/lupus_application.h include/lupus_headerbar.h include/lupus_profile.h include/lupus_profile_chooser.h include/utils.h)

add_executable(Lupus main.c ${SRCS} ${INCLUDES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})

link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

target_link_libraries(Lupus ${GTK3_LIBRARIES} ${CMAKE_SOURCE_DIR}/c-toxcore/_build/libtoxcore.so)

add_custom_command(
        TARGET Lupus
        PRE_BUILD
        COMMAND glib-compile-resources --target=src/resources.c --sourcedir=res/ui/ --generate-source res/lupus.gresource.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)