#!/bin/bash
export MYLOC=`pwd`/chess.so 
rm tempF
mpirun -np $1 ./head ${*:2}
