/* copyright 2016 Apache 2 libtvb authors */

#include "libtvb.h"
#include "test.h"

typedef struct {
	int n_calls, nx, nc;
	double t, *x, *c, *f, *g;
} sys_data;

static tvb_stat test_sys(void *data, tvb_sys_in *in, tvb_sys_out *out)
{
	sys_data *d = data;
	d->n_calls++;
	d->t = in->t;
	d->nx = in->nx;
	d->x = in->x;
	d->nc = in->nc;
	d->c = in->i;
	d->f = out->f;
	d->g = out->g;
	return TVB_OK;
}

typedef struct {
	int n_calls;
	double dt;
	tvb_rng *rng;
} sch_data;

static tvb_stat test_sch(void *data, tvb_hist *hist, tvb_rng *rng, tvb_sys *sys,
		double t, double dt, 
		uint32_t nx, double * restrict x,
		uint32_t nc, double * restrict c)
{
	sch_data *d = data;
	d->n_calls++;
	d->dt = dt;
	d->rng = rng;
	tvb_sys_in in = {.nx=nx, .nc=nc, .id=0, .t=t, .x=x, .i=c, .hist=hist, .rng=rng};
	tvb_sys_out out = {.f=NULL, .g=NULL, .o=c};
	return sys->apply(sys, &in, &out);
}

typedef struct {
	int nx;
	double tf, *x;
} out_data;

static tvb_stat test_out(void *data, double t, 
	     uint32_t nx, double * restrict x,
	     uint32_t nc, double * restrict c)
{
	out_data *d = data;
	/* unused */ (void) nc; (void) c;
	d->nx = nx;
	d->x = x;
	return t < d->tf ? TVB_CONT : TVB_STOP;
}

#define SEED 42
#define T0 2.34
#define DT 0.12
#define NX 3
#define NC 2

TEST(solv, simple) {
	uint32_t vi[NC];
	double x[NX], vd[NX], rand0;
	sys_data sysd;
	sch_data schd;
	out_data outd;
	tvb_sys *sys;
	tvb_sch *sch;
	tvb_out *out;
	tvb_sol *sol;
	tvb_rng *rng;
	tvb_hfill *hf;

	rng = tvb_rng_new_default();
	rng->seed(rng, SEED);
	rand0 = rng->norm(rng);
	rng->free(rng);

	sys = tvb_sys_new_cb(1, 1, 1, 0, 0, &sysd, &test_sys);
	sch = tvb_sch_new_cb(1, &schd, &test_sch);
	out = tvb_out_new_cb(&outd, &test_out);
	hf = tvb_hfill_new_val(0.0);

	sysd.n_calls = 0;
	schd.n_calls = 0;
	vi[0] = 0;
	vi[1] = 1;
	vd[0] = 2.1;
	vd[1] = 0.42;

	sol = tvb_sol_new_default(sys, sch, out, hf, 
		SEED, NX, x, NC, NC, vi, vd, T0, DT);

	tvb_rng *sol_rng = sol->get_rng(sol);
	EXPECT_EQ(sol_rng->norm(sol_rng), rand0);

	outd.tf = T0 + DT;

	sol->cont(sol);

	EXPECT_EQ(1,schd.n_calls);
	ASSERT_NEAR(DT,schd.dt, 1e-15);
	EXPECT_EQ(sol->get_rng(sol),schd.rng);

	EXPECT_EQ(1,sysd.n_calls);
	EXPECT_EQ(NX,sysd.nx);
	EXPECT_EQ(NC,sysd.nc);
	ASSERT_NEAR(T0,sysd.t, 1e-15);
	ASSERT_NEAR(T0+DT,sol->get_t(sol), 1e-15);
	EXPECT_EQ(sol->get_x(sol),sysd.x);
	EXPECT_EQ(sol->get_c(sol),sysd.c);
	EXPECT_EQ(NULL,sysd.f);
	EXPECT_EQ(NULL,sysd.g);

	outd.tf = T0 + 17 * DT;
	sol->cont(sol);
	EXPECT_EQ(17,sysd.n_calls);
	ASSERT_NEAR(sysd.t+DT, outd.tf, 1e-14);

	hf->free(hf);
	out->free(out);
	sch->free(sch);
	sys->free(sys);
	sol->free(sol);
}
