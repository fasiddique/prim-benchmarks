#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
   uint32_t *matrix_A;
   uint32_t *matrix_B;
   uint32_t *matrix_C;
   uint32_t *matrix_D;
   uint32_t *matrix_out;
   uint32_t matrix_len;
} mm_data_t;

typedef struct {
   int *data;
   int size; // matrix_len
} matrix_file_handler;

void dram_ap_valloc(uint32_t **src_v, int group_id, unsigned long long vl, int bit_len)
{	
	*src_v = (uint32_t*) malloc(vl * sizeof(uint32_t));
	return;
}

void dram_ap_vcpy(int32_t *dst_v, int32_t *src_v, unsigned int vl, unsigned int group_id)
{
	for(int i = 0; i < group_id; i++){
	    dst_v[vl+i] = src_v[i];
	}
}

void dram_ap_vcpy_split(uint32_t *dst_v1, uint32_t *dst_v2, uint32_t *src_v, unsigned int col)
{
	for(int i = 0; i < col; i++){
		if(i%2) {
			dst_v2[i/2] = src_v[i];
		} else {
			dst_v1[i/2] = src_v[i];
		}
	}
}

void dram_ap_vmax(uint32_t *src1_v, uint32_t *src2_v, unsigned int col)
{
	for (int i = 0; i < col; i+=1) {
		src1_v[i] = src1_v[i]>src2_v[i]?src1_v[i]:src2_v[i];
	}
	// printf("\n\n+++++++++++++++++1st Mat+++++++++++++++++++++++\n");
	// for (int i = 0; i < col; i++)
    // {
    //     printf("%d\t", src1_v[i]);
    // }
}