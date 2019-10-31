#!/bin/bash

export LD_LIBRARY_PATH=LD_LIBRARY_PATH:/Users/srizzi/Documents/LAMMPSsensei3/install/vtk/lib

mpirun -n 2 ./lammpsDriver in.rhodo -n 5 -sensei config.xml

