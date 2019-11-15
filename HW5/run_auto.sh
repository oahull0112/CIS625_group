for i in 16 8 4 2 1
do
    for k in 16
    do
      sbatch -N $k -n $k -c $i hw5_threads.sh 
    done
done
