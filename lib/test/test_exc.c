/* copyright 2016 Apache 2 libtvb authors */

#include <stdio.h>

#include "libtvb.h"
#include "test.h"

typedef struct out_data {
	int crossed;
	double tf;
	FILE *fd;
} out_data;

static tvb_stat test_out_apply(tvb_out *out, double t, 
			     uint32_t nx, double * restrict x,
			     uint32_t nc, double * restrict c)
{
	out_data *d = out->ptr;
	(void) nx; (void) nc; (void) c;
	fprintf(d->fd, "%f\t%f\t%f\n", t, x[0], x[1]);
	if (x[0] < 0.0)
		d->crossed = 1;
	return t < d->tf ? TVB_CONT : TVB_STOP;
}

static void test_out_free(tvb_out *out) { tvb_free(out->ptr); tvb_free(out); }

tvb_out test_out_defaults = {.free=&test_out_free, 
	        .apply=&test_out_apply, 
		    .ptr=NULL};

tvb_out * test_out_new()
{
	tvb_out *out = tvb_malloc(sizeof(tvb_out));;
	*out = test_out_defaults;
	out->ptr = tvb_malloc(sizeof(out_data));
	return out;
}

static double x0[2] = {1.010403, 0.030870};

static int for_scheme(tvb_sch *sch, char *name)
{
	tvb_sys_exc *sys = tvb_sys_exc_new();
	tvb_sol *sol;
	tvb_out *out = test_out_new();
	tvb_hfill *hf = tvb_hfill_new_val(0.0);
	out_data *outd = out->ptr;
	char dat_name[100];
	uint32_t vi[1];
	double vd[1];

	/* init solver */
	vi[0] = 0;
	vd[0] = 25.0;
	sol = tvb_sol_new_default(sys->sys(sys), sch, out, hf, 42, 2, x0, 
					         1, 1, vi, vd, 0.0, 0.05);

	/* fill in data */
	outd->tf = 20.0;
	sprintf(dat_name, "test_exc_%s.dat", name);
	outd->fd = fopen(dat_name, "w");
	sys->set_a(sys, 1.01);
	sys->set_tau(sys, 3.0);
	sys->set_k(sys, -1e-3);

	/* deterministic sub-thresh, no crossing */
	outd->crossed = 0;
	sys->set_D(sys, 0.0);
	sol->cont(sol);
	EXPECT_TRUE(!outd->crossed);

	/* stochastic sub-thresh, crossing */
	outd->crossed = 0;
	outd->tf = 40.0;
	sys->set_D(sys, 0.05);
	sol->cont(sol);
	EXPECT_TRUE(outd->crossed);

	/* clean up */
	fclose(outd->fd);
	out->free(out);
	tvb_sys *exc_sys_if = sys->sys(sys);
	exc_sys_if->free(exc_sys_if);
	sol->free(sol);
	hf->free(hf);

	return 0;
}


TEST(exc, em) {
	tvb_sch *sch = tvb_sch_new_em(2);
	for_scheme(sch, "em");
	sch->free(sch);
}
	
TEST(exc, heun){
	tvb_sch *sch = tvb_sch_new_heun(2);
	for_scheme(sch, "heun");
	sch->free(sch);
}

TEST(exc, emcolor){
	tvb_sch *sch = tvb_sch_new_emc(2, 1.0);
	for_scheme(sch, "emc");
	sch->free(sch);
}
