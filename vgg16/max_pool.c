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

static uint32_t **feature_mat;

/**
 * @brief creates a "test file" by filling a buffer of 64MB with pseudo-random values
 * @param nr_elements how many 32-bit elements we want the file to be
 * @return the buffer address
 */
void *create_test_file(unsigned int nr_elements)
{
    srand(0);
    feature_mat = (uint32_t **)malloc(nr_elements * sizeof(uint32_t *));
    for (int i = 0; i < nr_elements; i++)
    {
        feature_mat[i] = (uint32_t *)malloc(nr_elements * sizeof(uint32_t));
    }

    for (int i = 0; i < nr_elements; i++)
    {
        for (int j = 0; j < nr_elements; j++)
        {
            feature_mat[i][j] = (uint32_t)(rand());
        }
    }
    for (int i = 0; i < nr_elements; i++)
    {
        for (int j = 0; j < nr_elements; j++)
        {
            printf("%d\t", feature_mat[i][j]);
        }
        printf("\n");
    }
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
    cur_idx -= down_sample_col;
    dram_ap_vmax(bitMatrix.matrix_A, bitMatrix.matrix_B, cur_idx);
    dram_ap_vmax(bitMatrix.matrix_A, bitMatrix.matrix_C, cur_idx);
    dram_ap_vmax(bitMatrix.matrix_A, bitMatrix.matrix_D, cur_idx);
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
    create_test_file(file_size);
    max_pool(file_size, file_size, 2, 8);
    for (int i = 0; i < file_size; i++)
        free(feature_mat[i]);
    free(feature_mat);
    return 0;
}
