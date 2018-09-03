/* copyright 2016 Apache 2 libtvb authors */

#include "libtvb.h"
#include "conn/data.h"

static void row_wise_weighted_sum(struct tvb_conn *tvb_conn, double *values, double *sums)
{
	struct conn *conn = tvb_conn->data;
    /* TODO omp parallel */
	for (uint32_t i=0; i<conn->n_row; i++)
	{
		double sum = 0.0;
		for (uint32_t j=conn->row_offsets[i]; j<conn->row_offsets[i+1]; j++)
			sum += conn->weights[j] * values[j];
		sums[i] = sum;
	}
}

/* set / get */

static double get_delay_scale(struct tvb_conn *tvb_conn) { return ((struct conn*) tvb_conn->data)->delay_scale; }

static enum tvb_stat set_delay_scale(
	struct tvb_conn *tvb_conn, double new_delay_scale)
{
	if (new_delay_scale <= 0.0)
	{
		tvb_err("delay_scale <= 0");
		return TVB_ERR;
	}
	struct conn *conn = (struct conn*) tvb_conn->data; 
	double rescaler = conn->delay_scale / new_delay_scale;
	for (uint32_t i=0; i<conn->nnz; i++)
		conn->delays[i] *= rescaler;
	conn->delay_scale = new_delay_scale;
	return TVB_OK;
}

static double * get_weights(struct tvb_conn *c) { return ((struct conn*) c->data)->weights; }

static double * get_delays(struct tvb_conn *c) { return ((struct conn*) c->data)->delays; }

static uint32_t get_n_nonzero(struct tvb_conn *tvb_conn) { return ((struct conn*) tvb_conn->data)->nnz; }

static uint32_t *get_nonzero_indices(struct tvb_conn *tvb_conn)
{
    return ((struct conn *) tvb_conn->data)->col_indices;
}

static uint32_t get_n_row(struct tvb_conn *tvb_conn) { return ((struct conn*) tvb_conn->data)->n_row; }

static uint32_t get_n_col(struct tvb_conn *tvb_conn) { return ((struct conn*) tvb_conn->data)->n_col; }

/* obj */

static struct tvb_conn *copy(struct tvb_conn *tvb_conn)
{
	struct conn *data = tvb_conn->data;
	struct tvb_conn *tvb_conn_copy = tvb_conn_new_sparse(
		data->n_row, data->n_col, data->nnz,
		data->row_offsets, data->col_indices,
		data->weights, data->delays);
	if (tvb_conn_copy == NULL)
		tvb_err("failed to copy tvb_conn instance.");
	return tvb_conn_copy;
}

static uint32_t n_byte(struct tvb_conn *tvb_conn)
{
	struct conn *data = tvb_conn->data;
	uint32_t byte_count = sizeof(struct conn);
	byte_count += sizeof(uint32_t)*(data->n_row + 1);
	byte_count += sizeof(uint32_t)*data->nnz;
	byte_count += sizeof(uint32_t)*data->nnz;
	byte_count += sizeof(uint32_t)*data->nnz;
	return byte_count;
}

static void conn_free(struct tvb_conn *tvb_conn)
{
	struct conn *conn = tvb_conn->data;
	tvb_free(conn->weights);
	tvb_free(conn->delays);
	tvb_free(conn->row_offsets);
	tvb_free(conn->col_indices);
	tvb_free((void*) conn);
}

/* }}} */
	
/* constructors {{{ */

static struct tvb_conn tvb_conn_defaults = {
	.copy = &copy,
	.n_byte = &n_byte,
	.free = &conn_free,
	.row_wise_weighted_sum = &row_wise_weighted_sum,
	.get_n_nonzero = &get_n_nonzero,
    .get_nonzero_indices = &get_nonzero_indices,
	.get_weights = &get_weights,
	.get_delays = &get_delays,
	.get_delay_scale = &get_delay_scale,
	.set_delay_scale = &set_delay_scale,
	.get_n_row = &get_n_row,
	.get_n_col = &get_n_col
};

struct tvb_conn * tvb_conn_new_sparse(
	uint32_t n_rows,
	uint32_t n_cols,
	uint32_t n_nonzero,
	uint32_t *row_offsets,
	uint32_t *col_indices,
	double *weights,
	double *delays
)
{
	struct conn *conn, zero={0};
	if ((conn = tvb_malloc(sizeof(struct conn))) == NULL
	 || (*conn = zero, 0)
	 || (conn->row_offsets = tvb_malloc(sizeof(uint32_t)*(n_rows + 1))) == NULL
	 || (conn->col_indices = tvb_malloc(sizeof(uint32_t)*n_nonzero)) == NULL
	 || (conn->weights = tvb_malloc(sizeof(double)*n_nonzero)) == NULL
	 || (conn->delays = tvb_malloc(sizeof(double)*n_nonzero)) == NULL
	)
	{
		if (conn->row_offsets!=NULL) tvb_free(conn->row_offsets);
		if (conn->col_indices!=NULL) tvb_free(conn->col_indices);
		if (conn->weights!=NULL) tvb_free(conn->weights);
		if (conn!=NULL) tvb_free(conn);
		tvb_err("alloc conn data failed.");
		return NULL;
	}
	conn->n_row = n_rows;
	conn->n_col = n_cols;
	conn->nnz = n_nonzero;
	memcpy(conn->row_offsets, row_offsets, sizeof(uint32_t) * n_nonzero);
	memcpy(conn->col_indices, col_indices, sizeof(uint32_t) * n_nonzero);
	memcpy(conn->weights, weights, sizeof(double) * n_nonzero);
	memcpy(conn->delays, delays, sizeof(double) * n_nonzero);
	conn->delay_scale = 1.0;
	conn->tvb_conn = tvb_conn_defaults;
	conn->tvb_conn.data = conn;
	return &(conn->tvb_conn);
}

struct tvb_conn * tvb_conn_new_dense(
	uint32_t n_rows,
	uint32_t n_cols,
	double *weights,
	double *delays
)
{
	enum tvb_stat stat;
	uint32_t n_nonzeros, *row_offsets, *col_indices;
	double *sparse_weights, *sparse_delays;
	stat = tvb_util_sparse_from_dense(
		n_rows, n_cols, weights, delays, 0.0,
		&n_nonzeros, &row_offsets, &col_indices,
		&sparse_weights, &sparse_delays);
	if (stat != TVB_OK)
	{
		tvb_err("unable to convert dense connectivity to sparse");
		return NULL;
	}
	struct tvb_conn *sparse_conn = tvb_conn_new_sparse(
		n_rows, n_cols, n_nonzeros,
		row_offsets, col_indices, 
		sparse_weights, sparse_delays);
	tvb_free(row_offsets);
	tvb_free(col_indices);
	tvb_free(sparse_delays);
	tvb_free(sparse_weights);
	if (sparse_conn == NULL)
		tvb_err("failed to construct tvb_conn from sparse data set.");
	return sparse_conn;
}

/* }}} */

/* vim: foldmethod=marker
 */
