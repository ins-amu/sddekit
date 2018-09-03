/* copyright 2016 Apache 2 libtvb authors */

#include <stdio.h>

#include "libtvb.h"
#include "test.h"

# define M_PI		3.14159265358979323846

TEST(out, file) {
	double t, x[2], c[1];
	tvb_out_file *of;
	FILE *fd;
	char *fname;

	t = 2.3;
	x[0] = 2*t;
	x[1] = 3*t;
	c[0] = 4*t;
	fname = "test_out_file.dat";

	of = tvb_out_file_new_from_std(stderr);
	EXPECT_EQ(stderr, of->get_fd(of));
	EXPECT_EQ(true, of->is_std(of));
	TVB_CALL_AS_(of, out, free);

	of = tvb_out_file_new_from_name(fname);
	EXPECT_EQ(false, of->is_std(of));
	TVB_CALL_AS_(of, out, free);

	of = tvb_out_file_new_from_name(fname);
	TVB_CALL_AS(of, out, apply, t, 2, x, 1, c);
	TVB_CALL_AS_(of, out, free);

	fd = fopen(fname, "r");
	{
		int nx, nc;
		double t_, x_[2], c_[1];
		fscanf(fd, "%lf %d %lf %lf %d %lf\n", &t_, &nx, x_, x_+1, &nc, c_);
		ASSERT_NEAR(t_, t, 1e-10);
		EXPECT_EQ(2,nx);
		ASSERT_NEAR(x_[0], x[0], 1e-10);
		ASSERT_NEAR(x_[1], x[1], 1e-10);
		EXPECT_EQ(1,nc);
		ASSERT_NEAR(c_[0], c[0], 1e-10);
	}
	fclose(fd);
}

TEST(out, mem) {
	int i;
	double t, x[3][2], c[1];
	tvb_out_mem *om;

	t = 2.3;
	for (i=0; i<6; i++)
		x[i/2][i%2] = i*t;
	c[0] = 4*t;

	om = tvb_out_mem_new();
	EXPECT_EQ(NULL, om->get_xs(om));
	EXPECT_EQ(NULL, om->get_cs(om));
	EXPECT_EQ(0, om->get_n_sample(om));
	EXPECT_EQ(0, om->get_capacity(om));
	TVB_CALL_AS_(om, out, free);

	om = tvb_out_mem_new();
	TVB_CALL_AS(om, out, apply, t, 2, x[0], 1, c);
	EXPECT_EQ(2,om->get_capacity(om));
	EXPECT_EQ(1,om->get_n_sample(om));
	EXPECT_TRUE(NULL!=om->get_xs(om));
	EXPECT_TRUE(NULL!=om->get_cs(om));
	EXPECT_EQ(x[0][0],om->get_xs(om)[0]);
	EXPECT_EQ(x[0][1],om->get_xs(om)[1]);
	EXPECT_EQ(c[0],om->get_cs(om)[0]);

	TVB_CALL_AS(om, out, apply, t+0.1, 2, x[1], 1, c);
	EXPECT_EQ(x[0][0],om->get_xs(om)[0]);
	EXPECT_EQ(x[0][1],om->get_xs(om)[1]);
	EXPECT_EQ(x[1][0],om->get_xs(om)[2+0]);
	EXPECT_EQ(x[1][1],om->get_xs(om)[2+1]);
	EXPECT_EQ(c[0],om->get_cs(om)[1+0]);
	EXPECT_EQ(4,om->get_capacity(om));
	EXPECT_EQ(2,om->get_n_sample(om));

	TVB_CALL_AS(om, out, apply, t+0.2, 2, x[2], 1, c);
	EXPECT_EQ(x[0][0],om->get_xs(om)[0]);
	EXPECT_EQ(x[0][1],om->get_xs(om)[1]);
	EXPECT_EQ(x[1][0],om->get_xs(om)[2+0]);
	EXPECT_EQ(x[1][1],om->get_xs(om)[2+1]);
	EXPECT_EQ(x[2][0],om->get_xs(om)[4+0]);
	EXPECT_EQ(x[2][1],om->get_xs(om)[4+1]);
	EXPECT_EQ(c[0],om->get_cs(om)[2+0]);
	EXPECT_EQ(4,om->get_capacity(om));
	EXPECT_EQ(3,om->get_n_sample(om));

	TVB_CALL_AS_(om, out, free);
}

