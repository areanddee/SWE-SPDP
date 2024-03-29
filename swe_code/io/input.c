// Written by Samuel Elliott, Summer 2017. Last updated- Richelle Streater, Summer 2018.

#include <input.h>

#include <stdlib.h>
#include <stdio.h>

#define CHECK_OPEN_ERR(f, i) {if (f) { printf("Error in input.c: Could not open %s \n", i); exit(2);}}
#ifdef USE_NCIO
#include <netcdf.h>
#define CHECK_ERR(e) {if (e){ printf("Error in input.c: %s\n", nc_strerror(e)); exit(2);}}

// read simulation GSMD struct data
GSMD_struct get_GSMD_nc(char* inputFile) {
	
    GSMD_struct GSMD;
    // netcdf variable ids
    int ncid;                                           // file id
    int Nnodes_vid, a_vid, g_vid, gh0_vid, dt_vid;      // constant variable ids
    int x_vid, y_vid, z_vid, f_vid, ghm_vid;            // scalar variable ids (Nnodes)
    int pu_vid, pv_vid, pw_vid, gradghm_vid;            // vector variable ids (Nnodes x 3)

    // open input file; assign status and check if status is nonzero (print error statement)
    CHECK_OPEN_ERR(nc_open(inputFile, NC_NOWRITE, &ncid), inputFile);
	
    // read constant variables into GSMD struct
    CHECK_ERR(nc_inq_varid(ncid, "Nnodes", &Nnodes_vid));
    CHECK_ERR(nc_get_var_int(ncid, Nnodes_vid, &GSMD.Nnodes));
	
    CHECK_ERR(nc_inq_varid(ncid, "a", &a_vid));
    CHECK_ERR(nc_get_var_double(ncid, a_vid, &GSMD.a));
	
    CHECK_ERR(nc_inq_varid(ncid, "g", &g_vid));
    CHECK_ERR(nc_get_var_double(ncid, g_vid, &GSMD.g));

    CHECK_ERR(nc_inq_varid(ncid, "gh0", &gh0_vid));
    CHECK_ERR(nc_get_var_double(ncid, gh0_vid, &GSMD.gh0));

    CHECK_ERR(nc_inq_varid(ncid, "timestep", &dt_vid));
    CHECK_ERR(nc_get_var_double(ncid, dt_vid, &GSMD.dt));

    // allocate space for and read scalar/vector variables from input file
    GSMD.x = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    CHECK_ERR(nc_inq_varid(ncid, "x", &x_vid));
    CHECK_ERR(nc_get_var_double(ncid, x_vid, GSMD.x));

    GSMD.y = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    CHECK_ERR(nc_inq_varid(ncid, "y", &y_vid));
    CHECK_ERR(nc_get_var_double(ncid, y_vid, GSMD.y));

    GSMD.z = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    CHECK_ERR(nc_inq_varid(ncid, "z", &z_vid));
    CHECK_ERR(nc_get_var_double(ncid, z_vid, GSMD.z));
	
    GSMD.f = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    CHECK_ERR(nc_inq_varid(ncid, "f", &f_vid));
    CHECK_ERR(nc_get_var_double(ncid, f_vid, GSMD.f));

    GSMD.ghm = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    CHECK_ERR(nc_inq_varid(ncid, "ghm", &ghm_vid));
    CHECK_ERR(nc_get_var_double(ncid, ghm_vid, GSMD.ghm));

    GSMD.p_u = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    CHECK_ERR(nc_inq_varid(ncid, "p_u", &pu_vid));
    CHECK_ERR(nc_get_var_double(ncid, pu_vid, GSMD.p_u));

    GSMD.p_v = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    CHECK_ERR(nc_inq_varid(ncid, "p_v", &pu_vid));
    CHECK_ERR(nc_get_var_double(ncid, pu_vid, GSMD.p_v));

    GSMD.p_w = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    CHECK_ERR(nc_inq_varid(ncid, "p_w", &pu_vid));
    CHECK_ERR(nc_get_var_double(ncid, pu_vid, GSMD.p_w));

    GSMD.gradghm = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    CHECK_ERR(nc_inq_varid(ncid, "gradghm", &gradghm_vid));
    CHECK_ERR(nc_get_var_double(ncid, gradghm_vid, GSMD.gradghm));

    // Read in differentiation matrix data
	
    // netcdf ids
    int Nnbr_vid;
    int gamma_vid;
    int Dx_vid, Dy_vid, Dz_vid, L_vid, idx_vid;

    // get Nnbr
    CHECK_ERR(nc_inq_varid(ncid, "Nnbr", &Nnbr_vid));
    CHECK_ERR(nc_get_var_int(ncid, Nnbr_vid, &GSMD.Nnbr));

    // get hyperviscosity coefficient
    CHECK_ERR(nc_inq_varid(ncid, "gamma", &gamma_vid));
    CHECK_ERR(nc_get_var_double(ncid, gamma_vid, &GSMD.gamma));

    // total size of DM
    int DM_size = GSMD.Nnodes * GSMD.Nnbr;

    // read DM weights (dimensions: Nnodes x Nnbr)
    GSMD.Dx = (fType*)malloc(sizeof(fType) * DM_size);
    CHECK_ERR(nc_inq_varid(ncid, "DPx", &Dx_vid));
    CHECK_ERR(nc_get_var_double(ncid, Dx_vid, GSMD.Dx));

    GSMD.Dy = (fType*)malloc(sizeof(fType) * DM_size);
    CHECK_ERR(nc_inq_varid(ncid, "DPy", &Dy_vid));
    CHECK_ERR(nc_get_var_double(ncid, Dy_vid, GSMD.Dy));

    GSMD.Dz = (fType*)malloc(sizeof(fType) * DM_size);
    CHECK_ERR(nc_inq_varid(ncid, "DPz", &Dz_vid));
    CHECK_ERR(nc_get_var_double(ncid, Dz_vid, GSMD.Dz));
    
    GSMD.L = (fType*)malloc(sizeof(fType) * DM_size);
    CHECK_ERR(nc_inq_varid(ncid, "L", &L_vid));
    CHECK_ERR(nc_get_var_double(ncid, L_vid, GSMD.L));

    // weights are for unit sphere and need to be scaled to Earths radius/scaled by hyperviscosity coefficient
    for (int i = 0; i < DM_size; i++) {
        GSMD.Dx[i] = GSMD.Dx[i]/GSMD.a;
        GSMD.Dy[i] = GSMD.Dy[i]/GSMD.a;
        GSMD.Dz[i] = GSMD.Dz[i]/GSMD.a;
        GSMD.L[i] = GSMD.L[i]*GSMD.gamma;
    }

    // read DM stencil ids (dimensions: Nnodes x Nnbr)
    nc_inq_varid(ncid, "idx", &idx_vid);
    GSMD.idx = (int*) malloc(sizeof(int) * DM_size);
    nc_get_var_int(ncid, idx_vid, GSMD.idx);

    // 1-based indices must be shifted to 0-based indices
    for (int i = 0; i < DM_size; i++) {
        GSMD.idx[i] = GSMD.idx[i] - 1;
    }

    nc_close(ncid);
    return GSMD;
}

