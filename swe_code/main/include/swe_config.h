#ifndef SWE_CONFIG_H
#define SWE_CONFIG_H

#ifdef USE_OCL
#ifdef OCL_FOLDER
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif
#endif

// ======================================================================================================================== //

// ========================================= Vectorization/SIMD Relevant Definitions ====================================== //

#ifndef SIMD_LENGTH
	#define SIMD_LENGTH 8
#endif

// rounding operations 
#define ROUNDUP_SIMD(N) (((((N) - 1) / SIMD_LENGTH) + 1) * SIMD_LENGTH)
#define ROUNDDOWN_SIMD(N) (((N) / SIMD_LENGTH) * SIMD_LENGTH)

// ======================================================================================================================== //

// ============================================ DM Data Layout Relevant Definitions ======================================= //

// define functions to get a DM and vector data linear index for a corresponding node/neighbor id and node/component id combination
// NODE_ID -> normalized compute node id (first compute node in patch has normalized id of 0), NBR_ID -> stencil neighbor id, COMP_ID -> component id
// NNODES -> number of nodes in compute patch, NNBR -> number of neighbor nodes in each stencil (padded)
#ifdef USE_SFDL
	#define DM_LIN_ID(NODE_ID, NBR_ID, NNBR) ((ROUNDDOWN_SIMD(NODE_ID) * (NNBR)) + ((NBR_ID) * SIMD_LENGTH) + ((NODE_ID) % SIMD_LENGTH))
	#define VECT_LIN_ID(NODE_ID, COMP_ID) ((ROUNDDOWN_SIMD(NODE_ID) * 3) + ((COMP_ID) * SIMD_LENGTH) + ((NODE_ID) % SIMD_LENGTH))
#else
	#define DM_LIN_ID(NODE_ID, NBR_ID, NNBR) (((NODE_ID) * (NNBR)) + (NBR_ID))
	#define VECT_LIN_ID(NODE_ID, COMP_ID) (((NODE_ID) * 3) + (COMP_ID))
#endif

// define component ids for vector data
#define X_COMP_ID 0
#define Y_COMP_ID 1
#define Z_COMP_ID 2

// ======================================================================================================================== //

// ============================================ SVM Data Layout Relevant Definitions ====================================== //

// define function to get a SVM linear index for a corresponding node/neighbor combination
// PID -> node pid in patch, SV_ID -> state variable id
#ifdef USE_CFDL
	#define SVM_LIN_ID(PID, SV_ID) ((4 * (PID)) + (SV_ID))
#else
	#define SVM_LIN_ID(PID, SV_ID) ((ROUNDDOWN_SIMD(PID) * 4) + ((SV_ID) * SIMD_LENGTH) + ((PID) % SIMD_LENGTH))
#endif
	
// define state variable ids for each state variable: u -> 0, v -> 1, w -> 2, h -> 3
#define U_SV_ID 0
#define V_SV_ID 1
#define W_SV_ID 2
#define H_SV_ID 3

// ======================================================================================================================== //

// ============================================= Other Global Definitions ================================================= //

// maximum file path size
#define MAX_FILE_PATH_SIZE 200

// floating point type and related definitions

typedef double  fType_Input; // type of precomputed inputs to model 

// Sets 32 bit execution (or not)

#define RUN_32BIT 
//#define RUN_64BIT

#ifdef RUN_32BIT

typedef float    fType;      // type used by model calculations
#define TOLERANCE 4.0e-2
#ifdef USE_MPI
	#define MPI_FTYPE MPI_FLOAT
#endif

#else

typedef double    fType;      // type used by model calculations
#define TOLERANCE 1.0e-9
#ifdef USE_MPI
	#define MPI_FTYPE MPI_DOUBLE
#endif


#endif

// output verification error tolerance




// ======================================================================================================================== //

// =============================================== Model Struct Definitions =============================================== //

// runtime simulation and model parameterizations
typedef struct sim_params_struct {
	char inputFile[MAX_FILE_PATH_SIZE];		// input file path
	int nsteps;			// number of simulation time steps to run for
	int nattempts;
    int num_compute_units;
    int USE_NETCDF; // 1 to use NETCDF IO, 0 for binary IO

	#ifdef USE_HIST
	char outputFile[MAX_FILE_PATH_SIZE];	// ouptut file path
	int history_interval;					// number of timesteps between history writes to output file
	#endif

	// node ordering and layout options
	int USE_RCM;

    // OpenCL
    int OCL;

} sim_params_struct;


