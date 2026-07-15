cmake_minimum_required(VERSION 3.16)

project(ashes C)

add_executable(ashes 
	src/main.c
	src/bar.c
)

target_include_directories(ashes PRIVATE
    /usr/include/freetype2
    /usr/include/fontconfig
    /usr/include
)

target_compile_options(ashes PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(ashes PRIVATE
    X11
    Xft
    fontconfig
    freetype
    m
    Xrender
	Xinerama
)

# ===== INSTALL RULE =====
install(TARGETS ashes
    RUNTIME DESTINATION bin
)

