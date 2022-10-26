------------------------------------------------------------------------------------------------------------------

Radial Basis Functions with Finite Differencing for Shallow Water Equations

Last update: Richelle Streater, September 7, 2018

------------------------------------------------------------------------------------------------------------------

Getting started

Once all folders are on device, navigate to top directory: SWE-SPDP/

Serial/Gnu on personal device (generic gcc compile):
Compile with the following command: . ./compile_default.sh
Run with the following command: . ./run/openCL/run.sh
Expected output: output_cfdl.txt, output_sfdl.txt, output_cfdl_sfdl.txt, output_default.txt in run/openCL/output/

Serial/on personal device/serial Intel (generic icc compile):
Set OPENMP=0 in arch/intel/config.swe before compiling
Compile with the following command: . ./compile_intel.sh
Run with the following command: . ./run/OMP/run.sh
Expected output: output_cfdl.txt, output_sfdl.txt, output_cfdl_sfdl.txt, output_default.txt in run/openCL/output/

------------------------------------------------------------------------------------------------------------------

Requirements

To run with OpenMP (intel only):
--> Set OPENMP=1 OPENACC = 0 in arch/intel/config.swe before compiling
--> Set OMP_NUM_THREADS in run script

To run on the Casper GPU system with OpenACC:
--> Set OPENACC = 1; OPENMP = 0 in arch/volta/config.swe (There is an arch/pascal/config.swe but is not tested)
Compile with the following command: ./compile_casper.sh 
--> Run with: ./run/openacc/
--> 

To run with MPI: (NOT RECENTLY TESTED)
--> Set MPI=1  before compiling
--> Change LD_LIBRARY_PATH and PATH in run/hpcl/runMP.sh or run/hpcl/runCL.sh if necessary

To use NetCDF: (NOT RECENTLY TESTED)
--> Set NCIO=1 in config.swe before compiling
--> Set SWE_USE_NETCDF=1 in run script and set SWE_INPUT_FILE to a .nc file
--> Change NETCDF variable in include.mk if necessary

To MPI run with Intel Compiler:
--> Set MPICC=mpiicc and CC=icc in config.swe
--> Load icc module before compiling if necessary
--> Change LD_LIBRARY_PATH and PATH in run/hpcl/runMP.sh or run/hpcl/runCL.sh if necessary

------------------------------------------------------------------------------------------------------------------

Directory structure

Top Directory Folders:

./arch: Contains configuration parameters pascal, volta GPU testing and for general 
gnu or intel setup

./input_files: Contains all binary/netcdf input files for code (NOT INCLUDED due to file size)

./read_output_file: Contains code to read eval_rhs values from output files (NOT TESTED recently)

./run: Contains run scripts for OMP, openacc, generic gnu/intel serial testing in OpenCL (confusing for historical reasons)

./swe_code: Contains all c/cl code

------------------------------------------------------------------------------------------------------------------

swe_code folder structure:

./swe_code/io:
--> input.c:  Reads input files, either with binary or NetCDF format, and fills all differentiation matrices, 
state variable matrices, ordering, and constants
--> nc2bin.c: Converts to binary file from .nc format (not called by main function)

./swe_code/layout:
--> layout.c: Calls padding/reordering functions for differentiation matrices/state variable matrices
--> matrix_transformations.c: Functions to pad matrices (to allow for tiling/vectorizing) and rearrange based 
on CFDL/SFDL options

./swe_code/main:
--> main.c: Calls reading/reordering functions and calls patch initialization functions (for MPI). Declares
OpenCL objects and compiles kernels, opens device/platform, loads buffers, and sets kernel arguments for 
OpenCL. For n attempts and time steps, calls Runge-Kutta stepping function. Compares results to known array.
--> profiling.c: Use arrays of loop times to determine average time, min/max, and std dev for all operations.
Prints timing results.
--> rk4_rbffd_swe.c: Computes Runge-Kutta step with radial basis function finite differencing algorithm.
--> runtime_params.c: processes external variables set in run script.

./swe_code/mpi:
--> halos.c: Function for exchanging neighbor node information so that state variable matrix can be divided
among MPI threads
--> init_patches.c: Creates divided matrices and copies read and reordered arrays from thread 0 to other 
MPI threads.

./swe_code/ocl:
--> buffers.c: converts arrays into OpenCL buffer objects to be passed into the kernels and frees buffers
--> device_setup.c: Creates all OpenCL objects: kernels, devices, platforms, and command queues
--> RK_ocl.c: Version of rk4_rbffd_swe.c that used OpenCL and calls openCL kernels
--> kernel.cl: All Runge-Kutta step functions; vectorized along nodes and works for all layouts
--> kernel_CFDL.cl: All RK step functions; vectorized along u,v,w,h in state variable matrices. Only valid for
CFDL layout.

./swe_code/rcm:
--> rcm.c: Calls all reorder functions for Reverse CutHill-McKee ordering scheme
--> reorder_nodes: Defines mapping for Reverse CutHill-McKee ordering scheme

------------------------------------------------------------------------------------------------------------------

Setting single precision (FP32) vs double precision (FP64)...
Note added by: Richard Loft

------------------------------------------------------------------------------------------------------------------

SP/DP Precision is set in:

./swe_code/main/include/swe_config.h

via 

#define RUN_32BIT  

Commenting that line out will restore double precision.

The environment variable I suggest toggling is SWE_USE_RCM in the run.sh script:

export SWE_USE_RCM=1

It looks like RCM is turned on by default. You might change it to

export SWE_USE_RCM=0

The input files are quite large and must be provided separately. They are called icos<NODES>_tc5_input.nc, and MUST be located in a directory called  ./input_files.

