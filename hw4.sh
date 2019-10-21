#!/bin/bash

for i in 16 8 4 2
do
    ./hw4_1_working.x -np $i 50000 1000000
    ./hw4_2_working.x -np $i 50000 1000000
    ./hw4_3_working.x -np $i 50000 1000000
end
