#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
	uint32_t *matrix_A;
	uint32_t *matrix_B;
	uint32_t *matrix_C;
	uint32_t *matrix_D;
	uint32_t *matrix_E;
	uint32_t *matrix_F;
	uint32_t *matrix_G;
	uint32_t *matrix_H;
	uint32_t *matrix_I;
	uint32_t *matrix_out;
	uint32_t matrix_len;
	double *matrix_double;
} mm_data_t;

typedef struct
{
	int *data;
	int size; // matrix_len
} matrix_file_handler;

void dram_ap_valloc(uint32_t **src_v, int group_id, unsigned long long vl, int bit_len)
{
	*src_v = (uint32_t *)malloc(vl * sizeof(uint32_t));
	return;
}

void dram_ap_vcpy(int32_t *dst_v, int32_t *src_v, unsigned int group_id)
{
	for (int i = 0; i < group_id; i++)
	{
		dst_v[i] = src_v[i];
	}
}

void dram_ap_vbrdcst(int32_t *dst_v, int32_t val, unsigned int len)
{
	for (int i = 0; i < len; i++)
	{
		dst_v[i] = val;
	}
}

void dram_ap_vbinarize(int32_t *dst_v, double *src_v, unsigned int len)
{
	for (int i = 0; i < len; i++)
	{
		dst_v[i] = src_v[i]>0?1:0;
	}
}

void dram_ap_vcpy_split(uint32_t *dst_v1, uint32_t *dst_v2, uint32_t *src_v, unsigned int col, unsigned int curr_idx)
{
	for (int i = 0; i < col; i++)
	{
		int stride = i / 2;
		if (i % 2)
		{
			dst_v2[curr_idx + stride] = src_v[i];
		}
		else
		{
			dst_v1[curr_idx + stride] = src_v[i];
		}
	}
}

void dram_ap_vcpy_split3(uint32_t *dst_v1, uint32_t *dst_v2, uint32_t *dst_v3, uint32_t *src_v, unsigned int col, unsigned int curr_idx, unsigned int idx)
{
	int stride_1 = 0, stride_2 = 0, stride_3 = 0;
	for (int i = 0; i < col; i++)
	{
		if (stride_1 == 0)
		{
			dst_v1[curr_idx + stride_1] = 1;
			dst_v1[curr_idx + stride_1 + 1] = src_v[i];
			dst_v2[curr_idx + stride_2] = src_v[i];
			stride_1 += 2;
			stride_2 += 1;
		}
		else if (stride_2 == 0)
		{
			dst_v2[curr_idx + stride_2] = src_v[i];
			stride_2 += 1;
			if (stride_1 < idx)
			{
				dst_v1[curr_idx + stride_1] = src_v[i];
				stride_1 += 1;
			}
		}
		else
		{
			dst_v3[curr_idx + stride_3] = src_v[i];
			stride_3 += 1;
			if (stride_1 < idx)
			{
				dst_v1[curr_idx + stride_1] = src_v[i];
				stride_1 += 1;
			}
			if (stride_2 < idx)
			{
				dst_v2[curr_idx + stride_2] = src_v[i];
				stride_2 += 1;
			}
		}
	}
	if (stride_2 < idx)
	{
		dst_v2[curr_idx + stride_2] = 1;
	}
	if (stride_3 < idx)
	{
		dst_v3[curr_idx + stride_3] = 1;
	}
}

void dram_ap_vmax(uint32_t *src1_v, uint32_t *src2_v, unsigned int col)
{
	for (int i = 0; i < col; i += 1)
	{
		src1_v[i] = src1_v[i] > src2_v[i] ? src1_v[i] : src2_v[i];
	}
}

void dram_ap_vgt(uint32_t *src1_v, uint32_t *src2_v, unsigned int col)
{
	for (int i = 0; i < col; i += 1)
	{
		src1_v[i] = src1_v[i] >= src2_v[i] ? 1 : 0;
	}
}

void dram_ap_1bit_popcount(uint32_t *dst_v, uint32_t *src_v, unsigned int col) {
	for (int i = 0; i < col; i++) {
		if (src_v[i]) dst_v[i] += 1;
	}
}

void dram_ap_1bit_vxnor(uint32_t *dst_v, uint32_t *src1_v, uint32_t *src2_v, unsigned int col)
{
	for (int i = 0; i < col; i += 1)
	{
		dst_v[i] = !(src1_v[i] ^ src2_v[i]);
	}
}