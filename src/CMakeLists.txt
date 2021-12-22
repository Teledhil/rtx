project(rtx_app)

LIST (APPEND RTX_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
LIST (APPEND RTX_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
LIST (APPEND RTX_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/render)
LIST (APPEND RTX_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/shaders)

include_directories(
  ${RTX_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIRS}
  ${STB_INCLUDE_DIRS}
  ${TINYOBJLOADER_INCLUDE_DIRS}
  )

macro(glsl_to_spirv src basename)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${basename}/${src}.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/${basename}/${src}.h ${Vulkan_GLSLANG_VALIDATOR} false
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${Vulkan_GLSLANG_VALIDATOR}
        )
endmacro()


# RTX app
list(APPEND RTX_APP_SOURCES "main.cc")
list(APPEND RTX_APP_SOURCES "render/ray_tracing_extensions.cc")
# shaders
glsl_to_spirv(draw_cube.vert shaders)
list(APPEND RTX_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/shaders/draw_cube.vert.h)
glsl_to_spirv(draw_cube.frag shaders)
list(APPEND RTX_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/shaders/draw_cube.frag.h)
# Ray tracing shaders
glsl_to_spirv(raytrace.rgen shaders)
list(APPEND RTX_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/shaders/raytrace.rgen.h)
glsl_to_spirv(raytrace.rchit shaders)
list(APPEND RTX_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/shaders/raytrace.rchit.h)
glsl_to_spirv(raytrace.rmiss shaders)
list(APPEND RTX_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/shaders/raytrace.rmiss.h)
glsl_to_spirv(raytrace_shadow.rmiss shaders)
list(APPEND RTX_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/shaders/raytrace_shadow.rmiss.h)
# Dear Imgui
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui_demo.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui_draw.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui_tables.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui_widgets.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)

set(RTX_APP_NAME rtx)
add_executable(${RTX_APP_NAME} ${RTX_APP_SOURCES})
target_link_libraries(${RTX_APP_NAME} ${CMAKE_DL_LIBS})
target_link_libraries(${RTX_APP_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${RTX_APP_NAME} ${GLFW_LIBRARIES})
target_link_libraries(${RTX_APP_NAME} ${GLM_LIBRARIES})

# Inter procedural optimization
#check_ipo_supported(RESULT result)
#if(result)
#  set_property(TARGET ${RTX_APP_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
#endif()
