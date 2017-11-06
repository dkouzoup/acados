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

// hpipm
#include "hpipm_d_dense_qp.h"
#include "hpipm_d_dense_qp_sol.h"
// acados
#include "acados/utils/types.h"
#include "acados/dense_qp/dense_qp_common.h"



int dense_qp_in_calculate_size(int nv, int ne, int nb, int ng, int ns) {

    int size = sizeof(dense_qp_in);
    size += d_memsize_dense_qp(nv, ne, nb, ng, ns);
    return size;
}



char *assign_dense_qp_in(int nv, int ne, int nb, int ng, int ns, dense_qp_in **qp_in, void *ptr) {

    // char pointer
    char *c_ptr = (char *) ptr;

    *qp_in = (dense_qp_in *) c_ptr;
    c_ptr += sizeof(dense_qp_in);

    d_create_dense_qp(nv, ne, nb, ng, ns, *qp_in, c_ptr);
    c_ptr += d_memsize_dense_qp(nv, ne, nb, ng, ns);

    return c_ptr;
}



int dense_qp_out_calculate_size(int nv, int ne, int nb, int ng, int ns) {

    int size = sizeof(dense_qp_out);
    // TODO(dimitris): update HPIPM to get rid of those casts
    size += d_memsize_dense_qp_sol(nv, ne, nb, ng, ns);

    return size;
}



char *assign_dense_qp_out(int nv, int ne, int nb, int ng, int ns,
    dense_qp_out **qp_out, void *ptr) {

    // char pointer
    char *c_ptr = (char *) ptr;

    *qp_out = (dense_qp_out *) c_ptr;
    c_ptr += sizeof(dense_qp_out);

    d_create_dense_qp_sol(nv, ne, nb, ng, ns, *qp_out, c_ptr);
    c_ptr += d_memsize_dense_qp_sol(nv, ne, nb, ng, ns);

    return c_ptr;
}