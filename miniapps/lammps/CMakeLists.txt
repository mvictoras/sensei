if (NOT LAMMPS_DIR)
  message(FATAL_ERROR "To build LAMMPS driver you must set -DLAMMPS_DIR=<root of lammps dir>")
endif()

include_directories(${LAMMPS_DIR}/src)

set(sources lammpsDriver.cxx)

set(libs 
    ${LAMMPS_DIR}/src/liblammps_mpi.a
)

if(ENABLE_SENSEI)
  list(APPEND sources
    lammpsDataAdaptor.h
    lammpsDataAdaptor.cxx
    lammpsBridge.h
    lammpsBridge.cxx
  )
  list(APPEND libs sensei)
endif()

add_executable(lammpsDriver ${sources})

target_link_libraries(lammpsDriver ${libs})

install(TARGETS lammpsDriver
  RUNTIME DESTINATION bin
)



