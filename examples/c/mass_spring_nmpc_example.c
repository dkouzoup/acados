/*
 *    This file is part of acados.
 *
 *    acados is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    acados is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with acados; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// #include <xmmintrin.h>

#include "blasfeo/include/blasfeo_d_aux_ext_dep.h"
#include "blasfeo/include/blasfeo_i_aux_ext_dep.h"

#include "acados/ocp_qp/ocp_qp_common.h"
#include "acados/ocp_qp/ocp_qp_partial_condensing_solver.h"
#include "acados/ocp_qp/ocp_qp_full_condensing_solver.h"

#include "acados/dense_qp/dense_qp_hpipm.h"

#include "acados/utils/math.h"
#include "acados/utils/mem.h"
#include "acados/utils/print.h"
#include "acados/utils/timing.h"
#include "acados/utils/types.h"
#include "acados/utils/external_function_generic.h"

#include "acados/ocp_nlp/ocp_nlp_sqp.h"
#include "acados/ocp_nlp/ocp_nlp_cost_common.h"
#include "acados/ocp_nlp/ocp_nlp_cost_external.h"
#include "acados/ocp_nlp/ocp_nlp_dynamics_disc.h"
#include "acados/ocp_nlp/ocp_nlp_constraints_bgh.h"

// temp
#include "acados/ocp_qp/ocp_qp_hpipm.h"



#define MAX_SQP_ITERS 10
#define NREP 10

// constraints (at stage 0): 0 box, 1 general, 2 general+nonlinear
#define CONSTRAINTS 0

// xcond: 0 no condensing, 1 part condensing, 2 full condensing
#define XCOND 0



// mass spirng system
static void mass_spring_system(double Ts, int nx, int nu, double *A, double *B, double *b) {

    int nx2 = nx * nx;

    int info = 0;

    int pp = nx / 2;  // number of masses

    /***********************************************
    * build the continuous time system
    ***********************************************/

    double *T;
    d_zeros(&T, pp, pp);

    for (int ii = 0; ii < pp; ii++) T[ii * (pp + 1)] = -2;
    for (int ii = 0; ii < pp - 1; ii++) T[ii * (pp + 1) + 1] = 1;
    for (int ii = 1; ii < pp; ii++) T[ii * (pp + 1) - 1] = 1;

    double *Z;
    d_zeros(&Z, pp, pp);
    double *I;
    d_zeros(&I, pp, pp);
    for (int ii = 0; ii < pp; ii++) I[ii * (pp + 1)] = 1.0;  // I = eye(pp);
    double *Ac;
    d_zeros(&Ac, nx, nx);
    dmcopy(pp, pp, Z, pp, Ac, nx);
    dmcopy(pp, pp, T, pp, Ac + pp, nx);
    dmcopy(pp, pp, I, pp, Ac + pp * nx, nx);
    dmcopy(pp, pp, Z, pp, Ac + pp * (nx + 1), nx);
    free(T);
    free(Z);
    free(I);

    d_zeros(&I, nu, nu);
    for (int ii = 0; ii < nu; ii++) I[ii * (nu + 1)] = 1.0;  // I = eye(nu);
    double *Bc;
    d_zeros(&Bc, nx, nu);
    dmcopy(nu, nu, I, nu, Bc + pp, nx);
    free(I);

    /************************************************
    * compute the discrete time system
    ************************************************/

    double *bb;
    d_zeros(&bb, nx, 1);
    dmcopy(nx, 1, bb, nx, b, nx);

    dmcopy(nx, nx, Ac, nx, A, nx);
    dscal_3l(nx2, Ts, A);
    expm(nx, A);

    d_zeros(&T, nx, nx);
    d_zeros(&I, nx, nx);
    for (int ii = 0; ii < nx; ii++) I[ii * (nx + 1)] = 1.0;  // I = eye(nx);
    dmcopy(nx, nx, A, nx, T, nx);
    daxpy_3l(nx2, -1.0, I, T);
    dgemm_nn_3l(nx, nu, nx, T, nx, Bc, nx, B, nx);
    free(T);
    free(I);

    int *ipiv = (int *)malloc(nx * sizeof(int));
    dgesv_3l(nx, nu, Ac, nx, ipiv, B, nx, &info);
    free(ipiv);

    free(Ac);
    free(Bc);
    free(bb);
}



