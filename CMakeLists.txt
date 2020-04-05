cmake_minimum_required(VERSION 3.13)
project(plainNES)

# Dependencies
set(CATCH2_PATH "${CMAKE_SOURCE_DIR}/Catch2/single_include")

#Use bin folder for output and lib folder for library output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#Location of FindSDL2.cmake files
set(CMAKE_MODULE_PATH cmake)

#Needed to properly find Boost libraries
set(Boost_ARCHITECTURE "-x32")

find_package(Boost COMPONENTS chrono program_options REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIR})

#GLAD
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/glad)
add_library("glad" "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")
include_directories("${GLAD_DIR}/include")

set(GLM_DIR "C:/dev_libs/GLM")
include_directories("${GLM_DIR}/include")

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)

#Setup Executables
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")

#Using NES library which is shared between plainNES and unit tests
add_library(NES STATIC)
target_sources(NES PRIVATE
                src/apu.cpp
                src/cpu.cpp
                src/gamepak.cpp
                src/io.cpp
                src/nes.cpp
                src/ppu.cpp
                src/utils.cpp
                src/Mapper/mapper.cpp
                src/Mapper/mapper0.cpp
                src/Mapper/mapper1.cpp
                src/Mapper/mapper2.cpp
                src/Mapper/mapper3.cpp
                src/Mapper/mapper4.cpp
               )
target_include_directories(NES PRIVATE src)

#Main executable
add_executable(plainNES
                src/display.cpp
                src/emulator.cpp
                src/gui.cpp
                src/main.cpp
                src/render.cpp
                src/shader.cpp
                src/imgui/imgui.cpp
                src/imgui/imgui_draw.cpp
                src/imgui/imgui_widgets.cpp
                src/imgui/imgui_impl_opengl3.cpp
                src/imgui/imgui_impl_sdl.cpp
                )
target_include_directories(plainNES PRIVATE src)

#Unit test executable
add_executable(ROMtests test/romtests.cpp)
target_include_directories(ROMtests PRIVATE src)
target_include_directories(ROMtests PRIVATE ${CATCH2_PATH})
set_target_properties(ROMtests PROPERTIES RUNTIME_OUTPUT_DIRECTORY test)

#Libraries for both executables
#Unit tests not using GUI
IF (WIN32)
  set(WINDOWS_LIBS mingw32 comdlg32)
ENDIF()
target_link_libraries(plainNES NES SDL2 SDL2main ${SDL2_TTF_LIBRARIES} ${Boost_LIBRARIES} "glad" ${OPENGL_gl_LIBRARY} ${CMAKE_DL_LIBS} ${WINDOWS_LIBS})
target_link_libraries(ROMtests NES ${WINDOWS_LIBS})

#Copy over Resources folder into bin
add_custom_command(TARGET plainNES POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Resources $<TARGET_FILE_DIR:plainNES>)