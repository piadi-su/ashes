cmake_minimum_required(VERSION 3.10)
project(ashes C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(XFT REQUIRED xft)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

find_library(XINERAMA_LIB Xinerama REQUIRED)

add_executable(ashes src/main.c src/bar.c)

target_include_directories(ashes PRIVATE 
    ${X11_INCLUDE_DIR}
    ${XFT_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
)

target_link_libraries(ashes PRIVATE 
    ${X11_LIBRARIES}
    ${XINERAMA_LIB}
    ${XFT_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
)

install(TARGETS ashes RUNTIME DESTINATION bin)
