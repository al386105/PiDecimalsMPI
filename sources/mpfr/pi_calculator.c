#include <stdio.h>
#include <stdlib.h>
#include <mpfr.h>
#include <time.h>
#include <stdbool.h>
#include "mpi.h"
#include "algorithms/bbp.h"
#include "algorithms/bellard.h"
#include "algorithms/chudnovsky.h"
#include "check_decimals.h"
#include "../common/printer.h"


double gettimeofday();


void check_errors_mpfr(int num_procs, int precision, int num_iterations, int num_threads, int proc_id, int algorithm){
    if (precision <= 0){
        if(proc_id == 0) printf("  Precision should be greater than cero. \n\n");
        MPI_Finalize();
        exit(-1);
    } 
    if (num_iterations < (num_threads * num_procs)){
        if(proc_id == 0){
            printf("  The number of iterations required for the computation is too small to be solved with %d threads and %d procesess. \n", num_threads, num_procs);
            printf("  Try using a greater precision or lower threads/processes number. \n\n");
        }
        MPI_Finalize();
        exit(-1);
    }
}


void calculate_pi_mpfr(int num_procs, int proc_id, int algorithm, int precision, int num_threads, bool print_in_csv_format){
    double execution_time;
    struct timeval t1, t2;
    int num_iterations, decimals_computed, precision_bits; 
    mpfr_t pi;    
    char *algorithm_type;

    //Get init time 
    if(proc_id == 0){
        gettimeofday(&t1, NULL);
    }

    //Set gmp float precision (in bits) and init pi
    precision_bits = precision * 8;
    mpfr_set_default_prec(precision_bits); 
    if (proc_id == 0){
        mpfr_init_set_ui(pi, 0, MPFR_RNDN);
    }

    switch (algorithm)
    {
    case 0:
        num_iterations = precision * 0.84;
        check_errors_mpfr(num_procs, precision, num_iterations, num_threads, proc_id, algorithm);
        algorithm_type = "BBP (Processes and threads distributes the iterations in blocks)";
        bbp_algorithm_mpfr(num_procs, proc_id, pi, num_iterations, num_threads, precision_bits);
        break;

    case 1:
        num_iterations = precision / 3;
        check_errors_mpfr(num_procs, precision, num_iterations, num_threads, proc_id, algorithm);
        algorithm_type = "Bellard (Processes distributes the iterations in blocks and threads do it cyclically)";
        bellard_algorithm_mpfr(num_procs, proc_id, pi, num_iterations, num_threads, precision_bits);
        break;

    case 2:
        num_iterations = (precision + 14 - 1) / 14;  //Division por exceso
        check_errors_mpfr(num_procs, precision, num_iterations, num_threads, proc_id, algorithm);
        algorithm_type = "Chudnovsky (Block distribution by processes and threads and using the simplified mathematical expression)";
        chudnovsky_algorithm_mpfr(num_procs, proc_id, pi, num_iterations, num_threads, precision_bits);
        break;

    default:
        if (proc_id == 0){
            printf("  Algorithm selected is not correct. Try with: \n");
            printf("      algorithm == 0 -> BBP \n");
            printf("      algorithm == 1 -> Bellard \n");
            printf("      algorithm == 2 -> Chudnovsky \n");
            printf("\n");
        } 
        MPI_Finalize();
        exit(-1);
        break;
    }

    //Get time, check decimals, free pi and print the results
    if (proc_id == 0) {  
        gettimeofday(&t2, NULL);
        execution_time = ((t2.tv_sec - t1.tv_sec) * 1000000u +  t2.tv_usec - t1.tv_usec)/1.e6; 
        decimals_computed = check_decimals_mpfr(pi);
        if (print_in_csv_format) { print_results_csv("MPFR", algorithm_type, precision, num_iterations, num_procs, num_threads, decimals_computed, execution_time); } 
        else { print_results("MPFR", algorithm_type, precision, num_iterations, num_procs, num_threads, decimals_computed, execution_time); }
        mpfr_clear(pi);
    }

}

