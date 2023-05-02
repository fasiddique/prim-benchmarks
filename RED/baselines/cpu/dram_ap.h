#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../support/common.h"
#include "../../support/timer.h"
typedef struct {
   T *matrix_A;
   T *res;
   int32_t matrix_len;
} mm_data_t;

typedef struct {
   T *data;
   int size; // matrix_len
} matrix_file_handler;

void dram_ap_valloc(T **src_v, int group_id, unsigned long long vl, int bit_len)
{	
	*src_v = (T*) malloc(vl * sizeof(T));
	return;
}

void dram_ap_vcpy(T *dst_v, T *src_v, unsigned int vl, unsigned int group_id)
{
    //memcpy(dst_v, src_v, sizeof(int32_t)*vl);
	for(int i = vl; i < vl+group_id; i++){
	    dst_v[i] = src_v[i];
	}
}

void dram_ap_vredsum(T *scalar, T* src_v, unsigned long long vl, int bit_len)
{
	for (int i = vl; i < vl+bit_len; i++) {
		*scalar += src_v[i];
	}
}