TEST(out, tee) {
	double t, x[2], c[1];
	tvb_out_tee *tee;
	tvb_out_mem *m1, *m2, *m3;

	t = 2.3;
	x[0] = 2*t;
	x[1] = 3*t;
	c[0] = 4*t;

	tee = tvb_out_tee_new(3);
	EXPECT_EQ(3,tee->get_nout(tee));
	EXPECT_TRUE(!tee->outs_is_null(tee));

	m1 = tvb_out_mem_new();
	m2 = tvb_out_mem_new();
	m3 = tvb_out_mem_new();

	EXPECT_EQ(TVB_OK, tee->set_out(tee, 0, m1->out(m1)));
	EXPECT_EQ(m1->out(m1), tee->get_out(tee, 0));

	EXPECT_EQ(TVB_OK, tee->set_out(tee, 1, m2->out(m2)));
	EXPECT_EQ(m2->out(m2 ) ,tee->get_out(tee, 1));

	EXPECT_EQ(TVB_OK, tee->set_out(tee, 2, m3->out(m3)));
	EXPECT_EQ(m3->out(m3 ) ,tee->get_out(tee, 2));

	EXPECT_EQ(TVB_ERR, tee->set_out(tee, 3, m1->out(m1)));

	TVB_CALL_AS(tee, out, apply, t, 2, x, 1, c);

	EXPECT_EQ(x[0], m1->get_xs(m1)[0]);
	EXPECT_EQ(x[1], m1->get_xs(m1)[1]);
	EXPECT_EQ(c[0], m1->get_cs(m1)[0]);
	EXPECT_EQ(x[0], m2->get_xs(m2)[0]);
	EXPECT_EQ(x[1], m2->get_xs(m2)[1]);
	EXPECT_EQ(c[0], m2->get_cs(m2)[0]);
	EXPECT_EQ(x[0], m3->get_xs(m3)[0]);
	EXPECT_EQ(x[1], m3->get_xs(m3)[1]);
	EXPECT_EQ(c[0], m3->get_cs(m3)[0]);

	TVB_CALL_AS_(m1, out, free);
	TVB_CALL_AS_(m2, out, free);
	TVB_CALL_AS_(m3, out, free);
	TVB_CALL_AS_(tee, out, free);
}

TEST(out, tavg) {
	uint32_t i;
	double t, x[2], c[1];
	tvb_out_tavg *tavg;
	tvb_out_mem *mem;

	t = 0.0;
	x[0] = 0.0;
	c[0] = 1.0;

	mem = tvb_out_mem_new();
	tavg = tvb_out_tavg_new(10, TVB_AS(mem, out));
	EXPECT_EQ(10, tavg->get_len(tavg));
	EXPECT_EQ(0, tavg->get_pos(tavg));
	EXPECT_EQ(0.0, tavg->get_t(tavg));
	EXPECT_EQ(TVB_AS(mem, out), tavg->get_out(tavg));
	EXPECT_EQ(NULL, tavg->get_x(tavg));
	EXPECT_EQ(NULL, tavg->get_c(tavg));

	TVB_CALL_AS(tavg, out, apply, t, 1, x, 1, c);
	EXPECT_TRUE(NULL!= tavg->get_x(tavg));
	EXPECT_TRUE(NULL!= tavg->get_c(tavg));
	EXPECT_EQ(t, tavg->get_t(tavg));
	EXPECT_EQ(1, tavg->get_pos(tavg));
	for (i=0; i<8; i++) {
		x[0] += 1;
		c[0] += 1;
		TVB_CALL_AS(tavg, out, apply, 1.0, 1, x, 1, c);
		ASSERT_NEAR( tavg->get_t(tavg), i+1.0, 1e-10);
		EXPECT_EQ((i+2), tavg->get_pos(tavg));
	}
	x[0] += 1;
	c[0] += 1;
	TVB_CALL_AS(tavg, out, apply, 1.0, 1, x, 1, c);
	EXPECT_EQ(0, tavg->get_pos(tavg));
	EXPECT_EQ(0.0, tavg->get_x(tavg)[0]);
	EXPECT_EQ(0.0, tavg->get_c(tavg)[0]);
	EXPECT_EQ(0.0, tavg->get_t(tavg));

	ASSERT_NEAR( mem->get_xs(mem)[0], 4.5, 1e-10);
	ASSERT_NEAR( mem->get_cs(mem)[0], 5.5, 1e-10);

	TVB_CALL_AS_(tavg, out, free);
	TVB_CALL_AS_(mem, out, free);
}

TEST(out, sfilt) {
	uint32_t i;
	double t, x[2], c[2], xf[4], cf[4];
	tvb_out_sfilt *sfilt;
	tvb_out_mem *mem;

	xf[0] = cf[2] = 1.0;
	xf[1] = cf[3] = 1.0;
	xf[2] = cf[0] = 1.0;
	xf[3] = cf[1] = -1.0;

	mem = tvb_out_mem_new();
	sfilt = tvb_out_sfilt_new(2, 2, xf, cf, TVB_AS(mem, out));
	EXPECT_EQ(2, sfilt->get_nfilt(sfilt));
	EXPECT_EQ(2, sfilt->get_filtlen(sfilt));
	EXPECT_TRUE(NULL!= sfilt->get_xfilts(sfilt));
	EXPECT_TRUE(NULL!= sfilt->get_cfilts(sfilt));
	EXPECT_TRUE(NULL!= sfilt->get_x(sfilt));
	EXPECT_TRUE(NULL!= sfilt->get_c(sfilt));
	EXPECT_EQ(TVB_AS(mem, out), sfilt->get_out(sfilt));
	for (i=0; i<4; i++) {
		EXPECT_EQ(xf[i], sfilt->get_xfilts(sfilt)[i]);
		EXPECT_EQ(cf[i], sfilt->get_cfilts(sfilt)[i]);
	}

	TVB_CALL_AS_(sfilt, out, free);

	sfilt = tvb_out_sfilt_new(2, 2, xf, cf, TVB_AS(mem, out));

	t = 0.0;
	x[0] = 1.0;
	x[1] = 2.0;
	c[0] = 0.5;
	c[1] = 1.0;

	TVB_CALL_AS(sfilt, out, apply, t, 2, x, 2, c);

	ASSERT_NEAR( mem->get_xs(mem)[0], 3.0, 1e-10);
	ASSERT_NEAR( mem->get_xs(mem)[1], -1.0, 1e-10);
	ASSERT_NEAR( mem->get_cs(mem)[0], -0.5, 1e-10);
	ASSERT_NEAR( mem->get_cs(mem)[1], 1.5, 1e-10);

	TVB_CALL_AS_(sfilt, out, free);
	TVB_CALL_AS_(mem, out, free);
}

