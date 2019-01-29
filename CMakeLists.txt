cmake_minimum_required(VERSION 3.13)
project(plainNES)

#Location of Boost and SDL2
set(BOOST_ROOT "C:/dev_libs/boost")
set(SDL2_PATH "C:/dev_libs/SDL2")
set(SDL2_TTF_PATH "C:/dev_libs/SDL2")

#Location of FindSDL2.cmake files
set(CMAKE_MODULE_PATH cmake)

#Needed to properly find Boost libraries
set(Boost_ARCHITECTURE "-x32")

find_package(Boost COMPONENTS chrono program_options REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})

#Use bin folder for output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(plainNES "")

target_include_directories(plainNES PRIVATE src)
target_sources(plainNES PRIVATE
                src/apu.cpp
                src/cpu.cpp
                src/gamepak.cpp
                src/gui.cpp
                src/io.cpp
                src/main.cpp
                src/nes.cpp
                src/ppu.cpp
                src/render.cpp
                src/utils.cpp
                src/Mapper/mapper.cpp
                src/Mapper/mapper0.cpp
                src/Mapper/mapper1.cpp
                src/Mapper/mapper3.cpp
               )

target_link_libraries(plainNES mingw32 SDL2 SDL2main ${SDL2_TTF_LIBRARIES} ${Boost_LIBRARIES})

#Copy over Resources folder into bin
add_custom_command(TARGET plainNES POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Resources $<TARGET_FILE_DIR:plainNES>)