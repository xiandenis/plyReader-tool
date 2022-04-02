cmake_minimum_required(VERSION 3.14)
project(Text)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-std=c++14)
add_compile_options(-fpermissive)
add_compile_options(-fPIC)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Setup project directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include_directories("include/")
include_directories("src/")


FILE(GLOB_RECURSE HDRS_FILES "src/*.h") 
FILE(GLOB_RECURSE SRCS_FILES "src/*.cpp")

add_library(Plyreader SHARED ${HDRS_FILES} ${SRCS_FILES} )
SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/lib)            
INSTALL(TARGETS Plyreader LIBRARY DESTINATION lib)  

add_executable(main.exe ./test/main.cpp)
target_link_libraries(main.exe Plyreader)    
