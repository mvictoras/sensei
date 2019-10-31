## Building LAMMPS

The following lines will build LAMMPS as a library:

```
git clone https://github.com/lammps/lammps.git  
cd lammps/src/  
make clean-all  
make yes-KSPACE  
make yes-MOLECULE  
make yes-RIGID  
make mpi mode=lib -j4
```

## Enabling LAMMPS miniapp in SENSEI

Add the following to your cmake options:

```
-DENABLE_LAMMPS=ON
-DLAMMPS_DIR=<root of lammps dir>
```

## Running the LAMMPS driver miniapp

This is an example shell script to run LAMMPS instrumented with SENSEI:

```
export LD_LIBRARY_PATH=LD_LIBRARY_PATH:"path to libraries needed by the binary"
mpirun -n 2 ./lammpsDriver in.rhodo -n 5 -sensei config.xml
```

where:

```
-n 2: run on two MPI ranks
in.rhodo: LAMMPS input file
-n 5: run for five iterations
config.xml: XML file for the SENSEI configurable analysis adaptor
```
