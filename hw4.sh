#!/bin/bash
#SBATCH --job-name="625hw4"
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --time=00-23:59:00
#SBATCH --partition=ksu-chem-cmaikens.q,batch.q,ksu-chem-mri.q,killable.q
#SBATCH --constraint=moles
#SBATCH --mem=20G


for j in 1 2 3 4 5
do 
    for i in 16 8 4 2
    do
      srun --mpi=pmi2 --time=00-23:00:00 --mem=20G --constraint=moles --nodes=1 --ntasks-per-node=$i --partition=ksu-chem-cmaikens.q,batch.q,ksu-chem-mri.q,killable.q ./hw4_1_working.x 50000 1000000
      srun --mpi=pmi2 --time=00-23:00:00 --mem=20G --constraint=moles --nodes=1 --ntasks-per-node=$i --partition=ksu-chem-cmaikens.q,batch.q,ksu-chem-mri.q,killable.q ./hw4_2_working.x  50000 1000000
      srun --mpi=pmi2 --time=00-23:00:00 --mem=20G --constraint=moles --nodes=1 --ntasks-per-node=$i --partition=ksu-chem-cmaikens.q,batch.q,ksu-chem-mri.q,killable.q ./hw4_3_working.x  50000 1000000
    done
done
