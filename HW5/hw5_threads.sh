#!/bin/bash
#SBATCH --job-name="625hw5"
#SBATCH --time=00-00:00:10
#SBATCH --partition=batch.q,killable.q
##SBATCH --mem=3G

#SBATCH -N 1
#SBATCH -n 1
#SBATCH -c 2

module purge
module load OpenMPI

if [ -n "$SLURM_CPUS_PER_TASK" ]; then
  omp_threads=$SLURM_CPUS_PER_TASK
else
  omp_threads=1
fi
export OMP_NUM_THREADS=$omp_threads

#srun --cpu_bind=cores ./hw5 100000000000 $omp_threads
srun --cpu_bind=cores ./hw5 100 $omp_threads
