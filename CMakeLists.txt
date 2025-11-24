# CMakeLists.txt for The Wandering Earth
cmake_minimum_required(VERSION 3.10)
project(TheWanderingEarth LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/external)

file(GLOB_RECURSE SRC_SRC
	${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB_RECURSE SRC_EXTERNAL_CPP
	${CMAKE_SOURCE_DIR}/external/*.cpp
)
file(GLOB_RECURSE SRC_EXTERNAL_C
	${CMAKE_SOURCE_DIR}/external/*.c
)

set(COMMON_SOURCES ${SRC_SRC} ${SRC_EXTERNAL_CPP})

# Main game executable
set(MAIN_SRC ${CMAKE_SOURCE_DIR}/main.cpp)
add_executable(TheWanderingEarth ${COMMON_SOURCES} ${MAIN_SRC})

if (WIN32)
	target_link_libraries(TheWanderingEarth PRIVATE opengl32 glu32 gdi32 imm32 dsound)
endif()

# Fix compilation issues with MinGW GCC for external files and type conflicts
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(TheWanderingEarth PRIVATE 
		-Wno-error=pointer-arith
		-Wno-pointer-arith
		-fpermissive
		-Wno-error
	)
endif()

set(SRC_YSGL "${CMAKE_SOURCE_DIR}/external/ysglfontdata.c")
if(EXISTS ${SRC_YSGL})
	set_source_files_properties(${SRC_YSGL} PROPERTIES LANGUAGE C)
	target_sources(TheWanderingEarth PRIVATE ${SRC_YSGL})
endif()

# Test sources
set(play_pause_test_SRC ${CMAKE_SOURCE_DIR}/test/ui/play_pause_test.cpp)
set(gameover_menu_test_SRC ${CMAKE_SOURCE_DIR}/test/ui/gameover_menu_test.cpp)

add_executable(play_pause_test ${COMMON_SOURCES} ${play_pause_test_SRC})
add_executable(gameover_menu_test ${COMMON_SOURCES} ${gameover_menu_test_SRC})

# Test executables
if (WIN32)
	target_link_libraries(play_pause_test PRIVATE opengl32 glu32 gdi32)
	target_link_libraries(gameover_menu_test PRIVATE opengl32 glu32 gdi32)
endif()

set(SRC_YSGL "${CMAKE_SOURCE_DIR}/external/ysglfontdata.c")
if(EXISTS ${SRC_YSGL})
	set_source_files_properties(${SRC_YSGL} PROPERTIES LANGUAGE C)
	target_sources(play_pause_test PRIVATE ${SRC_YSGL})
	target_sources(gameover_menu_test PRIVATE ${SRC_YSGL})
endif()

# Test Monster AI
# Monster AI test
set(monster_ai_test_SRC ${CMAKE_SOURCE_DIR}/test/MonsterAI/MonsterAI_test.cpp)

add_executable(MonsterAI_test
  ${monster_ai_test_SRC}
  ${CMAKE_SOURCE_DIR}/src/entities/MonsterSystem.cpp
)

target_include_directories(MonsterAI_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
