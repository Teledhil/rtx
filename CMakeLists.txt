cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
Project(rtx)

cmake_policy(SET CMP0069 NEW)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(CMakeCompilerSettings)
include(Utils)

# CMake useful variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

########################
# EXTERNAL DEPENDENCIES
########################

# THREADS
#########
find_package(Threads REQUIRED)
set(CMAKE_THREAD_LIBS_INIT ${CMAKE_THREAD_LIBS_INIT} -pthread) # Ubuntu needs this :/

# VULKAN
########
find_package(Vulkan
  REQUIRED)
find_program(Vulkan_GLSLANG_VALIDATOR
  NAMES glslangValidator
  HINTS ENV VULKAN_SDK
  PATH_SUFFIXES bin
  REQUIRED)
message(STATUS "glslangValidator found: ${Vulkan_GLSLANG_VALIDATOR}")

# GLFW
######
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dep/glfw)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} EXCLUDE_FROM_ALL)
set(GLFW_INCLUDE_DIRS ${GLFW_DIR}/include)
set(GLFW_LIBRARIES "glfw")

# IMGUI
#######
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dep/imgui)
list(APPEND IMGUI_INCLUDE_DIRS ${IMGUI_DIR})
list(APPEND IMGUI_INCLUDE_DIRS ${IMGUI_DIR}/examples)

# TINYOBJLOADER
###############
set(TINYOBJLOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dep/tinyobjloader)
list(APPEND TINYOBJLOADER_INCLUDE_DIRS ${TINYOBJLOADER_DIR})

# STB_IMAGE
###########
set(STB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dep/stb)
list(APPEND STB_INCLUDE_DIRS ${STB_DIR})

# GLM
#####
set(glm_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dep/glm/cmake/glm)
find_package(glm REQUIRED)


########
# App
########
add_subdirectory(src)

#debug_environment()
