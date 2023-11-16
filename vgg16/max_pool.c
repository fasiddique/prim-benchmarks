/**
 * @file app.c
 * @brief Template for a Host Application Source File.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <stdint.h>

#include <omp.h>
#include "./support/timer.h"
#include "./dram_ap.h"

uint32_t **feature_mat, **filter_mat;

/**
 * @brief creates a "test file" by filling a buffer of 64MB with pseudo-random values
 * @param nr_elements how many 32-bit elements we want the file to be
 * @return the buffer address
 */
void createMatrix(unsigned int row, unsigned int col)
{
    srand(0);
    feature_mat = (uint32_t **)malloc(row * sizeof(uint32_t *));
    for (int i = 0; i < row; i++)
    {
        feature_mat[i] = (uint32_t *)malloc(col * sizeof(uint32_t));
    }

    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            feature_mat[i][j] = (uint32_t)(rand());
        }
    }

    filter_mat = (uint32_t **)malloc(3 * sizeof(uint32_t *));
    for (int i = 0; i < 3; i++)
    {
        filter_mat[i] = (uint32_t *)malloc(3 * sizeof(uint32_t));
    }

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            filter_mat[i][j] = (uint32_t)(rand());
        }
    }

    // for (int i = 0; i < row; i++)
    // {
    //     for (int j = 0; j < col; j++)
    //     {
    //         printf("%d\t", feature_mat[i][j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n");
    // for (int i = 0; i < 3; i++)
    // {
    //     for (int j = 0; j < 3; j++)
    //     {
    //         printf("%d\t", filter_mat[i][j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n\n");
}

/**
 * @brief compute output in the dram AP
 */

static void max_pool(unsigned int row, unsigned int col, unsigned int filter_length, int bit_len)
{
    int down_sample_col = col / filter_length;
    int down_sample_size = down_sample_col * down_sample_col;
    mm_data_t bitMatrix;
    dram_ap_valloc(&bitMatrix.matrix_A, 0, down_sample_size, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_B, 1, down_sample_size, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_C, 2, down_sample_size, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_D, 3, down_sample_size, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_out, 0, down_sample_size, bit_len);
    int cur_idx = 0;
    for (int i = 0; i < row; i += 2)
    {
        dram_ap_vcpy_split(bitMatrix.matrix_A, bitMatrix.matrix_B, feature_mat[i], col, cur_idx);
        dram_ap_vcpy_split(bitMatrix.matrix_C, bitMatrix.matrix_D, feature_mat[i + 1], col, cur_idx);
        cur_idx += down_sample_col;
    }
    dram_ap_vmax(bitMatrix.matrix_A, bitMatrix.matrix_B, down_sample_size);
    dram_ap_vmax(bitMatrix.matrix_A, bitMatrix.matrix_C, down_sample_size);
    dram_ap_vmax(bitMatrix.matrix_A, bitMatrix.matrix_D, down_sample_size);
    dram_ap_vcpy(bitMatrix.matrix_out, bitMatrix.matrix_A, down_sample_size);
    printf("\n\n+++++++++++++++++Res Max Pool+++++++++++++++++++++++\n");
    for (int i = 0; i < down_sample_size; i++)
    {
        printf("%d\t", bitMatrix.matrix_out[i]);
    }
    printf("\n");
    free(bitMatrix.matrix_A);
    free(bitMatrix.matrix_B);
    free(bitMatrix.matrix_C);
    free(bitMatrix.matrix_D);
    free(bitMatrix.matrix_out);
}

static void printMat(uint32_t *src1_v, unsigned int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d\t", src1_v[i]);
    }
    printf("\n");
}