// read initial conditions for the state variable matrix
fType* get_ICs_nc(char* inputFile) {

    // netcdf ids
    int ncid, Nnodes_vid, H_vid;

    // open input file
    CHECK_OPEN_ERR(nc_open(inputFile,NC_NOWRITE,&ncid), inputFile);

    // get Nnodes
    int Nnodes;
    CHECK_ERR(nc_inq_varid(ncid, "Nnodes", &Nnodes_vid));
    CHECK_ERR(nc_get_var_int(ncid, Nnodes_vid, &Nnodes));
	
    // State Variable Matrix
    fType* H = (fType*) malloc(sizeof(fType) * Nnodes * 4);

    // read SVM from input file
    CHECK_ERR(nc_inq_varid(ncid, "H", &H_vid));
    CHECK_ERR(nc_get_var_double(ncid, H_vid, H));

    // close input file
    CHECK_ERR(nc_close(ncid));
    return H;
}

// read verified resulting conditions for the state variable matrix after 100 timesteps
fType* get_FCs_nc(char* inputFile) {

    // netcdf ids
    int ncid, Nnodes_vid, H_vid;

    // open input file
    CHECK_OPEN_ERR(nc_open(inputFile, NC_NOWRITE, &ncid), inputFile);

    // get Nnodes
    int Nnodes;
    CHECK_ERR(nc_inq_varid(ncid, "Nnodes", &Nnodes_vid));
    CHECK_ERR(nc_get_var_int(ncid, Nnodes_vid, &Nnodes));

    // SVM
    fType* H = (fType*) malloc(sizeof(fType)* Nnodes * 4);

    // read SVM from input file
    CHECK_ERR(nc_inq_varid(ncid, "H_100", &H_vid));
    CHECK_ERR(nc_get_var_double(ncid, H_vid, H));

    // close input file
    CHECK_ERR(nc_close(ncid));
    return H;
}
#endif

