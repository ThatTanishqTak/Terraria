cmake_minimum_required(VERSION 3.8)
project(Terraria)

set(SOURCE "Src/Win32_main.cpp")

add_executable(Terraria ${SOURCE})

set_target_properties(Terraria PROPERTIES ENTRY_POINT "WinMain")
set_target_properties(Terraria PROPERTIES LINK_FLAGS "-mwindows")