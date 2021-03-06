/* This file was automatically generated by CasADi.
   The CasADi copyright holders make no ownership claim of its contents. */
#ifdef __cplusplus
extern "C" {
#endif

/* How to prefix internal symbols */
#ifdef CODEGEN_PREFIX
  #define NAMESPACE_CONCAT(NS, ID) _NAMESPACE_CONCAT(NS, ID)
  #define _NAMESPACE_CONCAT(NS, ID) NS ## ID
  #define CASADI_PREFIX(ID) NAMESPACE_CONCAT(CODEGEN_PREFIX, ID)
#else
  #define CASADI_PREFIX(ID) engine_ls_cost_N_ ## ID
#endif

#include <math.h>

#ifndef casadi_real
#define casadi_real double
#endif

#ifndef casadi_int
#define casadi_int int
#endif

/* Add prefix to internal symbols */
#define casadi_copy CASADI_PREFIX(copy)
#define casadi_f0 CASADI_PREFIX(f0)
#define casadi_fill CASADI_PREFIX(fill)
#define casadi_s0 CASADI_PREFIX(s0)
#define casadi_s1 CASADI_PREFIX(s1)
#define casadi_s2 CASADI_PREFIX(s2)
#define casadi_s3 CASADI_PREFIX(s3)

/* Symbol visibility in DLLs */
#ifndef CASADI_SYMBOL_EXPORT
  #if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
    #if defined(STATIC_LINKED)
      #define CASADI_SYMBOL_EXPORT
    #else
      #define CASADI_SYMBOL_EXPORT __declspec(dllexport)
    #endif
  #elif defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
    #define CASADI_SYMBOL_EXPORT __attribute__ ((visibility ("default")))
  #else
    #define CASADI_SYMBOL_EXPORT
  #endif
#endif

static const casadi_int casadi_s0[4] = {0, 2, 3, 4};
static const casadi_int casadi_s1[8] = {4, 1, 0, 4, 0, 1, 2, 3};
static const casadi_int casadi_s2[9] = {5, 1, 0, 5, 0, 1, 2, 3, 4};
static const casadi_int casadi_s3[14] = {4, 5, 0, 2, 3, 4, 5, 6, 2, 3, 0, 1, 2, 3};

void casadi_copy(const casadi_real* x, casadi_int n, casadi_real* y) {
  casadi_int i;
  if (y) {
    if (x) {
      for (i=0; i<n; ++i) *y++ = *x++;
    } else {
      for (i=0; i<n; ++i) *y++ = 0.;
    }
  }
}

void casadi_fill(casadi_real* x, casadi_int n, casadi_real alpha) {
  casadi_int i;
  if (x) {
    for (i=0; i<n; ++i) *x++ = alpha;
  }
}

/* engine_ls_cost_N:(i0[4])->(o0[5],o1[4x5,6nz]) */
static int casadi_f0(const casadi_real** arg, casadi_real** res, casadi_int* iw, casadi_real* w, void* mem) {
  casadi_int i;
  casadi_real *rr, *ss;
  const casadi_int *cii;
  const casadi_real *cs;
  casadi_real w0, *w1=w+1, w2, *w3=w+6, *w4=w+12, *w5=w+15, *w6=w+19, *w7=w+23, *w8=w+25;
  /* #0: @0 = 1000 */
  w0 = 1000.;
  /* #1: @1 = input[0][0] */
  casadi_copy(arg[0], 4, w1);
  /* #2: @2 = @1[2] */
  for (rr=(&w2), ss=w1+2; ss!=w1+3; ss+=1) *rr++ = *ss;
  /* #3: @0 = (@0*@2) */
  w0 *= w2;
  /* #4: @2 = @1[3] */
  for (rr=(&w2), ss=w1+3; ss!=w1+4; ss+=1) *rr++ = *ss;
  /* #5: @0 = (@0*@2) */
  w0 *= w2;
  /* #6: output[0][0] = @0 */
  if (res[0]) res[0][0] = w0;
  /* #7: output[0][1] = @1 */
  if (res[0]) casadi_copy(w1, 4, res[0]+1);
  /* #8: @3 = zeros(4x5,6nz) */
  casadi_fill(w3, 6, 0.);
  /* #9: @0 = 1000 */
  w0 = 1000.;
  /* #10: @2 = @1[3] */
  for (rr=(&w2), ss=w1+3; ss!=w1+4; ss+=1) *rr++ = *ss;
  /* #11: @0 = (@0*@2) */
  w0 *= w2;
  /* #12: @4 = ones(4x1,3nz) */
  casadi_fill(w4, 3, 1.);
  /* #13: @5 = vertcat(@0, @4) */
  rr=w5;
  *rr++ = w0;
  for (i=0, cs=w4; i<3; ++i) *rr++ = *cs++;
  /* #14: @6 = @5[:4] */
  for (rr=w6, ss=w5+0; ss!=w5+4; ss+=1) *rr++ = *ss;
  /* #15: (@3[0, 2, 3, 4] = @6) */
  for (cii=casadi_s0, rr=w3, ss=w6; cii!=casadi_s0+4; ++cii, ++ss) if (*cii>=0) rr[*cii] = *ss;
  /* #16: @0 = 1000 */
  w0 = 1000.;
  /* #17: @2 = @1[2] */
  for (rr=(&w2), ss=w1+2; ss!=w1+3; ss+=1) *rr++ = *ss;
  /* #18: @0 = (@0*@2) */
  w0 *= w2;
  /* #19: @2 = ones(4x1,1nz) */
  w2 = 1.;
  /* #20: @7 = vertcat(@0, @2) */
  rr=w7;
  *rr++ = w0;
  *rr++ = w2;
  /* #21: @8 = @7[:2] */
  for (rr=w8, ss=w7+0; ss!=w7+2; ss+=1) *rr++ = *ss;
  /* #22: (@3[1:9:4] = @8) */
  for (rr=w3+1, ss=w8; rr!=w3+9; rr+=4) *rr = *ss++;
  /* #23: output[1][0] = @3 */
  casadi_copy(w3, 6, res[1]);
  return 0;
}

CASADI_SYMBOL_EXPORT int engine_ls_cost_N(const casadi_real** arg, casadi_real** res, casadi_int* iw, casadi_real* w, void* mem){
  return casadi_f0(arg, res, iw, w, mem);
}

CASADI_SYMBOL_EXPORT void engine_ls_cost_N_incref(void) {
}

CASADI_SYMBOL_EXPORT void engine_ls_cost_N_decref(void) {
}

CASADI_SYMBOL_EXPORT casadi_int engine_ls_cost_N_n_in(void) { return 1;}

CASADI_SYMBOL_EXPORT casadi_int engine_ls_cost_N_n_out(void) { return 2;}

CASADI_SYMBOL_EXPORT const char* engine_ls_cost_N_name_in(casadi_int i){
  switch (i) {
    case 0: return "i0";
    default: return 0;
  }
}

CASADI_SYMBOL_EXPORT const char* engine_ls_cost_N_name_out(casadi_int i){
  switch (i) {
    case 0: return "o0";
    case 1: return "o1";
    default: return 0;
  }
}

CASADI_SYMBOL_EXPORT const casadi_int* engine_ls_cost_N_sparsity_in(casadi_int i) {
  switch (i) {
    case 0: return casadi_s1;
    default: return 0;
  }
}

CASADI_SYMBOL_EXPORT const casadi_int* engine_ls_cost_N_sparsity_out(casadi_int i) {
  switch (i) {
    case 0: return casadi_s2;
    case 1: return casadi_s3;
    default: return 0;
  }
}

CASADI_SYMBOL_EXPORT int engine_ls_cost_N_work(casadi_int *sz_arg, casadi_int* sz_res, casadi_int *sz_iw, casadi_int *sz_w) {
  if (sz_arg) *sz_arg = 3;
  if (sz_res) *sz_res = 3;
  if (sz_iw) *sz_iw = 0;
  if (sz_w) *sz_w = 27;
  return 0;
}


#ifdef __cplusplus
} /* extern "C" */
#endif