GSMD_struct get_GSMD_bin(char* inputFile) {

    GSMD_struct GSMD;

    // RDL temporary variables to enable Ftype and Ftype_Input to differ
    fType_Input* tmp;
    int i;
    fType_Input a;
    fType_Input gh0;
    fType_Input dt;
    fType_Input gamma;

    // open input file
    FILE* fp_in = fopen(inputFile, "r");
    CHECK_OPEN_ERR(!fp_in, inputFile);

    // constants/parameters

    fread((void*) &GSMD.Nnodes, sizeof(int), 1, fp_in);
    fread((void*) &GSMD.Nnbr, sizeof(int), 1, fp_in);

    fread((void*) &a, sizeof(fType_Input), 1, fp_in);
    GSMD.a = (fType)a;

    fread((void*) &gh0, sizeof(fType_Input), 1, fp_in);
    GSMD.gh0 = (fType)gh0;

    fread((void*) &dt, sizeof(fType_Input), 1, fp_in);
    GSMD.dt = (fType)dt;

	printf("Nnodes= %i Timestep =%f\n",GSMD.Nnodes,dt);

    fread((void*) &gamma, sizeof(fType_Input), 1, fp_in);
    GSMD.gamma = (fType)gamma;

    // allocate and read scalars
    //
    // RDL: new code to enable Ftype_Input and Ftype to actually differ

    tmp = (fType_Input*) malloc(sizeof(fType_Input) * GSMD.Nnodes);

    GSMD.x = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes, fp_in);
    for(i=0; i<GSMD.Nnodes; i++)
       GSMD.x[i] = (fType)tmp[i];

    GSMD.y = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes, fp_in);
    for(i=0; i<GSMD.Nnodes; i++)
       GSMD.y[i] = (fType)tmp[i];

    GSMD.z = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes, fp_in);
    for(i=0; i<GSMD.Nnodes; i++)
       GSMD.z[i] = (fType)tmp[i];

    GSMD.f = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes, fp_in);
    for(i=0; i<GSMD.Nnodes; i++)
       GSMD.f[i] = (fType)tmp[i];

    GSMD.ghm = (fType*) malloc(sizeof(fType) * GSMD.Nnodes);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes, fp_in);
    for(i=0; i<GSMD.Nnodes; i++)
       GSMD.ghm[i] = (fType)tmp[i];

    free(tmp);

    // allocate and read vectors
    //
    // RDL: new code to enable Ftype_Input and Ftype to actually differ

    tmp = (fType_Input*) malloc(sizeof(fType_Input) * GSMD.Nnodes * 3);

    GSMD.p_u  = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * 3, fp_in);
    for(i=0; i<GSMD.Nnodes*3; i++)
       GSMD.p_u[i] = (fType)tmp[i];

    GSMD.p_v  = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * 3, fp_in);
    for(i=0; i<GSMD.Nnodes*3; i++)
       GSMD.p_v[i] = (fType)tmp[i];

    GSMD.p_w  = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * 3, fp_in);
    for(i=0; i<GSMD.Nnodes*3; i++)
       GSMD.p_w[i] = (fType)tmp[i];

    GSMD.gradghm  = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * 3);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * 3, fp_in);
    for(i=0; i<GSMD.Nnodes*3; i++)
       GSMD.gradghm[i] = (fType)tmp[i];

    free(tmp);

    // allocate and read DMs
    //
    // RDL: new code to enable Ftype_Input and Ftype to actually differ

    tmp = (fType_Input*) malloc(sizeof(fType_Input) * GSMD.Nnodes * GSMD.Nnbr);

    GSMD.Dx = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * GSMD.Nnbr);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * GSMD.Nnbr, fp_in);
    for(i=0; i<GSMD.Nnodes*GSMD.Nnbr; i++)
       GSMD.Dx[i] = (fType)tmp[i];

    GSMD.Dy = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * GSMD.Nnbr);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * GSMD.Nnbr, fp_in);
    for(i=0; i<GSMD.Nnodes*GSMD.Nnbr; i++)
       GSMD.Dy[i] = (fType)tmp[i];

    GSMD.Dz = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * GSMD.Nnbr);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * GSMD.Nnbr, fp_in);
    for(i=0; i<GSMD.Nnodes*GSMD.Nnbr; i++)
       GSMD.Dz[i] = (fType)tmp[i];

    GSMD.L = (fType*) malloc(sizeof(fType) * GSMD.Nnodes * GSMD.Nnbr);
    fread((void*) tmp, sizeof(fType_Input), GSMD.Nnodes * GSMD.Nnbr, fp_in);
    for(i=0; i<GSMD.Nnodes*GSMD.Nnbr; i++)
       GSMD.L[i] = (fType)tmp[i];

    free(tmp);

    // weights are for unit sphere and need to be scaled to Earths radius/scaled by hyperviscosity coefficient
    // RDL: I've chosen to scale the DMs with the input prrecision, rather than the (presumably lower) model precision.

    for (i = 0; i < GSMD.Nnodes * GSMD.Nnbr; i++) {
        GSMD.Dx[i] = GSMD.Dx[i]/a;
        GSMD.Dy[i] = GSMD.Dy[i]/a;
        GSMD.Dz[i] = GSMD.Dz[i]/a;
        GSMD.L[i] = GSMD.L[i]*gamma;
    }

    // read DM stencil ids (dimensions: Nnodes x Nnbr)
    GSMD.idx = (int*) malloc(sizeof(int) * GSMD.Nnodes * GSMD.Nnbr);
    fread((void*) GSMD.idx, sizeof(int), GSMD.Nnodes * GSMD.Nnbr, fp_in);

    // 1-based indices must be shifted to 0-based indices
    for (i = 0; i < GSMD.Nnodes * GSMD.Nnbr; i++) {
        GSMD.idx[i] = GSMD.idx[i] - 1;
    }

    fclose(fp_in);
    return GSMD;
}

