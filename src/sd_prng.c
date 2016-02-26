/* copyright 2016 Apache 2 sddekit authors */

#define _POSIX_C_SOURCE 200809L
#include "sddekit.h"

#include "parallel/iuncl.h"
//#include "parallel/ranluxcl.cl"

/*

typedef struct{
	float
		s01, s02, s03, s04,
		s05, s06, s07, s08,
		s09, s10, s11, s12,
		s13, s14, s15, s16,
		s17, s18, s19, s20,
		s21, s22, s23, s24;
	float carry;
	float dummy; //Causes struct to be a multiple of 128 bits
	int in24;
	int stepnr;
} ranluxcl_state_t;

*/

/* 
 * Automatically set the number of work-groups and the number of work-items per
 * work-group if not specified by the user.
 */
static void autoSetWgNumAndSize(cl_kernel kernel, cl_command_queue queue, 
		size_t *wg_size, size_t *wg_num)
{
	cl_uint max_cu;
	cl_device_id device;

	size_t wg_per_cu = 16;

	IUNCLERR( clGetCommandQueueInfo(
				queue, CL_QUEUE_DEVICE, sizeof(device), &device, NULL) );
	if(*wg_num == 0){
		IUNCLERR( clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, 
					sizeof(max_cu), &max_cu, NULL) );
		*wg_num = wg_per_cu * max_cu;
	}
	
	if(*wg_size == 0){
		#ifdef CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE
		IUNCLERR( clGetKernelWorkGroupInfo(kernel, device,
			CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
			sizeof(*wg_size), wg_size, NULL) );
		#else
		printf("WARNING: Could not quiery preferred work-group size.\n");
		*wg_size = 64;
		#endif
	}
}

/*
 * Perform various setup tasks like compiling the kernels, setting the number
 * of work-items and initializing the generator.
 */
static void setup(size_t *wg_size, size_t *wg_num,
	int lux, int gen_per_it, bool no_warm, bool legacy,
	cl_mem *buff_state, cl_mem *buff_prns,
	cl_program *program, cl_context context, cl_command_queue queue)
{
	cl_uint ins = 0;
	cl_int err;
	cl_kernel kernel_init, kernel_prn;

	char bopt[1024] = " -I .";
	#endif
	
	if(no_warm){
		sprintf(&bopt[strlen(bopt)], " -D RANLUXCL_NO_WARMUP");
	}
	if(legacy) {
		sprintf(&bopt[strlen(bopt)], " -D RANLUXCL_USE_LEGACY_INITIALIZATION");
	}
	sprintf(&bopt[strlen(bopt)], " -D RANLUXCL_LUX=%d", lux);
	sprintf(&bopt[strlen(bopt)], " -D GEN_PER_IT=%d", gen_per_it);

	iunclCompileKernel("ranluxcl-test_kernels.cl", bopt, context, program, 1);
	printf("\n"); //Add newline since Nvidia's compiler likes to spit out stuff.

	kernel_init = clCreateKernel(*program, "kernelInit", &err);
	IUNCLERR( err );

	kernel_prn = clCreateKernel(*program, "kernelPrn", &err);
	IUNCLERR( err );

	//Auto set work-group size and number if not specified
	autoSetWgNumAndSize(kernel_prn, queue, wg_size, wg_num);

	size_t wi_tot = *wg_size * *wg_num;
	printf("Setting up generator."
			"\n%*s %d\n%*s %d\n%*s %zu\n%*s %zu\n%*s %zu\n\n",
		-PAD, "gen_per_it:", gen_per_it,
		-PAD, "Luxury:", lux,
		-PAD, "Work-items:", wi_tot,
		-PAD, "Work-group size:", *wg_size,
		-PAD, "Work-groups:", *wg_num);

	*buff_prns = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
			gen_per_it * wi_tot * sizeof(cl_float4), NULL, &err);
	IUNCLERR( err );
	*buff_state = clCreateBuffer(context, CL_MEM_READ_WRITE,
			sizeof(ranluxcl_state_t) * wi_tot, NULL, &err);
	IUNCLERR( err );

	IUNCLERR( clSetKernelArg(kernel_init, 0, sizeof(ins), &ins) );
	IUNCLERR( clSetKernelArg(kernel_init, 1, sizeof(*buff_state), buff_state) );

	IUNCLERR( clSetKernelArg(kernel_prn, 0, sizeof(*buff_state), buff_state) );
	IUNCLERR( clSetKernelArg(kernel_prn, 1, sizeof(*buff_prns), buff_prns) );

	//Initialize the generator.
	IUNCLERR( clEnqueueNDRangeKernel(queue, kernel_init, 1, NULL, &wi_tot,
				wg_size, 0, NULL, NULL) );
	IUNCLERR( clFinish(queue) );
}

static void filler(int lux, ) {
	cl_uint pid=0, did=0;	//Setting to 0 for now
	size_t wg_size, wg_num, gen_per_it;
	size_t wr_bytes;

	cl_mem buff_state, buff_prns;
	cl_program program;
	cl_context context;
	cl_command_queue queue;

	iunclGetSingleDeviceContext(pid, did, &context, &queue, 1);

	setup(&wg_size, &wg_num, lux, gen_per_it, chk,
		&buff_state, &buff_prns,
		&program, context, queue);

}

static void rng_seed(sd_prng *r, uint32_t seed)
{
	struct ranluxcl_state_t *d = r->ptr;
	d->seed = seed;
	d->ncall = 0;
	rk_seed(seed, &d->rks);
}

static double rng_norm(sd_prng *r)
{
	struct rng_data *d = r->ptr;
	return rk_gauss(&(d->rks));
}

static double rng_uniform(sd_prng *r)
{
	struct rng_data *d = r->ptr;
	return rk_random(&(d->rks)) * 1.0 / RK_MAX;
}

static void rng_fill_norm(sd_prng *r, uint32_t n, double *x)
{
	uint32_t i;
	struct prng_data *d = r->ptr;
	for (i=0; i<n; i++)
		x[i] = rk_gauss(&d->rks);
}

static void rng_free(sd_prng *r)
{
	sd_free(r->ptr);
	sd_free(r);
}

static uint32_t prng_nbytes(sd_prng *r)
{
	(void) r;
	return sizeof(struct ranluxcl_state_t) + sizeof(sd_prng);
}

static sd_prng prng_default = {
	.ptr = NULL,
	.seed = &prng_seed,
	.norm = &prng_norm,
	.uniform = &prng_uniform,
	.fill_norm = &prng_fill_norm,
	.nbytes = &prng_nbytes,
	.free = &prng_free
};

sd_prng *sd_prng_new_default()
{
	sd_prng *r;
    if ((r = sd_malloc (sizeof(sd_prng))) == NULL
	 || (*r = prng_default, 0)
	 || (r->ptr = sd_malloc (sizeof(struct ranluxcl_state_t))) == NULL)
	{
		sd_free(r);
		sd_err("alloc rng failed.");
		return NULL;
	}
	return r;
}