/* simple out keep just last sample for testing */
typedef struct { double t, x[2], c[1]; } keep_last;

static tvb_stat keep_last_apply(void *data, double t,
	uint32_t nx, double * restrict x,
	uint32_t nc, double * restrict c)
{
	(void) nx; (void) nc;
	keep_last *d = data;
	d->x[0] = x[0];
	d->x[1] = x[1];
	d->c[0] = c[0];
	d->t = t;
	return TVB_CONT;
}

TEST(out, conv) {
	uint32_t i;
	tvb_out_conv *conv;
	tvb_out *last;
	keep_last kl;

	/* let filt by sin(x) on x in (0, 2 * pi), convolve with
	 * signal sin(t); ignoring boundary effects, output must
	 * be -pi cos(t).
	 */
	uint32_t n = 100;
	double t, e, x[2], c[1], *filt = tvb_malloc(sizeof(double) * n);
	double dt=M_PI*2.0/(n - 1);

	for (i=0; i<n; i++)
		filt[i] = sin(i * dt);

	last = tvb_out_new_cb(&kl, &keep_last_apply);
	conv = tvb_out_conv_new(n, filt, last);

	/* pass over boundary effect */
	for (i=0; i<n; i++)
	{
		t = i * dt;
		x[0] = sin(t);
		x[1] = x[0] * 2;
		c[0] = x[1] * 2;
		EXPECT_EQ(TVB_CONT, TVB_CALL_AS(conv, out, apply, t, 2, x, 1, c));
	}

	EXPECT_EQ(99, conv->get_pos(conv));
	EXPECT_EQ(100, conv->get_len(conv));
	EXPECT_EQ(1, conv->get_ds(conv));
	EXPECT_EQ(1, conv->get_ds_count(conv));
	EXPECT_EQ(last, conv->get_next_out(conv));
	EXPECT_EQ(2, conv->get_nx(conv));
	EXPECT_EQ(1, conv->get_nc(conv));

	/* test output of convolution */
	for (i=0; i<10; i++)
	{
		t = i * dt;
		x[0] = sin(t);
		x[1] = x[0] * 2;
		c[0] = x[1] * 2;
		EXPECT_EQ(TVB_CONT, TVB_CALL_AS(conv, out, apply, t, 2, x, 1, c));
		e = - M_PI * cos(kl.t);
		ASSERT_NEAR(1.0, kl.x[0] * dt / e, 1e-2);
		ASSERT_NEAR(1.0, kl.x[1] * dt / (2*e), 1e-2);
		ASSERT_NEAR(1.0, kl.c[0] * dt /  (4*e), 1e-2);
	}

	/* clean up */
	last->free(last);
	TVB_CALL_AS_(conv, out, free);
	tvb_free(filt);
}

struct ign_test
{
	double t;
	bool nx_zero, nc_zero, x_null, c_null;
};

static tvb_stat ign_test_apply(void *data, double t, uint32_t nx, double *x, uint32_t nc, double *c)
{
	struct ign_test *d = data;
	d->t = t;
	d->nx_zero = nx == 0;
	d->nc_zero = nc == 0;
	d->x_null = x == NULL;
	d->c_null = c == NULL;
	return TVB_CONT;
}

static void test_ign_cond(bool x, bool c)
{
	double t = M_PI;
	tvb_out *ign, *test;
	struct ign_test test_data = { 0 };
	test = tvb_out_new_cb(&test_data, &ign_test_apply);
	ign = tvb_out_new_ign(x, c, test);
	ign->apply(ign, t, 5, &t, 4, &t);
	EXPECT_EQ(test_data.t, t);
	EXPECT_EQ(test_data.nx_zero, x);
	EXPECT_EQ(test_data.nc_zero, c);
	EXPECT_EQ(test_data.x_null, x);
	EXPECT_EQ(test_data.c_null, c);
	ign->free(ign);
	test->free(test);
}

TEST(out, ign)
{
	test_ign_cond(true, true);
	test_ign_cond(false, true);
	test_ign_cond(true, false);
	test_ign_cond(false, false);
}
