cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations -g")

project(chrome_keyboard)

file(GLOB SRCS src/chrome_keyboard.cpp)

find_package(PkgConfig)
pkg_check_modules(GTK gtk+-3.0)
pkg_check_modules(ZMQ libzmq)

include_directories(${GTK_INCLUDE_DIRS} ${ZMQ_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS} ${ZMQ_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} ${ZMQ_LIBRARIES} pthread)

install(TARGETS ${PROJECT_NAME} OBJECTS DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)


add_executable(watch_keyboard src/watch_keyboard.cpp)
add_executable(chrome_keyboard_tray src/chrome_keyboard_tray.cpp)
target_link_libraries(chrome_keyboard_tray ${GTK_LIBRARIES} ${ZMQ_LIBRARIES} pthread)