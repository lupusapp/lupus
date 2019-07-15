cmake_minimum_required(VERSION 3.14)

project(Lupus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(SRCS
        src/lupus_application.c src/lupus_headerbar.c src/lupus_profile.c
        src/lupus_profile_chooser.c src/resources.c src/utils.c
        src/lupus_profile_chooser_password_dialog.c src/lupus_main.c
        src/lupus_editable_label.c
        )
set(INCLUDES
        include/lupus.h include/lupus_application.h include/lupus_headerbar.h
        include/lupus_profile.h include/lupus_profile_chooser.h include/utils.h
        include/lupus_profile_chooser_password_dialog.h include/lupus_main.h
        include/lupus_editable_label.h
        )

add_executable(Lupus main.c ${SRCS} ${INCLUDES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})

link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

include_directories(c-toxcore/)
target_link_libraries(Lupus ${GTK3_LIBRARIES} ${CMAKE_SOURCE_DIR}/c-toxcore/_build/libtoxcore.so)

add_custom_command(
        TARGET Lupus
        PRE_BUILD
        COMMAND glib-compile-resources --target=src/resources.c --sourcedir=res/ --generate-source res/lupus.gresource.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)