static void xnor_convolution(unsigned int row, unsigned int col, unsigned int kernel_length, int bit_len)
{
    mm_data_t bitMatrix, filterMatrix;
    // x.y = 2*bitcount(xnor(x,y)) - #bits
    int idx = row;
    int mat_len = row*col;
    //feature matrix
    dram_ap_valloc(&bitMatrix.matrix_A, 0, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_B, 1, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_C, 2, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_D, 3, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_E, 0, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_F, 1, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_G, 2, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_H, 3, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_I, 0, mat_len, bit_len);
    dram_ap_valloc(&bitMatrix.matrix_out, 0, mat_len, bit_len);
    dram_ap_vbrdcst(bitMatrix.matrix_out, 0, mat_len);

    //filter matrix
    dram_ap_valloc(&filterMatrix.matrix_A, 0, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_B, 1, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_C, 2, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_D, 3, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_E, 0, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_F, 1, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_G, 2, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_H, 3, mat_len, bit_len);
    dram_ap_valloc(&filterMatrix.matrix_I, 0, mat_len, bit_len);

    //storing the filter matrix
    dram_ap_vbrdcst(filterMatrix.matrix_A, filter_mat[0][0], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_B, filter_mat[0][1], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_C, filter_mat[0][2], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_D, filter_mat[1][0], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_E, filter_mat[1][1], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_F, filter_mat[1][2], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_G, filter_mat[2][0], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_H, filter_mat[2][1], mat_len);
    dram_ap_vbrdcst(filterMatrix.matrix_I, filter_mat[2][2], mat_len);

    int cur_idx = 0;
    uint32_t *padding_array;
    dram_ap_valloc(&padding_array, 0, col, bit_len);
    dram_ap_vbrdcst(padding_array, 1, col);
    dram_ap_vcpy_split3(bitMatrix.matrix_A, bitMatrix.matrix_B, bitMatrix.matrix_C, padding_array, col, cur_idx, idx);
    for (int i = 0; i < row; i++)
    {   
        int stride = 0;
        if (i != 0) {
            dram_ap_vcpy_split3(bitMatrix.matrix_A, bitMatrix.matrix_B, bitMatrix.matrix_C, feature_mat[i], col, cur_idx, idx);
            stride+=1;
        }
        if (i + stride >= row)
        {
            dram_ap_vcpy_split3(bitMatrix.matrix_D, bitMatrix.matrix_E, bitMatrix.matrix_F, padding_array, col, cur_idx, idx);
            stride+=1;
        }
        else
        {
            dram_ap_vcpy_split3(bitMatrix.matrix_D, bitMatrix.matrix_E, bitMatrix.matrix_F, feature_mat[i + stride], col, cur_idx, idx);
            stride += 1;
        }
        if (i + stride >= row)
        {
            dram_ap_vcpy_split3(bitMatrix.matrix_G, bitMatrix.matrix_H, bitMatrix.matrix_I, padding_array, col, cur_idx, idx);
        }
        else
        {
            dram_ap_vcpy_split3(bitMatrix.matrix_G, bitMatrix.matrix_H, bitMatrix.matrix_I, feature_mat[i + stride], col, cur_idx, idx);
        }
        cur_idx += idx;
    }

    dram_ap_1bit_vxnor(bitMatrix.matrix_A, bitMatrix.matrix_A, filterMatrix.matrix_A, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_B, bitMatrix.matrix_B, filterMatrix.matrix_B, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_C, bitMatrix.matrix_C, filterMatrix.matrix_C, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_D, bitMatrix.matrix_D, filterMatrix.matrix_D, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_E, bitMatrix.matrix_E, filterMatrix.matrix_E, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_F, bitMatrix.matrix_F, filterMatrix.matrix_F, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_G, bitMatrix.matrix_G, filterMatrix.matrix_G, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_H, bitMatrix.matrix_H, filterMatrix.matrix_H, mat_len);
    dram_ap_1bit_vxnor(bitMatrix.matrix_I, bitMatrix.matrix_I, filterMatrix.matrix_I, mat_len);

    dram_ap_9bit_popcount(bitMatrix.matrix_out, mat_len);

    uint32_t *mask_array;
    dram_ap_valloc(&mask_array, 0, mat_len, bit_len);
    dram_ap_vbrdcst(mask_array, 5, mat_len);

    dram_ap_vgt(bitMatrix.matrix_out, mask_array, mat_len);

    free(bitMatrix.matrix_A);
    free(bitMatrix.matrix_B);
    free(bitMatrix.matrix_C);
    free(bitMatrix.matrix_D);
    free(bitMatrix.matrix_E);
    free(bitMatrix.matrix_F);
    free(bitMatrix.matrix_G);
    free(bitMatrix.matrix_H);
    free(bitMatrix.matrix_I);
    free(bitMatrix.matrix_out);

    free(filterMatrix.matrix_A);
    free(filterMatrix.matrix_B);
    free(filterMatrix.matrix_C);
    free(filterMatrix.matrix_D);
    free(filterMatrix.matrix_E);
    free(filterMatrix.matrix_F);
    free(filterMatrix.matrix_G);
    free(filterMatrix.matrix_H);
    free(filterMatrix.matrix_I);

}
// Params ---------------------------------------------------------------------
typedef struct Params
{
    int input_size;
    int n_warmup;
    int n_reps;
    int n_threads;
} Params;

void usage()
{
    fprintf(stderr,
            "\nUsage:  ./program [options]"
            "\n"
            "\nGeneral options:"
            "\n    -h        help"
            "\n    -t <T>    # of threads (default=8)"
            "\n    -w <W>    # of untimed warmup iterations (default=1)"
            "\n    -e <E>    # of timed repetition iterations (default=3)"
            "\n"
            "\nBenchmark-specific options:"
            "\n    -r <I>    square matrix size (default 224)"
            "\n");
}

struct Params input_params(int argc, char **argv)
{
    struct Params p;
    p.input_size = 32; // 16777216;
    p.n_warmup = 1;
    p.n_reps = 3;
    p.n_threads = 5;

    int opt;
    while ((opt = getopt(argc, argv, "hr:w:e:t:")) >= 0)
    {
        switch (opt)
        {
        case 'h':
            usage();
            exit(0);
            break;
        case 'r':
            p.input_size = atoi(optarg);
            break;
        case 'w':
            p.n_warmup = atoi(optarg);
            break;
        case 'e':
            p.n_reps = atoi(optarg);
            break;
        case 't':
            p.n_threads = atoi(optarg);
            break;
        default:
            fprintf(stderr, "\nUnrecognized option!\n");
            usage();
            exit(0);
        }
    }
    assert(p.n_threads > 0 && "Invalid # of ranks!");

    return p;
}

/**
 * @brief Main of the Host Application.
 */
int main(int argc, char **argv)
{

    struct Params p = input_params(argc, argv);
    const unsigned int file_size = p.input_size;

    // Create an input file with arbitrary data.
    createMatrix(file_size, file_size);
    // max_pool(file_size, file_size, 2, 8);
    xnor_convolution(file_size, file_size, 3, 8);
    for (int i = 0; i < file_size; i++)
        free(feature_mat[i]);
    free(feature_mat);
    return 0;
}
