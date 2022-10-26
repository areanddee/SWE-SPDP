#!/bin/bash -l

### Job Name

#PBS -N omp_job

### Charging account

#PBS -A UCSU0085

### Request one node of cascade lake CPUs with 36 CPU (cores) and 20 GB of memory

#PBS -l select=1:cpu_type=cascadelake:ncpus=36:ompthreads=36:mem=20GB

### Allow job to run up to 10 minutes

#PBS -l walltime=20:00

### Route the job to the casper queue

#PBS -q casper

### Join output and error streams into single file

#PBS -j oe

export TMPDIR=/glade/scratch/$USER/temp
mkdir -p $TMPDIR

export KMP_AFFINITY=scatter  #granularity=core,balanced
export OMP_SCHEDULE=static

#export SWE_NODES=2562
#export SWE_NODES=10242
#export SWE_NODES=40962
export SWE_NODES=163842
#export SWE_NODES=655362
export SWE_NSTEPS=100
export SWE_HIST_INT=1000
export SWE_NATTEMPTS=20
export SWE_USE_RCM=1
export SWE_USE_NETCDF=0
export SWE_USE_OCL=0
export SWE_NUM_COMPUTE_UNITS=36
export SWE_OUTPUT_NAME="output"
export SWE_TOP_DIR=../..
export SWE_INPUT_FILE=$SWE_TOP_DIR/input_files/icos"$SWE_NODES"_tc5_input.bin
export SWE_RUN_DIR=$SWE_TOP_DIR/run/OMP
export SWE_OUTPUT_DIR=$SWE_RUN_DIR/$SWE_OUTPUT_NAME
mkdir -p $SWE_OUTPUT_DIR

#export LD_LIBRARY_PATH=lib64:/opt/slurm/latest/lib64:/usr/local/openmpi/3.1.0/lib:$LD_LIBRARY_PATH:$LD_LIBRARY_PATH
#source /usr/local/intel/2018u3/bin/compilervars.sh intel64
#export PATH=/opt/slurm/latest/bin:/usr/local/openmpi/3.1.0/bin/:$PATH

for Layout in cfdl sfdl cfdl_sfdl default; do
echo "Begin OMP tests... " > "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
echo $SWE_NODES " Nodes" >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt

export NumSubdevices=2
export MAX_THREADS=$OMP_NUM_THREADS

while [ $NumSubdevices -le $MAX_THREADS ]; do

export OMP_NUM_THREADS=$NumSubdevices

if [ $NumSubdevices -gt 30 ]; then
export SWE_NATTEMPTS=30
elif [ $NumSubdevices -gt 16 ]; then
export SWE_NATTEMPTS=20
else
export SWE_NATTEMPTS=10
fi
echo " " >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
echo "OMP_NUM_THREADS=" $OMP_NUM_THREADS " SWE_NATTEMPTS=" $SWE_NATTEMPTS >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt

# RUN THE TEST

$SWE_RUN_DIR/swe_"$Layout".exe >> "$SWE_RUN_DIR"/output/"$SWE_OUTPUT_NAME"_"$Layout".txt
export NumSubdevices=$[$NumSubdevices+2]

done

done
echo "DONE!"
qstat -f $PBS_JOBID
