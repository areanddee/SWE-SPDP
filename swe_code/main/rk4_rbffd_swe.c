// Written by Samuel Elliott, Summer 2017

#include <rk4_rbffd_swe.h>
#include <halos.h>
#include <profiling.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef RHS_INNER_SIMD
#define OMP_SIMD_CLAUSE
#define ACC_SIMD_CLAUSE
#else
#define OMP_SIMD_CLAUSE simd
#define ACC_SIMD_CLAUSE vector
#endif

extern PSMD_struct* LPSMD;
extern timing_struct local_timer;

extern int mpi_rank;
extern int mpi_size;

void eval_K(fType* H, fType* K, fType* F, fType dt);
void update_D(fType* D, fType* F, fType coeff);
void update_H(fType* H, fType* D, fType dt);
void eval_RHS_swe(fType* K, fType* F);

void RK_substep(fType* H, fType* K, fType* F, fType* D, int substep_id) {

    // full RK4 timestep length
    const fType dt = LPSMD->dt;

    switch(substep_id) {
        case 0:
            // get F_0 = d/dt(K_0 = H)
            eval_RHS_swe(H, F);
            // initialize D = F_0
            copy_fp_arr(D, F, LPSMD->compute_size * 4);
            // evaluate K_1 = H + (dt/2) * F_0
            eval_K(H, K, F, dt/2.0);
            break;

        case 1:
            // get F_1 = d/dt(K_1 = H + (dt/2) * F_0)
            eval_RHS_swe(K, F);
            // update D += 2 * F_1
            update_D(D, F, 2.0);
            // evaluate K_2 = H + (dt/2) * F_0
            eval_K(H, K, F, dt/2.0);
            break;

        case 2:
            // get F_2 = d/dt(K_2 = H + (dt/2) * F_1)
            eval_RHS_swe(K, F);
            // update D += 2 * F_2
            update_D(D, F, 2.0);
            // evaluate K_3 = H + dt * F_0
            eval_K(H, K, F, dt);
            break;

        case 3:
            // get F_3 = d/dt(K_3 = H + dt * F_2)
            eval_RHS_swe(K, F);
            // update D += F_3
            update_D(D, F, 1.0);
            // update H
            update_H(H, D, dt);
            break;
    }
}

