if (ENABLE_VTKM)
  set(SENSEI_VTKM_COMPONENTS Base)
  if (ENABLE_VTKM_RENDERING)
    list(APPEND SENSEI_VTKM_COMPONENTS Rendering)
  endif()

  find_package(VTKm REQUIRED
    COMPONENTS ${SENSEI_VTKM_COMPONENTS}
    OPTIONAL_COMPONENTS TBB CUDA)

  add_library(sVTKm INTERFACE)
  target_link_libraries(sVTKm INTERFACE vtkm_cont)
  #target_include_directories(sVTKm SYSTEM INTERFACE ${VTKm_INCLUDE_DIRS})
  #target_compile_definitions(sVTKm INTERFACE ${VTKm_COMPILE_OPTIONS})
  install(TARGETS sVTKm EXPORT sVTKm)
  install(EXPORT sVTKm DESTINATION lib/cmake EXPORT_LINK_INTERFACE_LIBRARIES)
endif()
