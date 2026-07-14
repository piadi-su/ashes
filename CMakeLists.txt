cmake_minimum_required(VERSION 3.16)

project(ash C)

add_executable(ash 
	src/main.c
	src/bar.c
)

target_include_directories(ash PRIVATE
    /usr/include/freetype2
    /usr/include/fontconfig
    /usr/include
)

target_compile_options(ash PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(ash PRIVATE
    X11
    Xft
    fontconfig
    freetype
    m
    Xrender
)

# ===== INSTALL RULE =====
install(TARGETS ash
    RUNTIME DESTINATION bin
)