// Get Initial Conditions 

fType* get_ICs_bin(char* inputFile) {

    fType* H_init;
    long Nnodes, Nnbr;
    int temp;

    // RDL Enables fType and fType_Input to actually differ

    fType_Input *tmp;
    int i;

    // open input file
    FILE* fp_in = fopen(inputFile, "r");
    CHECK_OPEN_ERR(!fp_in, inputFile);

    // constants/parameters
    fread((void*) &temp, sizeof(int), 1, fp_in);
    Nnodes = (long) temp;
    fread((void*) &temp, sizeof(int), 1, fp_in);
    Nnbr = (long) temp;

    // calculate and position file pointer
    long offset = (sizeof(int) * (2 + (Nnodes * Nnbr))) + (sizeof(fType_Input) * (4 + (5 * Nnodes) + (4 * Nnodes * 3) + (4 * Nnodes * Nnbr)));
    fseek(fp_in, offset, SEEK_SET);

    // allocate/read H_init
    // RDL Enables fType and fType_Input to actually differ

    tmp = (fType_Input*) malloc(sizeof(fType_Input) * Nnodes * 4);
    H_init = (fType*) malloc(sizeof(fType) * Nnodes * 4);
    fread((void*) tmp, sizeof(fType_Input), Nnodes * 4, fp_in);
    for(i=0; i<Nnodes*4; i++)
      H_init[i] = (fType)tmp[i];
    free(tmp);

    fclose(fp_in);
    return H_init;
}

// Get Final Conditions after 100 timesteps

fType* get_FCs_bin(char* inputFile) {

    fType* H_100;
    long Nnodes, Nnbr;
    int temp;

    // RDL Enables fType and fType_Input to actually differ

    fType_Input* tmp;
    int i;

    // open input file
    FILE* fp_in = fopen(inputFile, "r");
    CHECK_OPEN_ERR(!fp_in, inputFile);

    // constants/parameters
    fread((void*) &temp, sizeof(int), 1, fp_in);
    Nnodes = (long) temp;
    fread((void*) &temp, sizeof(int), 1, fp_in);
    Nnbr = (long) temp;

    // calculate and position file pointer
    long offset = (sizeof(int) * (2 + (Nnodes * Nnbr))) + (sizeof(fType_Input) * (4 + (5 * Nnodes) + (4 * Nnodes * 3) + (4 * Nnodes * Nnbr) + (Nnodes * 4)));
    fseek(fp_in, offset, SEEK_SET);

    // allocate/read H_100
    // RDL Enables fType and fType_Input to actually differ
    
    tmp = (fType_Input*) malloc(sizeof(fType_Input) * Nnodes * 4);
    H_100 = (fType*) malloc(sizeof(fType) * Nnodes * 4);
    fread((void*) tmp, sizeof(fType_Input), Nnodes * 4, fp_in);
    for(i=0; i<Nnodes*4; i++)
      H_100[i] = (fType)tmp[i];
    free(tmp);

    fclose(fp_in);
    return H_100;
}
