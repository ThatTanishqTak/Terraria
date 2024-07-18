cmake_minimum_required(VERSION 3.8)
project(Terraria)

set(SOURCE "Src/Win32_terraria.cpp" "Src/Terraria.cpp")

add_executable(${PROJECT_NAME} ${SOURCE})
include_directories("Include")

set_target_properties(${PROJECT_NAME} PROPERTIES ENTRY_POINT "WinMain" LINK_FLAGS "-mwindows")