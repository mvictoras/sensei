## Build LAMMPS

```
git clone https://github.com/lammps/lammps.git  
cd lammps/src/  
make clean-all  
make yes-KSPACE  
make yes-MOLECULE  
make yes-RIGID  
make mpi mode=lib -j4
```


