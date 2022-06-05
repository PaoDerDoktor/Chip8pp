CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(Chip8pp)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

ADD_SUBDIRECTORY(submodules/glfw)
ADD_SUBDIRECTORY(submodules/spdlog)
ADD_SUBDIRECTORY(submodules/glm)

INCLUDE_DIRECTORIES(include)

ADD_EXECUTABLE(chip8pp src/main.cpp
                       src/Chip8.cpp
                       src/gl.c)

ADD_EXECUTABLE(test-main src/test-main.cpp)

TARGET_LINK_LIBRARIES(chip8pp glfw spdlog::spdlog glm)
TARGET_LINK_LIBRARIES(test-main glm)

FILE(COPY resources DESTINATION .)
