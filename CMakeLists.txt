cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

project(utfz)

include(GNUInstallDirs)

add_library(utfz STATIC utfz.cpp utfz.h)

target_include_directories(utfz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS utfz
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
