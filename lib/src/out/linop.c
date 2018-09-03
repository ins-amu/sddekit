/* copyright 2016 Apache 2 libtvb authors */

#include "libtvb.h"

struct data
{
	bool on_state;
	uint32_t n_row, n_col;
	double *matrix, *result;
	struct tvb_out tvb_out;
	struct tvb_out_linop tvb_out_linop;
	struct tvb_out *receiver;
};

/* apply {{{ */

static enum tvb_stat out_apply(struct tvb_out *tvb_out, struct tvb_out_sample *samp)
{
	struct data *data = tvb_out->data;
	double *input = data->on_state ? samp->state : samp->output;
	/* apply lin op as mat vec multiply */
	for (uint32_t i_row=0; i_row<data->n_row; i_row++)
	{
		double sum = 0.0
		     , *row = data->matrix + i_row * data->n_col
		     , *vec = input;
		for (uint32_t i_col=0; i_col<data->n_col; i_col++, row++, vec++)
		{
			sum += *row * *vec;
		}
		data->result[i_row] = sum;
	}
	/* prep samp for receiver */
	struct tvb_out_sample proj_samp = { .time = samp->time };
	if (data->on_state)
	{
		proj_samp.n_dim = data->n_row;
		proj_samp.state = data->result;
	}
	else
	{
		proj_samp.n_out = data->n_row;
		proj_samp.output = data->result;
	}
	return data->receiver->apply(data->receiver, &proj_samp);
}

/* }}} */

/* obj free n_byte copy {{{ */

static void data_free(struct data *data)
{
	tvb_free(data->matrix);
	tvb_free(data->result);
	tvb_free(data);
}

static uint32_t data_n_byte(struct data *data)
{
	uint32_t byte_count = sizeof(struct data);
	byte_count += sizeof(double) * (data->n_row + data->n_row * data->n_col);
	return byte_count;
}

static struct data *data_copy(struct data *data)
{
	struct data *copy = tvb_out_linop_new(
			data->on_state, data->n_row, data->n_col,
			data->matrix, data->receiver)->data;
	if (copy == NULL)
		tvb_err("copy linop out failed.");
	return copy;

}

tvb_declare_tag_functions(tvb_out)
tvb_declare_tag_functions(tvb_out_linop)

/* }}} */

/* out get n_dim n_out {{{ */

static uint32_t out_get_n_dim(struct tvb_out *tvb_out)
{
	struct data *data = tvb_out->data;
	return data->on_state ? data->n_row : 0;
}

static uint32_t out_get_n_out(struct tvb_out *tvb_out)
{
	struct data *data = tvb_out->data;
	return data->on_state ? 0 : data->n_row;
}

/* }}} */

/* linop getters {{{ */

#define GET(type, field) \
static type linop_get_ ## field(struct tvb_out_linop *linop) \
{ \
	return ((struct data *) linop->data)->field; \
}

GET(bool , on_state)
GET(uint32_t , n_row)
GET(uint32_t , n_col)
GET(double *, matrix)
GET(struct tvb_out *, receiver)

#undef GET

static struct tvb_out* linop_as_out(struct tvb_out_linop *linop)
{
	return &((struct data *) linop->data)->tvb_out;
}

/* }}} */

/* vtables {{{ */

static struct tvb_out tvb_out_defaults = {
	tvb_declare_tag_vtable(tvb_out),
	.get_n_dim = &out_get_n_dim,
	.get_n_out = &out_get_n_out,
	.apply = &out_apply
};

static struct tvb_out_linop tvb_out_linop_defaults = {
	tvb_declare_tag_vtable(tvb_out_linop),
	.get_on_state = &linop_get_on_state,
	.get_n_row = &linop_get_n_row,
	.get_n_col = &linop_get_n_col,
	.get_matrix = &linop_get_matrix,
	.get_receiver = &linop_get_receiver
};

/* }}} */

/* ctor {{{ */

struct tvb_out_linop *
tvb_out_linop_new(bool on_state, uint32_t n_row, uint32_t n_col,
		 double *matrix, struct tvb_out *receiver)
{
	struct data *data, zero = {0};
	/* alloc & error check {{{ */
	if ((data = tvb_malloc(sizeof(struct data))) == NULL
	 || (*data = zero, n_row == 0 || n_col == 0 || matrix == NULL)
	 || (data->matrix = tvb_malloc(sizeof(double) * n_row * n_col)) == NULL
	 || (data->result = tvb_malloc(sizeof(double) * n_row)) == NULL
	)
	{
		if (data->matrix != NULL) tvb_free(data->matrix);
		if (data != NULL) tvb_free(data);
		tvb_err("alloc linop fail or n_row=0 or n_col=0 or NULL matrix array.");
		return NULL;
	}
	/* }}} */
	data->on_state = on_state;
	data->n_row = n_row;
	data->n_col = n_col;
	data->receiver = receiver;
	memcpy(data->matrix, matrix, sizeof(double) * n_row * n_col);
	data->tvb_out = tvb_out_defaults;
	data->tvb_out_linop = tvb_out_linop_defaults;
	data->tvb_out.data = data->tvb_out_linop.data = data;
	return &data->tvb_out_linop;
}

/* }}} */

/* vim: foldmethod=marker
 */
