if(ENABLE_ADIOS)
  find_package(ADIOS REQUIRED)

  add_library(sADIOS INTERFACE)
  target_link_libraries(sADIOS INTERFACE ${ADIOS_LIBRARIES})
  target_include_directories(sADIOS SYSTEM INTERFACE ${ADIOS_INCLUDE_DIRS})
  if(ADIOS_DEFINITIONS)
    target_compile_definitions(sADIOS INTERFACE ${ADIOS_DEFINITIONS})
  endif()
  install(TARGETS sADIOS EXPORT sADIOS)
  install(EXPORT sADIOS DESTINATION lib/cmake EXPORT_LINK_INTERFACE_LIBRARIES)
endif()