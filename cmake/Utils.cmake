# Print some platform info. This function only works well when placed after a
# project() call.
function(info_platform)
  message("CMAKE_SYSTEM: ${CMAKE_SYSTEM}")
  message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
  message("CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
  message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
  message("CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")
endfunction(info_platform)

function(debug_environment)
  info_platform()
  set(CMAKE_VERBOSE_MAKEFILE on )
  get_cmake_property(_variableNames VARIABLES)
  list (SORT _variableNames)
  foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endfunction(debug_environment)
