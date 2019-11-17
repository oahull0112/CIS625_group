#!/bin/bash
#SBATCH --job-name="625hw5"
#SBATCH --nodes=16
#SBATCH --time=00-23:00:00
#SBATCH --mem=10G

module purge
module load OpenMPI

for j in 1 2 3 4 5
do 
    for i in 16 8 4 2 1
    do
        for k in 16 8 4 2 1
        do
          srun --mpi=pmi2 --time=00-23:00:00 --mem=10G --nodes=$i --ntasks-per-node=1 --partition=batch.q,killable.q ./hw5 100000000000 $k
        done
    done
done
