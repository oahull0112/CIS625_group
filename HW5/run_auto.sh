for i in 1 2 4 8 16
do
    for k in 1 2 4
    do
      sbatch -N $k -n $k -c $i hw5_threads.sh 
    done
done