void copy_fp_arr(fType* dest, fType* src, int size) {
  int i;
    #ifdef _OPENACC
    #pragma acc parallel loop gang vector vector_length(SIMD_LENGTH) present(dest[:size],src[:size])
    for (i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    #else
    memcpy(dest, src, sizeof(fType) * size);
    #endif
}

void update_H(fType* H, fType* D, fType dt) {
  int i;
    // patch information
    const int compute_pid_s = LPSMD->compute_pid_s;
    const int compute_size = LPSMD->compute_size;
    
    double t_start = getTime();

    // update H values for each state variable at each compute node
    #ifdef _OPENMP
    #ifdef USE_GCC
    #pragma omp simd
    #pragma omp parallel for schedule(runtime)
    #else
    #pragma omp parallel for simd schedule(runtime)
    #endif
    #endif
	
    #ifdef _OPENACC
    #pragma acc parallel loop gang vector vector_length(SIMD_LENGTH) present(H[:LPSMD->patch_size*4],D[:LPSMD->compute_size*4])
    #endif
	
    for (i = 0; i < compute_size * 4; i++) {
        H[(compute_pid_s * 4) + i] += ((dt/6.0) * D[i]);
    }

    local_timer.t_update_H[local_timer.attempt] +=  (getTime() - t_start);

    t_start = getTime();

    #ifdef USE_MPI
    exchange_SVM_halos(H);
    #endif

    local_timer.t_mpi[local_timer.attempt] +=  (getTime() - t_start);
}

void eval_K(fType* H, fType* K, fType* F, fType dt) {
  int i;
    // patch information
    const int compute_pid_s = LPSMD->compute_pid_s;
    const int compute_size = LPSMD->compute_size;

    double t_start = getTime();

    // update K values for each state variable at each compute node
    #ifdef _OPENMP
    #ifdef USE_GCC
    #pragma omp simd
    #pragma omp parallel for schedule(runtime)
    #else
    #pragma omp parallel for simd schedule(runtime)
    #endif
    #endif
	
    #ifdef _OPENACC
    #pragma acc parallel loop gang vector vector_length(SIMD_LENGTH) present(H[:LPSMD->patch_size*4],K[:LPSMD->patch_size*4],F[:LPSMD->compute_size*4])
    #endif
    
    for (i = 0; i < compute_size * 4; i++) {
        K[(compute_pid_s * 4) + i] = H[(compute_pid_s * 4) + i] + (dt * F[i]);
    }

    local_timer.t_eval_K[local_timer.attempt] +=  (getTime() - t_start);

    t_start = getTime();

    #ifdef USE_MPI
    exchange_SVM_halos(K);
    #endif

    local_timer.t_mpi[local_timer.attempt] +=  (getTime() - t_start);
}

void update_D(fType* D, fType* F, fType coeff) {

  int i;
    // patch information
    const int compute_size = LPSMD->compute_size;

    double t_start = getTime();

    // update D values for each state variable at each compute node
    #ifdef _OPENMP
    #ifdef USE_GCC
    #pragma omp simd
    #pragma omp parallel for schedule(runtime)
    #else
    #pragma omp parallel for simd schedule(runtime)
    #endif
    #endif

    #ifdef _OPENACC
    #pragma acc parallel loop gang vector vector_length(SIMD_LENGTH) present(D[:LPSMD->compute_size*4],F[:LPSMD->compute_size*4])
    #endif
	
    for (i = 0; i < compute_size * 4; i++) {
        D[i] += (coeff * F[i]);
    }

    local_timer.t_update_D[local_timer.attempt] += (getTime() - t_start);
}

// Approximates RHS of SWE PDE using RBF-FD for the spatial differentiation and projects the results to confine motion to the surface of the sphere
void eval_RHS_swe(fType* K, fType* F) {
  int i,j;
    // global constants
    const fType gh0 = LPSMD->gh0;

    // node point coordinates/constants/projection vectors
    const fType* x = LPSMD->x;
    const fType* y = LPSMD->y;
    const fType* z = LPSMD->z;
    const fType* f = LPSMD->f;
    const fType* ghm = LPSMD->ghm;

    const fType* p_u = LPSMD->p_u;
    const fType* p_v = LPSMD->p_v;
    const fType* p_w = LPSMD->p_w;
    const fType* gradghm = LPSMD->gradghm;

    // DM data
    const int Nnbr = LPSMD->Nnbr;
    const int padded_Nnbr = LPSMD->padded_Nnbr;

    const int* idx = LPSMD->idx;
    const fType* Dx = LPSMD->Dx;
    const fType* Dy = LPSMD->Dy;
    const fType* Dz = LPSMD->Dz;
    const fType* L = LPSMD->L;

    // extract constants from patch structure
    const int compute_pid_s = LPSMD->compute_pid_s;
    const int compute_size = LPSMD->compute_size;
    const int patch_size = LPSMD->patch_size;

    double t_start = getTime();

    // ====================================== Calculate RHS of Confined PDE for Each State Variable at Each Node ================================= //

    #ifdef _OPENACC
    #pragma acc parallel vector_length(SIMD_LENGTH) present(\
        x[:compute_size],y[:compute_size],z[:compute_size],f[:compute_size],ghm[:compute_size], \
        gradghm[:compute_size*3],p_u[:compute_size*3],p_v[:compute_size*3],p_w[:compute_size*3], \
        idx[:compute_size*padded_Nnbr],Dx[:compute_size*padded_Nnbr],Dy[:compute_size*padded_Nnbr], \
        Dz[:compute_size*padded_Nnbr],L[:compute_size*padded_Nnbr],K[:patch_size*4],F[:compute_size*4])
    #endif
    {

    // iterate through each node in the local compute patch
    #ifdef _OPENMP
    #pragma omp parallel for OMP_SIMD_CLAUSE schedule(runtime)
    #endif
	
    #ifdef _OPENACC
    #pragma acc loop gang ACC_SIMD_CLAUSE
    #endif
	
    for (i = 0; i < compute_size; i++) {

        // --------------- Calculate RBF-FD SV Spatial Differentiation Approximations for Corresponding Node --------------------- //

        // temporary variables to hold state variable data for current node
        fType u;
        fType v;
        fType w;
        fType h;

        // initialize temporary variables for spatial differentiation at respective node
        fType du_dx = 0.0;
        fType dv_dx = 0.0;
        fType dw_dx = 0.0;
        fType dh_dx = 0.0;

        fType du_dy = 0.0;
        fType dv_dy = 0.0;
        fType dw_dy = 0.0;
        fType dh_dy = 0.0;

        fType du_dz = 0.0;
        fType dv_dz = 0.0;
        fType dw_dz = 0.0;
        fType dh_dz = 0.0;

        fType Lu = 0.0;
        fType Lv = 0.0;
        fType Lw = 0.0;
        fType Lh = 0.0;

        // iterate through each neighbor node in stencil
        #ifdef RHS_INNER_SIMD
        #ifdef _OPENMP
        #pragma omp simd reduction(+:du_dx,dv_dx,dw_dx,dh_dx,du_dy,dv_dy,dw_dy,dh_dy,du_dz,dv_dz,dw_dz,dh_dz,Lu,Lv,Lw,Lh)
        #endif
    
        #ifdef _OPENACC
        #pragma acc loop vector reduction(+:du_dx,dv_dx,dw_dx,dh_dx,du_dy,dv_dy,dw_dy,dh_dy,du_dz,dv_dz,dw_dz,dh_dz,Lu,Lv,Lw,Lh)
        #endif
        #endif
	
        for (j = 0; j < Nnbr; j++) {

            // get corresponding DM linear id and neighbor pid
            const int dm_lind = DM_LIN_ID(i, j, padded_Nnbr);
            const int nbr_pid = idx[dm_lind];

            // get neighbor node's state vars
            u = K[SVM_LIN_ID(nbr_pid, U_SV_ID)];
            v = K[SVM_LIN_ID(nbr_pid, V_SV_ID)];
            w = K[SVM_LIN_ID(nbr_pid, W_SV_ID)];
            h = K[SVM_LIN_ID(nbr_pid, H_SV_ID)];

            // update sums
            du_dx += Dx[dm_lind] * u;
            dv_dx += Dx[dm_lind] * v;
            dw_dx += Dx[dm_lind] * w;
            dh_dx += Dx[dm_lind] * h;

            du_dy += Dy[dm_lind] * u;
            dv_dy += Dy[dm_lind] * v;
            dw_dy += Dy[dm_lind] * w;
            dh_dy += Dy[dm_lind] * h;

            du_dz += Dz[dm_lind] * u;
            dv_dz += Dz[dm_lind] * v;
            dw_dz += Dz[dm_lind] * w;
            dh_dz += Dz[dm_lind] * h;
        
            Lu += L[dm_lind] * u;
            Lv += L[dm_lind] * v;
            Lw += L[dm_lind] * w;
            Lh += L[dm_lind] * h;
        }

        // ----------------------------------------------------------------------------------------------------------------------- //

        // ------------ Use RBF-FD Approximations to Calculate the RHS of the Unconfined Momentum Equations ---------------------- //

        // get current node's state vars
        u = K[SVM_LIN_ID(i + compute_pid_s, U_SV_ID)];
        v = K[SVM_LIN_ID(i + compute_pid_s, V_SV_ID)];
        w = K[SVM_LIN_ID(i + compute_pid_s, W_SV_ID)];
        h = K[SVM_LIN_ID(i + compute_pid_s, H_SV_ID)];

        // Evaluate RHS of unconstrained Momentum Equations
    
        const fType rhs_u = - ((u * du_dx) + (v * du_dy) + (w * du_dz) + (f[i] * ((y[i] * w) - (z[i] * v))) + dh_dx);
        const fType rhs_v = - ((u * dv_dx) + (v * dv_dy) + (w * dv_dz) + (f[i] * ((z[i] * u) - (x[i] * w))) + dh_dy);
        const fType rhs_w = - ((u * dw_dx) + (v * dw_dy) + (w * dw_dz) + (f[i] * ((x[i] * v) - (y[i] * u))) + dh_dz);

        // ----------------------------------------------------------------------------------------------------------------------- //

        // -------- Project Momentum Equations to Confine Motion to the Surface of the Sphere and Add Hyperviscosity ------------- //

        // vector linear ids
        const int vect_idx = VECT_LIN_ID(i, X_COMP_ID);
        const int vect_idy = VECT_LIN_ID(i, Y_COMP_ID);
        const int vect_idz = VECT_LIN_ID(i, Z_COMP_ID);

        // evaluate projections and apply hyperviscosity
        F[SVM_LIN_ID(i, U_SV_ID)] = (p_u[vect_idx] * rhs_u) + (p_u[vect_idy] * rhs_v) + (p_u[vect_idz] * rhs_w) + Lu;
        F[SVM_LIN_ID(i, V_SV_ID)] = (p_v[vect_idx] * rhs_u) + (p_v[vect_idy] * rhs_v) + (p_v[vect_idz] * rhs_w) + Lv;
        F[SVM_LIN_ID(i, W_SV_ID)] = (p_w[vect_idx] * rhs_u) + (p_w[vect_idy] * rhs_v) + (p_w[vect_idz] * rhs_w) + Lw;

        // ----------------------------------------------------------------------------------------------------------------------- //

        // ----------------- Use RBF-FD Approximations to Calculate the RHS of the Geopotential Equation ------------------------- //

        F[SVM_LIN_ID(i, H_SV_ID)] = - ((u * (dh_dx - gradghm[vect_idx])) + (v * (dh_dy - gradghm[vect_idy])) + (w * (dh_dz - gradghm[vect_idz])) + ((h + gh0 - ghm[i]) * (du_dx + dv_dy + dw_dz))) + Lh;

        // ----------------------------------------------------------------------------------------------------------------------- //
    }
    }

    local_timer.t_eval_rhs[local_timer.attempt] +=  (getTime() - t_start);

    // =========================================================================================================================================== //
}

void print_SVM(fType* H) {
  int i;
    int N = LPSMD->patch_size;

    printf("\n\n==================================================== State Variable Matrix Data ====================================================\n\n");

    for (i = 0; i < N; i++) {
        printf("\tnode_id = %4d:\tu = %12.6f,\tv = %12.6f,\tw = %12.6f,\th = %12.6f\n", i,
        H[SVM_LIN_ID(i, U_SV_ID)], H[SVM_LIN_ID(i, V_SV_ID)], H[SVM_LIN_ID(i, W_SV_ID)], H[SVM_LIN_ID(i, H_SV_ID)]);
    }

    printf("\n\n====================================================================================================================================\n\n");

}
