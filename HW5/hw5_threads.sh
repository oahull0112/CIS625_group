#!/bin/bash
#SBATCH --job-name="625hw5"
#SBATCH --time=00-00:30:00
#SBATCH --constraint=elves
#SBATCH --mem=1G

##SBATCH -N 1
##SBATCH -n 1
##SBATCH -c 2
# Number of nodes, number of MPI tasks, and cores per MPI task are being specified
# in the feeder script run_auto.sh

module purge
module load OpenMPI

if [ -n "$SLURM_CPUS_PER_TASK" ]; then
  omp_threads=$SLURM_CPUS_PER_TASK
else
  omp_threads=1
fi
export OMP_NUM_THREADS=$omp_threads

for j in 1 2 3 4 5
do
  srun --cpu_bind=cores ./hw5_fix_v2 100000000000 $omp_threads
done