// global static model data (GSMD) struct that holds all time-independent data for the simulation
typedef struct GSMD_struct {

	// --------------- Nodeset and Relevant Nodepoint Data ---------------------------------- //

	int Nnodes, padded_Nnodes;	// total number of nodes and padded values

	// static node point data
	fType* x;		// x-coordinates
	fType* y;		// y-coordinates
	fType* z;		// z-coordinates

	fType* f;	 	// Coriolis force

	fType g;		// gravitational constant (m/s^2)
	fType a;		// mean radius of the earth (constant in meters)
	fType gh0;		// Initial condition for the geopotential field
	fType dt;		// time step size

	fType* ghm;		// the profile of the mountain
	fType* gradghm; // gradient of the mountains profile

	// surface projection constants variables for arbitrary cartesian vectors
	fType* p_u;
	fType* p_v;
	fType* p_w;

	// ------------------- RBF-FD Differentiation Matrix Data ------------------------------- //

	int Nnbr, padded_Nnbr;	// number of rbf stencil neighbors and padded neighbors

	fType gamma;	// hyperviscosity coefficient

	int* idx;	// Nnodes x Nnbr
	fType* Dx;	// Nnodes x Nnbr
	fType* Dy;	// Nnodes x Nnbr
	fType* Dz;	// Nnodes x Nnbr
	fType* L;	// Nnodes x Nnbr
	
} GSMD_struct;

// patch decomposition struct containing MPI patch partitioning information
typedef struct patch_struct {

	// halo and compute start/end ids (global) for a MPI patch
	int halo_gid_s;
	int halo_gid_e;
	int compute_gid_s;
	int compute_gid_e;

	// neighbor halo layer info
	int lnbr_halo_gid_e;
	int rnbr_halo_gid_s;

	// sizes
	int compute_size;
	int patch_size;

} patch_struct;

// local patch static model data (PSMD) struct that holds all time-independent data for the simulation
typedef struct PSMD_struct {

	// ---------------------------------- MPI data -------------------------------------------- //

	// neighbor rank info
	int lnbr_rank;
	int rnbr_rank;

	// local patch info
	int patch_size, compute_size, lhalo_size, rhalo_size;			// local patch total, compute and halo layer sizes
	int halo_pid_s, compute_pid_s, compute_pid_e, halo_pid_e;		// halo and compute layer start/stop node ids

	int lnbr_halo_size, rnbr_halo_size;			// neighbor halo layer sizes
	int lnbr_halo_pid_e, rnbr_halo_pid_s;		// relative to the local patch node indexing


	// ------------------------ Static Patch Nodeset and Compute Data -------------------------- //

	fType* x;		// x-coordinates
	fType* y;		// y-coordinates
	fType* z;		// z-coordinates

	fType* f;	 	// Coriolis force

	fType gh0;		// Initial condition for the geopotential field
	fType dt;		// time step size

	fType* ghm;		// the profile of the mountain
	fType* gradghm; // gradient of the mountains profile

	// surface projection constants variables for arbitrary cartesian vectors
	fType* p_u;
	fType* p_v;
	fType* p_w;

	// --------------------- RBF-FD Differentiation Matrix Data -------------------------------- //

	int Nnbr, padded_Nnbr;	// number of rbf stencil neighbors and padded neighbors

	int* idx;

	fType* Dx;
	fType* Dy;
	fType* Dz;
	fType* L;
	
} PSMD_struct;

// OpenCL buffer structures
#ifdef USE_OCL
typedef struct LPSMD_buffers {
    cl_mem idx;     // Objects are the same as the PSMD objects, but OpenCL input buffers
    
    cl_mem Dx;
    cl_mem Dy;
    cl_mem Dz;
    cl_mem L;
    
    cl_mem x;
    cl_mem y;
    cl_mem z;
    cl_mem f;
    
    cl_mem ghm;
    
    cl_mem p_u;
    cl_mem p_v;
    cl_mem p_w;
    
    cl_mem gradghm;
} LPSMD_buffers;
#endif

// ======================================================================================================================== //

typedef struct timing_struct {

	// simulation attempt number
	int attempt;

	// initialization timers
	double t_init;
    double t_ocl;

	// main timestepping timers
	double* t_main;
	double* t_eval_rhs;
	double* t_mpi;
	double* t_eval_K;
	double* t_update_D;
	double* t_update_H;

} timing_struct;

#endif
