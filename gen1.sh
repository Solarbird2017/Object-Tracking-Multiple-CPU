#!/bin/bash

#BATCH --job-name pf
#SBATCH --nodes 2
#SBATCH --ntasks 16
#SBATCH --ntasks-per-node 8
#SBATCH –mem-per-cpu= 1GB
#SBATCH --partition partedu

cd $SLURM_SUBMIT_DIR
echo 0 > cap

mpirun -np 16 ./gen.out 20 0.2 430 2 1 8 2 1 14 1
mpirun -np 16 ./gen.out 20 0.2 430 12 2 14 8 1 9 1
mpirun -np 16 ./gen.out 20 0.2 430 32 2 36 8 1 9 1




