#!/bin/bash
#SBATCH --job-name="625hw5"
#SBATCH --nodes=1
#SBATCH --time=00-00:10:00
#SBATCH --partition=batch.q,killable.q
#SBATCH --mem=10G

for j in 1 2 3 4 5
do 
    for i in 16 8 4 2 1
    do
        for k in 16 8 4 2 1
        do
          srun --mpi=pmi2 --time=00-00:10:00 --mem=10G --nodes=1 --ntasks-per-node=$i --partition=batch.q,killable.q ./hw5 1000000000000 $k
        done
    done
done
