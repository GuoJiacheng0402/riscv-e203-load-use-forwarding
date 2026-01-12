#include "coremark.h"
#include <stdint.h>
#include <stdio.h>

/* ========================================================================== */
/* Hardware Performance Counter Functions                                    */
/* Read RISC-V machine-mode performance counters with RV32/RV64 support     */
/* ========================================================================== */

/* Read machine-mode cycle counter (mcycle)
 * For RV32: Handle high/low word consistency to avoid rollover issues */
static inline uint64_t get_mcycles(void) {
#if __riscv_xlen == 32
    volatile uint32_t hi, lo, hi2;
    do {
        asm volatile ("csrr %0, mcycleh" : "=r"(hi));
        asm volatile ("csrr %0, mcycle"  : "=r"(lo));
        asm volatile ("csrr %0, mcycleh" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    uint64_t val;
    asm volatile ("csrr %0, mcycle" : "=r"(val));
    return val;
#endif
}

/* Read machine-mode instruction retired counter (minstret)
 * For RV32: Handle high/low word consistency to avoid rollover issues */
static inline uint64_t get_minstret(void) {
#if __riscv_xlen == 32
    volatile uint32_t hi, lo, hi2;
    do {
        asm volatile ("csrr %0, minstreth" : "=r"(hi));
        asm volatile ("csrr %0, minstret"  : "=r"(lo));
        asm volatile ("csrr %0, minstreth" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    uint64_t val;
    asm volatile ("csrr %0, minstret" : "=r"(val));
    return val;
#endif
}

/* Helper function: Safely print 64-bit values
 * Uses hex format for values > 32-bit to avoid printf issues */
void print_uint64(char *label, uint64_t val) {
    if (val <= 0xFFFFFFFF) {
        ee_printf("%s : %lu\n", label, (ee_u32)val);
    } else {
        ee_printf("%s : 0x%08x%08x (hex)\n", label, (ee_u32)(val >> 32), (ee_u32)(val & 0xFFFFFFFF));
    }
}

/* ========================================================================== */
/* CoreMark Standard Validation CRC Values                                   */
/* Each array contains 5 CRC values for different run configurations         */
/* ========================================================================== */
static ee_u16 list_known_crc[]   = {(ee_u16)0xd4b0,(ee_u16)0x3340,(ee_u16)0x6a79,(ee_u16)0xe714,(ee_u16)0xe3c1};
static ee_u16 matrix_known_crc[] = {(ee_u16)0xbe52,(ee_u16)0x1199,(ee_u16)0x5608,(ee_u16)0x1fd7,(ee_u16)0x0747};
static ee_u16 state_known_crc[]  = {(ee_u16)0x5e47,(ee_u16)0x39bf,(ee_u16)0xe5a4,(ee_u16)0x8e3a,(ee_u16)0x8d84};

void *iterate(void *pres) {
    ee_u32 i;
    ee_u16 crc;
    core_results *res=(core_results *)pres;
    ee_u32 iterations=res->iterations;
    res->crc=0;
    res->crclist=0;
    res->crcmatrix=0;
    res->crcstate=0;

    for (i=0; i<iterations; i++) {
        crc=core_bench_list(res,1);
        res->crc=crcu16(crc,res->crc);
        crc=core_bench_list(res,-1);
        res->crc=crcu16(crc,res->crc);
        if (i==0) res->crclist=res->crc;
    }
    return NULL;
}

#if (SEED_METHOD==SEED_ARG)
ee_s32 get_seed_args(int i, int argc, char *argv[]);
#define get_seed(x) (ee_s16)get_seed_args(x,argc,argv)
#define get_seed_32(x) get_seed_args(x,argc,argv)
#else
ee_s32 get_seed_32(int i);
#define get_seed(x) (ee_s16)get_seed_32(x)
#endif

#if (MEM_METHOD==MEM_STATIC)
ee_u8 static_memblk[TOTAL_DATA_SIZE];
#endif
char *mem_name[3] = {"Static","Heap","Stack"};

/* Function: main */
#if MAIN_HAS_NOARGC
MAIN_RETURN_TYPE main(void) {
    int argc=0;
    char *argv[1];
#else
MAIN_RETURN_TYPE main(int argc, char *argv[]) {
#endif

    ee_u16 i,j=0,num_algorithms=0;
    ee_s16 known_id=-1,total_errors=0;
    ee_u16 seedcrc=0;
    CORE_TICKS total_time;
    core_results results[MULTITHREAD];
#if (MEM_METHOD==MEM_STACK)
    ee_u8 stack_memblock[TOTAL_DATA_SIZE*MULTITHREAD];
#endif
    portable_init(&(results[0].port), &argc, argv);

    if (sizeof(struct list_head_s)>128) {
        ee_printf("list_head structure too big for comparable data!\r\n");
        return MAIN_RETURN_VAL;
    }
    results[0].seed1=get_seed(1);
    results[0].seed2=get_seed(2);
    results[0].seed3=get_seed(3);
    results[0].iterations=get_seed_32(4);
#if CORE_DEBUG
    results[0].iterations=1;
#endif

#ifdef CFG_SIMULATION
    results[0].iterations=2;
#else
    /* Set iteration count for performance run
     * Recommended: 2000-4000 iterations to ensure >10s execution time
     * Required by CoreMark specification for valid results */
    #ifndef ITERATIONS
    #define ITERATIONS 2000
    #endif
    results[0].iterations=ITERATIONS;
#endif

    ee_printf ("Start to run coremark for %d iterations\r\n", results[0].iterations);

    results[0].execs=get_seed_32(5);
    if (results[0].execs==0) {
        results[0].execs=ALL_ALGORITHMS_MASK;
    }

    if ((results[0].seed1==0) && (results[0].seed2==0) && (results[0].seed3==0)) { /* validation run */
        results[0].seed1=0;
        results[0].seed2=0;
        results[0].seed3=0x66;
    }
    if ((results[0].seed1==1) && (results[0].seed2==0) && (results[0].seed3==0)) { /* perfromance run */
        results[0].seed1=0x3415;
        results[0].seed2=0x3415;
        results[0].seed3=0x66;
    }

#if (MEM_METHOD==MEM_STATIC)
    results[0].memblock[0]=(void *)static_memblk;
    results[0].size=TOTAL_DATA_SIZE;
    results[0].err=0;
#elif (MEM_METHOD==MEM_MALLOC)
    for (i=0 ; i<MULTITHREAD; i++) {
        ee_s32 malloc_override=get_seed(7);
        if (malloc_override != 0)
            results[i].size=malloc_override;
        else
            results[i].size=TOTAL_DATA_SIZE;
        results[i].memblock[0]=portable_malloc(results[i].size);
        results[i].seed1=results[0].seed1;
        results[i].seed2=results[0].seed2;
        results[i].seed3=results[0].seed3;
        results[i].err=0;
        results[i].execs=results[0].execs;
    }
#elif (MEM_METHOD==MEM_STACK)
    for (i=0 ; i<MULTITHREAD; i++) {
        results[i].memblock[0]=stack_memblock+i*TOTAL_DATA_SIZE;
        results[i].size=TOTAL_DATA_SIZE;
        results[i].seed1=results[0].seed1;
        results[i].seed2=results[0].seed2;
        results[i].seed3=results[0].seed3;
        results[i].err=0;
        results[i].execs=results[0].execs;
    }
#else
#error "Please define a way to initialize a memory block."
#endif

    for (i=0; i<NUM_ALGORITHMS; i++) {
        if ((1<<(ee_u32)i) & results[0].execs)
            num_algorithms++;
    }
    for (i=0 ; i<MULTITHREAD; i++)
        results[i].size=results[i].size/num_algorithms;

    for (i=0; i<NUM_ALGORITHMS; i++) {
        ee_u32 ctx;
        if ((1<<(ee_u32)i) & results[0].execs) {
            for (ctx=0 ; ctx<MULTITHREAD; ctx++)
                results[ctx].memblock[i+1]=(char *)(results[ctx].memblock[0])+results[0].size*j;
            j++;
        }
    }

    for (i=0 ; i<MULTITHREAD; i++) {
        if (results[i].execs & ID_LIST) {
            results[i].list=core_list_init(results[0].size,results[i].memblock[1],results[i].seed1);
        }
        if (results[i].execs & ID_MATRIX) {
            core_init_matrix(results[0].size, results[i].memblock[2], (ee_s32)results[i].seed1 | (((ee_s32)results[i].seed2) << 16), &(results[i].mat) );
        }
        if (results[i].execs & ID_STATE) {
            core_init_state(results[0].size,results[i].seed1,results[i].memblock[3]);
        }
    }

    if (results[0].iterations==0) {
        secs_ret secs_passed=0;
        ee_u32 divisor;
        results[0].iterations=1;
        while (secs_passed < (secs_ret)1) {
            results[0].iterations*=10;
            start_time();
            iterate(&results[0]);
            stop_time();
            secs_passed=time_in_secs(get_time());
        }
        divisor=(ee_u32)secs_passed;
        if (divisor==0)
            divisor=1;
        results[0].iterations*=1+10/divisor;
    }

    /* ================================================= */
    /* Performance Measurement Start                    */
    /* ================================================= */
    uint64_t my_start_cyc  = get_mcycles();
    uint64_t my_start_inst = get_minstret();

    start_time();
#if (MULTITHREAD>1)
    if (default_num_contexts>MULTITHREAD) {
        default_num_contexts=MULTITHREAD;
    }
    for (i=0 ; i<default_num_contexts; i++) {
        results[i].iterations=results[0].iterations;
        results[i].execs=results[0].execs;
        core_start_parallel(&results[i]);
    }
    for (i=0 ; i<default_num_contexts; i++) {
        core_stop_parallel(&results[i]);
    }
#else
    iterate(&results[0]);
#endif
    stop_time();

    /* ================================================= */
    /* Performance Measurement End                      */
    /* ================================================= */
    uint64_t my_end_cyc    = get_mcycles();
    uint64_t my_end_inst   = get_minstret();

    total_time=get_time();

    /* Calculate seed CRC to identify run configuration */
    seedcrc=crc16(results[0].seed1,seedcrc);
    seedcrc=crc16(results[0].seed2,seedcrc);
    seedcrc=crc16(results[0].seed3,seedcrc);
    seedcrc=crc16(results[0].size,seedcrc);

    /* Identify which standard configuration was used */
    switch (seedcrc) {
        case 0x8a02: known_id=0; ee_printf("6k performance run parameters for coremark.\n"); break;
        case 0x7b05: known_id=1; ee_printf("6k validation run parameters for coremark.\n"); break;
        case 0x4eaf: known_id=2; ee_printf("Profile generation run parameters for coremark.\n"); break;
        case 0xe9f5: known_id=3; ee_printf("2K performance run parameters for coremark.\n"); break;
        case 0x18f2: known_id=4; ee_printf("2K validation run parameters for coremark.\n"); break;
        default: total_errors=-1; break;
    }

    if (known_id>=0) {
        for (i=0 ; i<default_num_contexts; i++) {
            results[i].err=0;
            if ((results[i].execs & ID_LIST) && (results[i].crclist!=list_known_crc[known_id])) {
                ee_printf("[%u]ERROR! list crc 0x%04x - should be 0x%04x\n",i,results[i].crclist,list_known_crc[known_id]);
                results[i].err++;
            }
            if ((results[i].execs & ID_MATRIX) && (results[i].crcmatrix!=matrix_known_crc[known_id])) {
                ee_printf("[%u]ERROR! matrix crc 0x%04x - should be 0x%04x\n",i,results[i].crcmatrix,matrix_known_crc[known_id]);
                results[i].err++;
            }
            if ((results[i].execs & ID_STATE) && (results[i].crcstate!=state_known_crc[known_id])) {
                ee_printf("[%u]ERROR! state crc 0x%04x - should be 0x%04x\n",i,results[i].crcstate,state_known_crc[known_id]);
                results[i].err++;
            }
            total_errors+=results[i].err;
        }
    }
    total_errors+=check_data_types();

    /* Calculate total iterations across all contexts */
    uint64_t total_iterations = (uint64_t)default_num_contexts * results[0].iterations;

    ee_printf("CoreMark Size    : %lu\n",(ee_u32)results[0].size);
    ee_printf("Total ticks      : %lu\n",(ee_u32)total_time);
#if HAS_FLOAT
    ee_printf("Total time (secs): %f\n",time_in_secs(total_time));
    if (time_in_secs(total_time) > 0)
        ee_printf("Iterations/Sec   : %f\n",total_iterations/time_in_secs(total_time));
#else
    ee_printf("Total time (secs): %d\n",time_in_secs(total_time));
    if (time_in_secs(total_time) > 0)
        ee_printf("Iterations/Sec   : %d\n",total_iterations/time_in_secs(total_time));
#endif

#ifndef CFG_SIMULATION
    if (time_in_secs(total_time) < 10) {
        ee_printf("ERROR! Must execute for at least 10 secs for a valid result!\n");
        total_errors++;
    }
#endif

    ee_printf("Iterations       : %lu\n",(ee_u32)total_iterations);
    ee_printf("Compiler version : %s\n",COMPILER_VERSION);
    ee_printf("Compiler flags   : %s\n",COMPILER_FLAGS);
#if (MULTITHREAD>1)
    ee_printf("Parallel %s : %d\n",PARALLEL_METHOD,default_num_contexts);
#endif
    ee_printf("Memory location  : %s\n",MEM_LOCATION);
    ee_printf("seedcrc          : 0x%04x\n",seedcrc);

    if (results[0].execs & ID_LIST)
        for (i=0 ; i<default_num_contexts; i++) ee_printf("[%d]crclist        : 0x%04x\n",i,results[i].crclist);
    if (results[0].execs & ID_MATRIX)
        for (i=0 ; i<default_num_contexts; i++) ee_printf("[%d]crcmatrix      : 0x%04x\n",i,results[i].crcmatrix);
    if (results[0].execs & ID_STATE)
        for (i=0 ; i<default_num_contexts; i++) ee_printf("[%d]crcstate       : 0x%04x\n",i,results[i].crcstate);
    for (i=0 ; i<default_num_contexts; i++) ee_printf("[%d]crcfinal       : 0x%04x\n",i,results[i].crc);

    if (total_errors==0) {
        ee_printf("Correct operation validated. See readme.txt for run and reporting rules.\n");
#if HAS_FLOAT
        if (known_id==3) {
            ee_printf("CoreMark 1.0 : %f / %s %s",total_iterations/time_in_secs(total_time),COMPILER_VERSION,COMPILER_FLAGS);
#if defined(MEM_LOCATION) && !defined(MEM_LOCATION_UNSPEC)
            ee_printf(" / %s",MEM_LOCATION);
#else
            ee_printf(" / %s",mem_name[MEM_METHOD]);
#endif
#if (MULTITHREAD>1)
            ee_printf(" / %d:%s",default_num_contexts,PARALLEL_METHOD);
#endif
            ee_printf("\n");
        }
#endif
    }
    if (total_errors>0) ee_printf("Errors detected\n");
    if (total_errors<0) ee_printf("Cannot validate operation for these seed values.\n");

    /* ========================================================================== */
    /* Enhanced Performance Analysis Output                                      */
    /* Provides detailed metrics for pipeline optimization and debugging         */
    /* ========================================================================== */

    uint64_t my_total_cyc  = my_end_cyc - my_start_cyc;
    uint64_t my_total_inst = my_end_inst - my_start_inst;

    /* CoreMark/MHz Calculation
     * Formula: (Total_Iterations * 1,000,000) / Cycle_Count
     * This normalized metric allows cross-platform comparison
     * independent of actual CPU frequency */
    float coremark_per_mhz = 0.0f;
    if (my_total_cyc > 0) {
        coremark_per_mhz = ((float)total_iterations * 1000000.0f) / (float)my_total_cyc;
    }

    float iterations_per_sec = 0.0f;
    if (time_in_secs(total_time) > 0) {
        iterations_per_sec = (float)total_iterations / time_in_secs(total_time);
    }

    /* Calculate actual CPU frequency from measured time */
    float actual_freq_mhz = 0.0f;
    if (time_in_secs(total_time) > 0) {
        actual_freq_mhz = ((float)my_total_cyc / time_in_secs(total_time)) / 1000000.0f;
    }

    /* Calculate average cycles and instructions per iteration */
    float cycles_per_iter = 0.0f;
    float insts_per_iter = 0.0f;
    if (total_iterations > 0) {
        cycles_per_iter = (float)my_total_cyc / (float)total_iterations;
        insts_per_iter = (float)my_total_inst / (float)total_iterations;
    }

    /* Calculate MIPS (Million Instructions Per Second) */
    float mips = 0.0f;
    if (time_in_secs(total_time) > 0) {
        mips = ((float)my_total_inst / time_in_secs(total_time)) / 1000000.0f;
    }

    ee_printf ("\n========================================================\n");
    ee_printf ("[Experiment 4 - Pipeline Performance Analysis]\n");
    ee_printf ("========================================================\n");

    ee_printf ("\n--- Raw Hardware Counters ---\n");
    print_uint64("Total Cycles (mcycle)      ", my_total_cyc);
    print_uint64("Total Instructions (minstret)", my_total_inst);
    ee_printf ("Total Iterations            : %lu\n", (ee_u32)total_iterations);
    ee_printf ("Total Time (seconds)        : %f\n", time_in_secs(total_time));

    if (my_total_cyc > 0 && my_total_inst > 0) {
        ee_printf ("\n--- Core Performance Metrics ---\n");
        ee_printf ("CPI (Cycles Per Instruction): %f\n", (double)my_total_cyc / my_total_inst);
        ee_printf ("IPC (Instructions Per Cycle): %f\n", (double)my_total_inst / my_total_cyc);
        ee_printf ("Actual CPU Frequency        : %.2f MHz\n", actual_freq_mhz);
        ee_printf ("MIPS (Million Inst/Sec)     : %.2f\n", mips);

        ee_printf ("\n--- Benchmark Efficiency Metrics ---\n");
        ee_printf ("Iterations/Second           : %.2f\n", iterations_per_sec);
        ee_printf ("CoreMark/MHz (Normalized)   : %.4f\n", coremark_per_mhz);
        ee_printf ("Cycles per Iteration        : %.0f\n", cycles_per_iter);
        ee_printf ("Instructions per Iteration  : %.0f\n", insts_per_iter);

        ee_printf ("\n--- Pipeline Optimization Indicators ---\n");
        if ((double)my_total_inst / my_total_cyc > 0.8) {
            ee_printf ("IPC Status                  : EXCELLENT (>0.8, approaching ideal)\n");
        } else if ((double)my_total_inst / my_total_cyc > 0.5) {
            ee_printf ("IPC Status                  : GOOD (0.5-0.8, moderate pipeline efficiency)\n");
        } else if ((double)my_total_inst / my_total_cyc > 0.3) {
            ee_printf ("IPC Status                  : FAIR (0.3-0.5, room for improvement)\n");
        } else {
            ee_printf ("IPC Status                  : NEEDS OPTIMIZATION (<0.3, significant stalls)\n");
        }

        ee_printf ("\n--- Module Execution Status ---\n");
        if (results[0].execs & ID_LIST) {
            ee_printf ("List Benchmark              : EXECUTED\n");
            ee_printf ("  CRC Value                 : 0x%04x\n", results[0].crclist);
            if (known_id >= 0) {
                ee_printf ("  Expected CRC              : 0x%04x\n", list_known_crc[known_id]);
                ee_printf ("  Status                    : %s\n",
                    (results[0].crclist == list_known_crc[known_id]) ? "PASS ✓" : "FAIL ✗");
            }
        } else {
            ee_printf ("List Benchmark              : SKIPPED\n");
        }
        if (results[0].execs & ID_MATRIX) {
            ee_printf ("Matrix Benchmark            : EXECUTED\n");
            ee_printf ("  CRC Value                 : 0x%04x\n", results[0].crcmatrix);
            if (known_id >= 0) {
                ee_printf ("  Expected CRC              : 0x%04x\n", matrix_known_crc[known_id]);
                ee_printf ("  Status                    : %s\n",
                    (results[0].crcmatrix == matrix_known_crc[known_id]) ? "PASS ✓" : "FAIL ✗");
                if (results[0].crcmatrix != matrix_known_crc[known_id]) {
                    ee_printf ("  Note: Matrix test is called via calc_func() during list sort\n");
                }
            }
        } else {
            ee_printf ("Matrix Benchmark            : SKIPPED\n");
        }
        if (results[0].execs & ID_STATE) {
            ee_printf ("State Machine Benchmark     : EXECUTED\n");
            ee_printf ("  CRC Value                 : 0x%04x\n", results[0].crcstate);
            if (known_id >= 0) {
                ee_printf ("  Expected CRC              : 0x%04x\n", state_known_crc[known_id]);
                ee_printf ("  Status                    : %s\n",
                    (results[0].crcstate == state_known_crc[known_id]) ? "PASS ✓" : "FAIL ✗");
                if (results[0].crcstate != state_known_crc[known_id]) {
                    ee_printf ("  Note: State test is called via calc_func() during list sort\n");
                }
            }
        } else {
            ee_printf ("State Machine Benchmark     : SKIPPED\n");
        }

        ee_printf ("\n--- Validation Result ---\n");
        ee_printf ("CRC Validation              : %s\n", (total_errors == 0) ? "PASS ✓" : "FAIL ✗");
        if (total_errors > 0) {
            ee_printf ("Total Errors                : %d\n", total_errors);
        }

        ee_printf ("\n--- Suggestions for Pipeline Optimization ---\n");
        if ((double)my_total_inst / my_total_cyc < 0.5) {
            ee_printf ("• Consider optimizing data hazards and control hazards\n");
            ee_printf ("• Check for load-use delays and branch prediction misses\n");
            ee_printf ("• Implement forwarding paths if not present\n");
        }
        if ((double)my_total_inst / my_total_cyc < 0.3) {
            ee_printf ("• Pipeline may have significant structural hazards\n");
            ee_printf ("• Consider adding more pipeline stages or improving hazard handling\n");
        }

        if (total_errors > 0) {
            ee_printf ("\n--- CRC Validation Failure Diagnosis ---\n");
            ee_printf ("Possible causes for CRC mismatch:\n");
            ee_printf ("1. Hardware Implementation Issues:\n");
            ee_printf ("   • Data hazard handling (RAW/WAR/WAW)\n");
            ee_printf ("   • Branch prediction or control flow\n");
            ee_printf ("   • Memory load/store ordering\n");
            ee_printf ("   • Arithmetic operation correctness\n");
            ee_printf ("2. Compiler/Toolchain Issues:\n");
            ee_printf ("   • Try different optimization levels (-O0, -O1, -O2)\n");
            ee_printf ("   • Disable aggressive optimizations one by one\n");
            ee_printf ("   • Check for ABI compliance\n");
            ee_printf ("3. Memory/Alignment Issues:\n");
            ee_printf ("   • Verify data alignment requirements\n");
            ee_printf ("   • Check stack size (currently using MEM_STACK mode)\n");
            ee_printf ("4. Debug Steps:\n");
            ee_printf ("   • Enable CORE_DEBUG mode to trace execution\n");
            ee_printf ("   • Compare with a known-good reference implementation\n");
            ee_printf ("   • Run with validation seeds (0,0,0x66) first\n");
        }
    } else {
        ee_printf ("\nError: Counters invalid (0). Check HW support.\n");
    }
    ee_printf ("========================================================\n");

    /* Cleanup - Must be after performance output to avoid UART shutdown */
#if (MEM_METHOD==MEM_MALLOC)
    for (i=0 ; i<MULTITHREAD; i++) portable_free(results[i].memblock[0]);
#endif
    portable_fini(&(results[0].port));

    return MAIN_RETURN_VAL;
}
