cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_CXX_STANDARD 17)

project(chrome_keyboard)

file(GLOB SRCS src/chrome_keyboard.c)

add_executable(${PROJECT_NAME} ${SRCS})

install(TARGETS ${PROJECT_NAME} OBJECTS DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)


add_executable(watch_keyboard src/watch_keyboard.cpp)