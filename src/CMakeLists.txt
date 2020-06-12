project(rtx_app)

LIST (APPEND RTX_INCLUDE DIRS ${CMAKE_CURRENT_SOURCE_DIR})
LIST (APPEND RTX_INCLUDE DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
LIST (APPEND RTX_INCLUDE DIRS ${CMAKE_CURRENT_SOURCE_DIR}/render)

include_directories(
  ${RTX_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIRS}
  )

# RTX app
list(APPEND RTX_APP_SOURCES "main.cc")
# Dear Imgui
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui_draw.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/examples/imgui_impl_vulkan.cpp)
list(APPEND RTX_APP_SOURCES ${IMGUI_DIR}/imgui_widgets.cpp)

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