// hand-generated external function for externally provided hessian and gradient
void ext_cost(void *fun, ext_fun_arg_t *type_in, void **in, ext_fun_arg_t *type_out, void **out)
{

	int ii;

	int nu = 3;
	int nx = 8;

	int nv = nu+nx;

	// Hessian
	double *hess = out[1];
	for (ii=0; ii<nv*nv; ii++)
		hess[ii] = 0.0;
	for (ii=0; ii<nu; ii++)
		hess[ii*(nv+1)] = 2.0; // R
	for (; ii<nu+nx; ii++)
		hess[ii*(nv+1)] = 1.0; // Q

	// gradient
	double *ux = in[0];
	double *grad = out[0];
	for (ii=0; ii<nu; ii++)
		grad[ii] = 0.0 + hess[ii*(nv+1)]*ux[ii]; // r
	for (; ii<nu+nx; ii++)
		grad[ii] = 0.0 + hess[ii*(nv+1)]*ux[ii]; // q


	return;

}

void ext_costN(void *fun, ext_fun_arg_t *type_in, void **in, ext_fun_arg_t *type_out, void **out)
{

	int ii;

	int nu = 0;
	int nx = 8;

	int nv = nu+nx;

	// Hessian
	double *hess = out[1];
	for (ii=0; ii<nv*nv; ii++)
		hess[ii] = 0.0;
	for (ii=0; ii<nu; ii++)
		hess[ii*(nv+1)] = 2.0; // R
	for (; ii<nu+nx; ii++)
		hess[ii*(nv+1)] = 1.0; // Q

	// gradient
	double *ux = in[0];
	double *grad = out[0];
	for (ii=0; ii<nu; ii++)
		grad[ii] = 0.0 + hess[ii*(nv+1)]*ux[ii]; // r
	for (; ii<nu+nx; ii++)
		grad[ii] = 0.0 + hess[ii*(nv+1)]*ux[ii]; // q

	return;

}

void disc_model(void *fun0, ext_fun_arg_t *type_in, void **in, ext_fun_arg_t *type_out, void **out)
{

	int ii, jj;

	int nu = 3;
	int nx = 8;

	// compute mass sping dynamics
    double *A;
    d_zeros(&A, nx, nx);  // states update matrix
    double *B;
    d_zeros(&B, nx, nu);  // inputs matrix
    double *b;
    d_zeros(&b, nx, 1);  // states offset
    double Ts = 0.5;

    mass_spring_system(Ts, nx, nu, A, B, b);

    for (ii=0; ii<nx; ii++)
		b[ii] = 0.0;

	// extract input
	double *x = in[0];
	double *u = in[1];

	// extract output
	double *fun = out[0];
	double *jac = out[1];

	// fun
	for (ii=0; ii<nx; ii++)
		fun[ii] = b[ii];
	for (ii=0; ii<nx; ii++)
		for (jj=0; jj<nx; jj++)
			fun[ii] += A[ii+nx*jj] * x[jj];
	for (ii=0; ii<nx; ii++)
		for (jj=0; jj<nu; jj++)
			fun[ii] += B[ii+nx*jj] * u[jj];

	// jac
	for (jj=0; jj<nx; jj++)
		for (ii=0; ii<nx; ii++)
			jac[ii+nx*jj] = A[ii+nx*jj];
	for (jj=0; jj<nu; jj++)
		for (ii=0; ii<nx; ii++)
			jac[nx*nx+ii+nx*jj] = B[ii+nx*jj];

	// free memory
	free(A);
	free(B);
	free(b);

	return;

}



/************************************************
* main
************************************************/

