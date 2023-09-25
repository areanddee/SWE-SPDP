#!/bin/bash -l
### Job Name

#PBS -N oacc_job

### Charging account

#PBS -A UCSU0085

### Request one node of cascade lake CPUs with 36 CPU (cores) and 20 GB of memory

#PBS -l select=1:ncpus=1:ngpus=1:mem=40GB

### Specify that the GPUs will be V100s
#PBS -l gpu_type=v100

### Allow job to run up to 20 minutes

#PBS -l walltime=20:00
### Route the job to the casper queue

#PBS -q casper
###Or directly to development queue
#PBS -q gpudev

### Join output and error streams into single file

#PBS -j oe

### Provide CUDA runtime libraries
### module load cuda

export KMP_AFFINITY=disabled  #granularity=core,balanced
#export SWE_NODES=2562
#export SWE_NODES=10242
#export SWE_NODES=40962
#export SWE_NODES=163842
export SWE_NODES=655362
##export SWE_NSTEPS=100
export SWE_NSTEPS=100
export SWE_HIST_INT=100
##export SWE_HIST_INT=10000
export SWE_NATTEMPTS=10
export SWE_USE_RCM=1
export SWE_USE_NETCDF=0
export SWE_USE_OCL=0
export SWE_NUM_COMPUTE_UNITS=1
export SWE_OUTPUT_NAME="output"

export SWE_TOP_DIR=../..
export SWE_INPUT_FILE=$SWE_TOP_DIR/input_files/icos"$SWE_NODES"_tc5_input.bin
export SWE_RUN_DIR=$SWE_TOP_DIR/run/OMP
export SWE_OUTPUT_DIR=$SWE_RUN_DIR/$SWE_OUTPUT_NAME
mkdir -p $SWE_OUTPUT_DIR
export SWE_RUN_DIR=$SWE_TOP_DIR/run/oacc/

for Layout in sfdl; do
export NumSubdevices=1
echo "Run script name: runOACC.sh" >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
while [ $NumSubdevices -le 1 ]; do
echo " " > "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
echo "NumSubDevices= $NumSubDevices SWE_NATTEMPTS= $SWE_NATTEMPTS" >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt

if [ $NumSubdevices -gt 30 ]; then
export SWE_NATTEMPTS=30
elif [ $NumSubdevices -gt 16 ]; then
export SWE_NATTEMPTS=20
else
export SWE_NATTEMPTS=10
##export SWE_NATTEMPTS=10
fi

$SWE_RUN_DIR/swe_"$Layout".exe >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt

export NumSubdevices=$[$NumSubdevices+1]
done
done
echo "DONE!"
