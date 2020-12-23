find_package(MPI)

if (NOT MPI_C_FOUND)
  message(FATAL_ERROR "Failed to locate MPI C libraries and headers")
endif()

# MPI to use extern "C" when including headers
add_definitions(-DOMPI_SKIP_MPICXX=1 -DMPICH_SKIP_MPICXX=1)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
# interface libarary for use elsewhere in the project

#target_link_libraries(sMPI INTERFACE ${MPI_C_LIBRARIES})

#install(TARGETS sMPI EXPORT sMPI)
#install(EXPORT sMPI DESTINATION lib/cmake EXPORT_LINK_INTERFACE_LIBRARIES)