int main() {
    // _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);

	// failure
	int status = 1;

	int tmp_size;

	int ii;

    /************************************************
    * dimensions
    ************************************************/

	int N = 15;
	int nx_ = 8;
	int nu_ = 3;

	int nb_ = nx_+nu_;
	int ng_ = 0;
	int nh_ = 0;
	// int ns_ = 0;

	int nbu_ = nu_<nb_ ? nu_ : nb_;
    int nbx_ = nb_ - nu_ > 0 ? nb_ - nu_ : 0;



    int nx[N+1];
    nx[0] = nx_;
    for (int ii = 1; ii <= N; ii++)
	{
        nx[ii] = nx_;
    }

    int nu[N+1];
    for (int ii = 0; ii < N; ii++)
	{
        nu[ii] = nu_;
    }
    nu[N] = 0;

    int nbu[N+1];
    for (int ii = 0; ii < N; ii++)
    {
        nbu[ii] = nbu_;
	}
	nbu[N] = 0;

    int nbx[N+1];
	nbx[0] = nx_;
    for (int ii = 1; ii <= N; ii++)
    {
        nbx[ii] = nbx_;
    }

    int nb[N+1];
    for (int ii = 0; ii <= N; ii++)
	{
        nb[ii] = nbu[ii]+nbx[ii];
    }

    int ng[N+1];
    for (int ii = 0; ii <= N; ii++)
	{
        ng[ii] = ng_;
    }

    int nh[N+1];
    for (int ii = 0; ii <= N; ii++)
	{
        nh[ii] = nh_;
    }

    int nq[N+1];
    for (int ii = 0; ii <= N; ii++)
	{
        nq[ii] = 0;
    }

	int ns[N+1];
	ns[0] = 0;
	for (int ii = 1; ii <= N; ii++)
	{
//        ns[ii] = 0;
        ns[ii] = nx[ii];
    }

    int ny[N+1];
    for (int ii = 0; ii <= N; ii++)
	{
        ny[ii] = 0;
    }

	int nz[N+1];
	for (int ii = 0; ii <= N; ii++)
	{
		nz[ii] = 0;
	}



    /************************************************
    * config
    ************************************************/

	int config_size = ocp_nlp_solver_config_calculate_size(N);
	void *config_mem = malloc(config_size);
	ocp_nlp_solver_config *config = ocp_nlp_solver_config_assign(N, config_mem);

#if XCOND==2
	// full condensing HPIPM
	ocp_qp_full_condensing_solver_config_initialize_default(config->qp_solver);
	dense_qp_hpipm_config_initialize_default(config->qp_solver->qp_solver);
#else
	// no condensing or partial condensing HPIPM
	ocp_qp_partial_condensing_solver_config_initialize_default(config->qp_solver);
	ocp_qp_hpipm_config_initialize_default(config->qp_solver->qp_solver);
#endif


	// external cost
    for (int ii = 0; ii <= N; ii++)
    {
		ocp_nlp_cost_external_config_initialize_default(config->cost[ii]);
    }

	// dynamics: discrete model
    for (int ii = 0; ii < N; ii++)
    {
		ocp_nlp_dynamics_disc_config_initialize_default(config->dynamics[ii]);
    }

	// constraitns
    for (int ii = 0; ii <= N; ii++)
    {
		ocp_nlp_constraints_bgh_config_initialize_default(config->constraints[ii]);
    }

    /************************************************
    * ocp_nlp_dims
    ************************************************/

	int dims_size = ocp_nlp_dims_calculate_size(config);
	void *dims_mem = malloc(dims_size);
	ocp_nlp_dims *dims = ocp_nlp_dims_assign(config, dims_mem);
	ocp_nlp_dims_initialize(config, nx, nu, ny, nbx, nbu, ng, nh, nq, ns, nz, dims);

    /************************************************
    * dynamics
    ************************************************/

	external_function_generic disc_model_generic;
	disc_model_generic.evaluate = &disc_model;

    /************************************************
    * external cost
    ************************************************/

	external_function_generic ext_cost_generic;
	ext_cost_generic.evaluate = &ext_cost;

	external_function_generic ext_costN_generic;
	ext_costN_generic.evaluate = &ext_costN;

    /************************************************
    * constraints
    ************************************************/

    /* box constraints */

    // initial state
    double *x0;
    d_zeros(&x0, nx_, 1);  // initial state
    x0[0] = 2.5;
    x0[1] = 2.5;

    int jj_end;

    int *idxb0;
    int_zeros(&idxb0, nb[0], 1);
    double *lb0;
    d_zeros(&lb0, nb[0], 1);
    double *ub0;
    d_zeros(&ub0, nb[0], 1);
    jj_end = nu[0] < nb[0] ? nu[0] : nb[0];
    for (int jj = 0; jj < jj_end; jj++) {
        lb0[jj] = -0.5;  // umin
        ub0[jj] =  0.5;  // umax
        idxb0[jj] = jj;
    }
    for (int jj = jj_end; jj < nb[0]; jj++) {
        lb0[jj] = x0[jj-jj_end];  // initial state
        ub0[jj] = x0[jj-jj_end];  // initial state
        idxb0[jj] = jj;
    }

    int *idxb1;
    int_zeros(&idxb1, nb[1], 1);
    double *lb1;
    d_zeros(&lb1, nb[1], 1);
    double *ub1;
    d_zeros(&ub1, nb[1], 1);
    jj_end = nu[1] < nb[1] ? nu[1] : nb[1];
    for (int jj = 0; jj < jj_end; jj++) {
        lb1[jj] = -0.5;  // umin
        ub1[jj] = +0.5;  // umax
        idxb1[jj] = jj;
    }
    for (int jj = jj_end; jj < nb[1]; jj++) {
        lb1[jj] = -1.0;  // xmin
        ub1[jj] = +1.0;  // xmax
        idxb1[jj] = jj;
    }
    //    int_print_mat(nb[1], 1, idxb1, nb[1]);
    //    d_print_mat(nb[1], 1, lb1, nb[1]);

    int *idxbN;
    int_zeros(&idxbN, nb[N], 1);
    double *lbN;
    d_zeros(&lbN, nb[N], 1);
    double *ubN;
    d_zeros(&ubN, nb[N], 1);
    jj_end = nu[N] < nb[N] ? nu[N] : nb[N];
    for (int jj = 0; jj < jj_end; jj++) {
        lbN[jj] = -0.5;  // umin
        ubN[jj] = +0.5;  // umax
        idxbN[jj] = jj;
    }
    for (int jj = jj_end; jj < nb[N]; jj++)
    {
        lbN[jj] = -1.0;  // xmin
        ubN[jj] = +1.0;  // xmax
        idxbN[jj] = jj;
    }


    /* soft constraints */

	double *Zl0; d_zeros(&Zl0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		Zl0[ii] = 1e3;
	double *Zu0; d_zeros(&Zu0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		Zu0[ii] = 1e3;
	double *zl0; d_zeros(&zl0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		zl0[ii] = 1e2;
	double *zu0; d_zeros(&zu0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		zu0[ii] = 1e2;
	int *idxs0; int_zeros(&idxs0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		idxs0[ii] = nu[0]+ii;
	double *ls0; d_zeros(&ls0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		ls0[ii] = 0.0;
	double *us0; d_zeros(&us0, ns[0], 1);
	for(ii=0; ii<ns[0]; ii++)
		us0[ii] = 0.0;

	double *Zl1; d_zeros(&Zl1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		Zl1[ii] = 1e3;
	double *Zu1; d_zeros(&Zu1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		Zu1[ii] = 1e3;
	double *zl1; d_zeros(&zl1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		zl1[ii] = 1e2;
	double *zu1; d_zeros(&zu1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		zu1[ii] = 1e2;
	int *idxs1; int_zeros(&idxs1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		idxs1[ii] = nu[1]+ii;
	double *ls1; d_zeros(&ls1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		ls1[ii] = 0.0;
	double *us1; d_zeros(&us1, ns[1], 1);
	for(ii=0; ii<ns[1]; ii++)
		us1[ii] = 0.0;

	double *ZlN; d_zeros(&ZlN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		ZlN[ii] = 1e3;
	double *ZuN; d_zeros(&ZuN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		ZuN[ii] = 1e3;
	double *zlN; d_zeros(&zlN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		zlN[ii] = 1e2;
	double *zuN; d_zeros(&zuN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		zuN[ii] = 1e2;
	int *idxsN; int_zeros(&idxsN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		idxsN[ii] = nu[N]+ii;
	double *lsN; d_zeros(&lsN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		lsN[ii] = 0.0;
	double *usN; d_zeros(&usN, ns[N], 1);
	for(ii=0; ii<ns[N]; ii++)
		usN[ii] = 0.0;



    /************************************************
    * nlp_in (wip)
    ************************************************/

	tmp_size = ocp_nlp_in_calculate_size(config, dims);
	void *nlp_in_mem = malloc(tmp_size);
	ocp_nlp_in *nlp_in = ocp_nlp_in_assign(config, dims, nlp_in_mem);

// ocp_nlp_dims_print(nlp_in->dims);

	// sampling times
	double Ts = 0.5;

	for (int ii=0; ii<N; ii++)
		nlp_in->Ts[ii] = Ts;


	/* external cost */

    ocp_nlp_cost_external_model **cost = (ocp_nlp_cost_external_model **) nlp_in->cost;

	for (int i=0; i<N; i++)
	{
		cost[i]->ext_cost = &ext_cost_generic;
	}
	cost[N]->ext_cost = &ext_costN_generic;

	blasfeo_pack_dvec(ns[0], Zl0, &cost[0]->Z, 0);
	blasfeo_pack_dvec(ns[0], Zu0, &cost[0]->Z, ns[0]);
	blasfeo_pack_dvec(ns[0], zl0, &cost[0]->z, 0);
	blasfeo_pack_dvec(ns[0], zu0, &cost[0]->z, ns[0]);
	for (ii=1; ii<N; ii++)
	{
		blasfeo_pack_dvec(ns[ii], Zl1, &cost[ii]->Z, 0);
		blasfeo_pack_dvec(ns[ii], Zu1, &cost[ii]->Z, ns[ii]);
		blasfeo_pack_dvec(ns[ii], zl1, &cost[ii]->z, 0);
		blasfeo_pack_dvec(ns[ii], zu1, &cost[ii]->z, ns[ii]);
	}
	blasfeo_pack_dvec(ns[N], ZlN, &cost[N]->Z, 0);
	blasfeo_pack_dvec(ns[N], ZuN, &cost[N]->Z, ns[N]);
	blasfeo_pack_dvec(ns[N], zlN, &cost[N]->z, 0);
	blasfeo_pack_dvec(ns[N], zuN, &cost[N]->z, ns[N]);


	/* dynamics */
	ocp_nlp_dynamics_disc_model **dynamics = (ocp_nlp_dynamics_disc_model **) nlp_in->dynamics;

	for (int i=0; i<N; i++)
	{
		dynamics[i]->discrete_model = &disc_model_generic;
	}


    /* constraints */

	ocp_nlp_constraints_bgh_model **constraints = (ocp_nlp_constraints_bgh_model **) nlp_in->constraints;

	// fist stage
#if CONSTRAINTS==0 // box constraints
	blasfeo_pack_dvec(nb[0], lb0, &constraints[0]->d, 0);
	blasfeo_pack_dvec(nb[0], ub0, &constraints[0]->d, nb[0]+ng[0]+nh[0]);
    constraints[0]->idxb = idxb0;
	blasfeo_pack_dvec(ns[0], ls0, &constraints[0]->d, 2*nb[0]+2*ng[0]+2*nh[0]);
	blasfeo_pack_dvec(ns[0], us0, &constraints[0]->d, 2*nb[0]+2*ng[0]+2*nh[0]+ns[0]);
    constraints[0]->idxs = idxs0;
#elif CONSTRAINTS==1 // general constraints
	// TODO
#else // general+nonlinear constraints
	// TODO
#endif

	// other stages
    for (int i = 1; i < N; i++)
	{
		blasfeo_pack_dvec(nb[i], lb1, &constraints[i]->d, 0);
		blasfeo_pack_dvec(nb[i], ub1, &constraints[i]->d, nb[i]+ng[i]+nh[i]);
        constraints[i]->idxb = idxb1;
		blasfeo_pack_dvec(ns[i], ls1, &constraints[i]->d, 2*nb[i]+2*ng[i]+2*nh[i]);
		blasfeo_pack_dvec(ns[i], us1, &constraints[i]->d, 2*nb[i]+2*ng[i]+2*nh[i]+ns[i]);
		constraints[i]->idxs = idxs1;
    }
	blasfeo_pack_dvec(nb[N], lbN, &constraints[N]->d, 0);
	blasfeo_pack_dvec(nb[N], ubN, &constraints[N]->d, nb[N]+ng[N]+nh[N]);
    constraints[N]->idxb = idxbN;
	blasfeo_pack_dvec(ns[N], lsN, &constraints[N]->d, 2*nb[N]+2*ng[N]+2*nh[N]);
	blasfeo_pack_dvec(ns[N], usN, &constraints[N]->d, 2*nb[N]+2*ng[N]+2*nh[N]+ns[N]);
    constraints[N]->idxs = idxsN;


#if 0
	for (int ii=0; ii<=N; ii++)
	{
		blasfeo_print_dmat(nu[ii]+nx[ii], ng[ii], &constraints[ii]->DCt, 0, 0);
		blasfeo_print_tran_dvec(2*nb[ii]+2*ng[ii]+2*nh[ii], &constraints[ii]->d, 0);
	}
	exit(1);
#endif

    /************************************************
    * sqp opts
    ************************************************/

	tmp_size = ocp_nlp_sqp_opts_calculate_size(config, dims);
	void *nlp_opts_mem = malloc(tmp_size);
	ocp_nlp_sqp_opts *nlp_opts = ocp_nlp_sqp_opts_assign(config, dims, nlp_opts_mem);

	ocp_nlp_sqp_opts_initialize_default(config, dims, nlp_opts);

#if XCOND==1
	// partial condensing
	ocp_qp_partial_condensing_solver_opts *pcond_solver_opts = nlp_opts->qp_solver_opts;
	pcond_solver_opts->pcond_opts->N2 = 5; // set partial condensing horizon
#endif

    for (int i = 0; i < N; ++i)
	{
		// dynamics: discrete model
		// no options
    }



	// XXX hack: overwrite config with hand-setted one
// nlp_opts->qp_solver = &config_qp;
// nlp_opts->sim_solvers = config_sim_ptrs;
// for (int ii=0; ii<N; ii++)
// 	nlp_opts->sim_solvers[ii] = config_sim_ptrs[ii];



    nlp_opts->maxIter = MAX_SQP_ITERS;
    nlp_opts->min_res_g = 1e-6;
    nlp_opts->min_res_b = 1e-9;
    nlp_opts->min_res_d = 1e-9;
    nlp_opts->min_res_m = 1e-9;

	// update after user-defined options
	ocp_nlp_sqp_opts_update(config, dims, nlp_opts);

    /************************************************
    * ocp_nlp out
    ************************************************/

	tmp_size = ocp_nlp_out_calculate_size(config, dims);
	void *nlp_out_mem = malloc(tmp_size);
	ocp_nlp_out *nlp_out = ocp_nlp_out_assign(config, dims, nlp_out_mem);

// ocp_nlp_dims_print(nlp_out->dims);

    /************************************************
    * sqp memory
    ************************************************/

	tmp_size = ocp_nlp_sqp_memory_calculate_size(config, dims, nlp_opts);
	void *nlp_mem_mem = malloc(tmp_size);
	ocp_nlp_sqp_memory *nlp_mem = ocp_nlp_sqp_memory_assign(config, dims, nlp_opts, nlp_mem_mem);


    /************************************************
    * sqp workspace
    ************************************************/

    int workspace_size = ocp_nlp_sqp_workspace_calculate_size(config, dims, nlp_opts);
    void *nlp_work = acados_malloc(workspace_size, 1);

    /************************************************
    * sqp solve
    ************************************************/

    acados_timer timer;
    acados_tic(&timer);

    for (int rep = 0; rep < NREP; rep++)
    {
		// warm start output initial guess of solution
// 	if (rep==0)
// 	{
			for (int i=0; i<=N; i++)
			{
				blasfeo_dvecse(nu[i], 0.0, nlp_out->ux+i, 0);
				blasfeo_dvecse(nx[i], 0.0, nlp_out->ux+i, nu[i]);
// 			blasfeo_pack_dvec(nu[i], uref, nlp_out->ux+i, 0);
// 			blasfeo_pack_dvec(nx[i], xref, nlp_out->ux+i, nu[i]);
			}
// 	}

		// call nlp solver
        status = ocp_nlp_sqp(config, dims, nlp_in, nlp_out, nlp_opts, nlp_mem, nlp_work);
    }

    double time = acados_toc(&timer)/NREP;

	printf("\nresiduals (max = %e)\n", nlp_out->inf_norm_res);
// ocp_nlp_res_print(dims, nlp_mem->nlp_res);

	printf("\nsolution\n");
	ocp_nlp_out_print(dims, nlp_out);

    printf("\n\nstatus = %i, iterations (max %d) = %d, total time = %f ms\n\n", status, MAX_SQP_ITERS, nlp_mem->sqp_iter, time*1e3);

    for (int k =0; k < 3; k++) {
        printf("u[%d] = \n", k);
		blasfeo_print_tran_dvec(nu[k], nlp_out->ux+k, 0);
        printf("x[%d] = \n", k);
		blasfeo_print_tran_dvec(nx[k], nlp_out->ux+k, nu[k]);
    }
    printf("u[N-1] = \n");
	blasfeo_print_tran_dvec(nu[N-1], nlp_out->ux+N-1, 0);
    printf("x[N] = \n");
	blasfeo_print_tran_dvec(nx[N], nlp_out->ux+N, nu[N]);

    /************************************************
    * free memory
    ************************************************/

	free(config_mem);
	free(dims_mem);
    free(nlp_in_mem);
    free(nlp_out_mem);
    free(nlp_work);
    free(nlp_mem_mem);
    free(nlp_opts_mem);
#if 0
#endif

/************************************************
* return
************************************************/

	if (status == 0)
		printf("\nsuccess!\n\n");
	else
		printf("\nfailure!\n\n");

	return 0;

}
