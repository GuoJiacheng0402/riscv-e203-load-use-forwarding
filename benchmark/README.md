# CoreMark Benchmark with Enhanced Pipeline Performance Analysis

![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)
![RISC-V](https://img.shields.io/badge/RISC--V-Compatible-green.svg)
![CoreMark](https://img.shields.io/badge/CoreMark-1.0-orange.svg)
![Educational](https://img.shields.io/badge/Purpose-Educational-purple.svg)


**Educational Resource for Computer Architecture Course (Undergraduate Level)**

This is an enhanced version of the EEMBC CoreMark benchmark, specifically designed for computer architecture education with detailed performance analysis and debugging features.


---

## 🎯 Overview

This enhanced CoreMark implementation adds **comprehensive performance metrics** to help students understand:
- Pipeline efficiency (CPI/IPC)
- Hardware performance counters
- Benchmark validation
- Performance debugging

### What is CoreMark?

CoreMark® is an industry-standard benchmark that measures the performance of:
- **Central Processing Units (CPUs)**
- **Embedded Microcontrollers (MCUs)**

It tests three core algorithms commonly found in embedded systems:
1. **List Processing** - Linked list operations (find, sort, remove)
2. **Matrix Operations** - Integer matrix multiplication
3. **State Machine** - Finite state machine for input validation

---

## ✨ Key Features

### Standard CoreMark Features
- ✅ EEMBC CoreMark 1.0 compliant
- ✅ CRC validation for correctness
- ✅ Three benchmark algorithms (List, Matrix, State)
- ✅ Multiple memory modes (Static, Heap, Stack)

### Enhanced Features for Education

#### 📊 Detailed Performance Metrics
- **Raw Hardware Counters**
  - Total Cycles (mcycle CSR)
  - Total Instructions Retired (minstret CSR)
  - Execution Time

- **Core Performance Metrics**
  - CPI (Cycles Per Instruction)
  - IPC (Instructions Per Cycle)
  - Actual CPU Frequency
  - MIPS (Million Instructions Per Second)

- **Benchmark Efficiency**
  - Iterations/Second
  - CoreMark/MHz (normalized score)
  - Cycles per Iteration
  - Instructions per Iteration

#### 🔍 Automatic Performance Analysis
- IPC status rating (EXCELLENT/GOOD/FAIR/NEEDS OPTIMIZATION)
- Per-module CRC validation status
- Pipeline optimization suggestions
- Diagnostic guidance for CRC failures

#### 🎓 Student-Friendly Output
- Clear, structured output format
- Visual indicators (✓/✗) for pass/fail
- Detailed explanations of metrics
- Debugging hints and suggestions

------

## 🚀 Key Innovations

### 1. Educational Output Format

**Before (Standard CoreMark):**
```
CoreMark 1.0 : 36.347035 / GCC14.2.1 ... / STACK
```

**After (Our Enhancement):**
```
========================================================
[Pipeline Performance Analysis]
========================================================

--- Core Performance Metrics ---
CPI (Cycles Per Instruction): 1.919271
IPC (Instructions Per Cycle): 0.521031
IPC Status                  : GOOD (0.5-0.8)

--- Module Execution Status ---
List Benchmark              : EXECUTED
  Status                    : PASS ✓
Matrix Benchmark            : EXECUTED
  Status                    : PASS ✓

--- Suggestions for Pipeline Optimization ---
• Consider optimizing data hazards and control hazards
• Check for load-use delays and branch prediction misses
```

### 2. Diagnostic Intelligence

When things fail, we explain **why** and **how to fix it**:

```
--- CRC Validation Failure Diagnosis ---
Possible causes for CRC mismatch:
1. Hardware Implementation Issues:
   • Data hazard handling (RAW/WAR/WAW)
   • Branch prediction or control flow
   [...]
4. Debug Steps:
   • Enable CORE_DEBUG mode to trace execution
   • Compare with a known-good reference implementation
   • Run with validation seeds (0,0,0x66) first
```

### 3. Real Understanding

We explain **how CoreMark actually works**, not just what it does:

- Matrix and State tests are **embedded in list sorting**
- Called through `calc_func()` during `cmp_complex()`
- Creates realistic, data-dependent control flow
- This is **crucial** for understanding results

---

## 🚀 Quick Start

### Prerequisites

- **RISC-V Toolchain** (GCC recommended)
- **Hardware or Simulator** supporting:
  - `mcycle` and `minstret` CSRs
  - Basic RISC-V instructions (RV32I/RV64I minimum)

### Build and Run

```bash
# Standard build
make compile run

# With specific iteration count (recommended: 2000-4000)
make compile ITERATIONS=2000 run

# Different optimization levels
make compile XCFLAGS="-O2" run
make compile XCFLAGS="-O3" run
```

### Expected Output

```
CoreMark Size    : 666
Total ticks      : 220093603
...
Correct operation validated. See readme.txt for run and reporting rules.

========================================================
[Experiment 4 - Pipeline Performance Analysis]
========================================================

--- Raw Hardware Counters ---
Total Cycles (mcycle)       : 220093672
Total Instructions (minstret) : 114675651
Total Iterations            : 500
Total Time (seconds)        : 13.756280

--- Core Performance Metrics ---
CPI (Cycles Per Instruction): 1.919271
IPC (Instructions Per Cycle): 0.521031
Actual CPU Frequency        : 16.00 MHz
MIPS (Million Inst/Sec)     : 8.34

--- Module Execution Status ---
List Benchmark              : EXECUTED
  CRC Value                 : 0xe714
  Expected CRC              : 0xe714
  Status                    : PASS ✓
Matrix Benchmark            : EXECUTED
  CRC Value                 : 0x1fd7
  Expected CRC              : 0x1fd7
  Status                    : PASS ✓
State Machine Benchmark     : EXECUTED
  CRC Value                 : 0x8e3a
  Expected CRC              : 0x8e3a
  Status                    : PASS ✓

--- Validation Result ---
CRC Validation              : PASS ✓
```

---

## 📖 Understanding the Output

### CPI (Cycles Per Instruction)

**Formula:** `Total Cycles / Total Instructions`

**Interpretation:**
- **Lower is better**
- Ideal: ~1.0 for single-issue in-order pipeline
- Reality: 1.5-3.0 for complex embedded processors
- >3.0: Indicates significant pipeline stalls

**Example:** CPI = 1.92 means each instruction takes an average of 1.92 cycles.

### IPC (Instructions Per Cycle)

**Formula:** `Total Instructions / Total Cycles`

**Interpretation:**
- **Higher is better** (inverse of CPI)
- Rating scale:
  - IPC > 0.8: **EXCELLENT** ✨
  - IPC 0.5-0.8: **GOOD** ✅
  - IPC 0.3-0.5: **FAIR** ⚠️
  - IPC < 0.3: **NEEDS OPTIMIZATION** ❌

**Example:** IPC = 0.52 means ~0.52 instructions complete per cycle on average.

### CoreMark/MHz

**Formula:** `(Total Iterations × 1,000,000) / Total Cycles`

**Interpretation:**
- Frequency-normalized benchmark score
- Allows fair comparison across different CPU frequencies
- Reflects **instruction efficiency** rather than raw speed
- Higher values indicate better architecture/implementation

### MIPS (Million Instructions Per Second)

**Formula:** `Total Instructions / Execution Time / 1,000,000`

**Interpretation:**
- Traditional throughput metric
- Depends on both clock frequency and IPC
- **Note:** Not the same as "MIPS architecture"

---

## 🔧 Common Issues and Solutions

### Issue 1: CRC Validation Failures

**Symptoms:**
```
[0]ERROR! matrix crc 0x6e0a - should be 0x1fd7
[0]ERROR! state crc 0x9d67 - should be 0x8e3a
```

**Possible Causes & Solutions:**

#### 1. Hardware Implementation Issues (Most Common)

**Data Hazards:**
- Check RAW (Read After Write) hazard handling
- Verify forwarding paths (EX→EX, MEM→EX)
- Test with `nop` instructions between dependent operations

**Control Hazards:**
- Verify branch prediction logic
- Check pipeline flush on misprediction
- Test with simple non-branching code first

**Memory Hazards:**
- Verify load-use delay handling
- Check store-to-load forwarding
- Test memory alignment requirements

#### 2. Compiler Optimization Issues

**Debug Steps:**
```bash
# Step 1: Try without optimization
make compile XCFLAGS="-O0" run

# Step 2: Try conservative optimization
make compile XCFLAGS="-O1" run

# Step 3: Try standard optimization
make compile XCFLAGS="-O2" run

# Step 4: Disable specific optimizations
make compile XCFLAGS="-O2 -fno-inline -fno-unroll-loops" run
```

#### 3. Enable Debug Mode

Modify `core_portme.h`:
```c
#define CORE_DEBUG 1
```

This will output intermediate values to help locate the exact failure point.

#### 4. Validate Hardware First

Test with known-good validation seeds:
```c
// In core_main.c
results[0].seed1 = 0;
results[0].seed2 = 0;
results[0].seed3 = 0x66;
```

### Issue 2: Performance Counters Return Zero

**Symptoms:**
```
Error: Counters invalid (0). Check HW support.
```

**Solutions:**
1. Verify `mcycle` and `minstret` CSR implementation
2. Check if counters are enabled in supervisor mode
3. Confirm RISC-V privilege mode configuration

### Issue 3: Execution Time < 10 seconds

**Symptoms:**
```
ERROR! Must execute for at least 10 secs for a valid result!
```

**Solution:**
Increase iteration count:
```bash
make compile ITERATIONS=4000 run
```

**Note:** This is a CoreMark requirement for valid performance results.

---

## 🎓 How CoreMark Works

### The Clever Design: Embedded Test Execution

CoreMark has a **unique architecture** that surprises many students:

#### ❌ What You Might Expect (But Wrong!)

```c
for (i=0; i<iterations; i++) {
    core_bench_list();
    core_bench_matrix();   // NOT called directly!
    core_bench_state();    // NOT called directly!
}
```

#### ✅ What Actually Happens

```
iterate()
  └─> core_bench_list(res, 1)
       └─> core_list_mergesort(..., cmp_complex, ...)
            └─> cmp_complex()
                 └─> calc_func()
                      ├─> core_bench_state()  ← Called during list sort!
                      └─> core_bench_matrix() ← Called during list sort!
```

### Why This Design?

1. **Realistic Workload**: Mimics real embedded applications where complex operations are interleaved
2. **Cache Behavior**: Tests instruction and data cache with unpredictable access patterns
3. **Branch Prediction**: Exercises branch predictor with data-dependent branches
4. **Prevents Over-Optimization**: Compiler cannot easily optimize the entire benchmark

### Execution Flow

```
main()
 ├─> Initialize memory and data structures
 │    ├─> core_list_init()    - Create linked list
 │    ├─> core_init_matrix()  - Allocate matrices
 │    └─> core_init_state()   - Setup state machine input
 │
 ├─> Start performance counters
 │    ├─> mcycle_start = CSR_READ(mcycle)
 │    └─> minstret_start = CSR_READ(minstret)
 │
 ├─> iterate() × N times
 │    └─> core_bench_list() × 2 per iteration
 │         ├─> List operations (find, reverse, sort)
 │         └─> During sort: calc_func() calls matrix/state
 │
 ├─> Stop performance counters
 │    ├─> mcycle_end = CSR_READ(mcycle)
 │    └─> minstret_end = CSR_READ(minstret)
 │
 ├─> Validate CRC values
 └─> Print performance analysis
```

### The calc_func() Mechanism

Located in `core_list_join.c`, this function is the **key to understanding CoreMark**:

```c
ee_s16 calc_func(ee_s16 *pdata, core_results *res) {
    ee_s16 flag = data & 0x7;  // Extract operation type from data

    switch (flag) {
        case 0:
            // Call state machine benchmark
            retval = core_bench_state(...);
            if (res->crcstate == 0)
                res->crcstate = retval;
            break;
        case 1:
            // Call matrix benchmark
            retval = core_bench_matrix(...);
            if (res->crcmatrix == 0)
                res->crcmatrix = retval;
            break;
    }
    return retval;
}
```

**Key Points:**
- Called during `cmp_complex()` when sorting the list
- Operation type determined by list element data
- Matrix and State CRCs captured on first execution
- This creates **data-dependent control flow** that's hard to predict

---

## 🏗️ Pipeline Optimization Guide

### Understanding Your Baseline

**Example Baseline Results:**
```
CPI: 2.50
IPC: 0.40
IPC Status: FAIR (0.3-0.5, room for improvement)
```

### Optimization Strategies

#### 1. Data Hazard Optimization

**Goal:** Reduce RAW hazard stalls

**Techniques:**
- **Forwarding/Bypassing:** Forward results from EX and MEM stages
- **Out-of-Order Execution:** Execute independent instructions during stalls
- **Scoreboarding:** Track register dependencies

**Expected Improvement:** CPI reduction of 20-30%

#### 2. Control Hazard Optimization

**Goal:** Reduce branch misprediction penalty

**Techniques:**
- **Branch Prediction:** Implement 2-bit saturating counters
- **Branch Target Buffer (BTB):** Cache branch target addresses
- **Early Branch Resolution:** Calculate branch in ID stage
- **Delayed Branching:** Execute branch delay slot

**Expected Improvement:** CPI reduction of 10-20%

#### 3. Structural Hazard Elimination

**Goal:** Remove resource conflicts

**Techniques:**
- **Separate Instruction/Data Caches:** Avoid memory port conflicts
- **Pipelined Multiplier:** Multi-cycle operations don't stall pipeline
- **Dual-Port Register File:** Simultaneous read/write

**Expected Improvement:** CPI reduction of 5-15%

### Measuring Improvement

**Before Optimization:**
```
CPI: 2.50
CoreMark/MHz: 1.50
```

**After Optimization:**
```
CPI: 1.25  (50% reduction!)
CoreMark/MHz: 3.00  (2× improvement!)
```

**Speedup Calculation:**
```
Speedup = CPI_old / CPI_new = 2.50 / 1.25 = 2.0×
```

### Realistic Targets

| Pipeline Type | Expected CPI | Expected IPC |
|--------------|-------------|-------------|
| Simple 5-stage (no forwarding) | 2.5 - 3.5 | 0.29 - 0.40 |
| 5-stage with forwarding | 1.5 - 2.0 | 0.50 - 0.67 |
| 5-stage with forwarding + branch prediction | 1.2 - 1.5 | 0.67 - 0.83 |
| Superscalar (2-way) | 0.8 - 1.2 | 0.83 - 1.25 |

---

## 🐛 Debugging Tips

### Enable Verbose Output

```c
// In core_portme.h
#define CORE_DEBUG 1
```

This enables detailed tracing of:
- List operations
- Matrix calculations
- State machine transitions
- Intermediate CRC values

### Test Individual Modules

You can temporarily disable modules to isolate issues:

```c
// In core_list_join.c, calc_func()
switch (flag) {
    case 0:
        retval = core_bench_state(...);
        break;
    case 1:
        retval = 0;  // Temporarily disable matrix
        break;
}
```

### Compare with Reference

Run the same code on:
1. **Your hardware implementation**
2. **Spike (RISC-V ISA Simulator)** - Known correct behavior
3. **QEMU** - Another reference

Compare outputs step-by-step to find divergence.

### Use Smaller Iteration Counts

For debugging, use fewer iterations:
```c
#define ITERATIONS 10  // Instead of 2000
```

This makes it easier to:
- Trace execution manually
- Compare intermediate values
- Identify where results diverge

---

## 📊 Performance Comparison Table

### Example Results (For Reference)

| Implementation | CPI | IPC | CoreMark/MHz | Notes |
|----------------|-----|-----|--------------|-------|
| Simple 5-stage (baseline) | 2.50 | 0.40 | 1.50 | No forwarding |
| + EX forwarding | 2.00 | 0.50 | 1.88 | 25% improvement |
| + MEM forwarding | 1.70 | 0.59 | 2.21 | 18% additional |
| + Branch prediction | 1.30 | 0.77 | 2.88 | 30% additional |
| 2-way superscalar | 0.90 | 1.11 | 4.17 | 45% additional |

*Note: Actual results will vary based on implementation details*

---

## 🔗 References

### Official CoreMark Resources
- [EEMBC CoreMark Official Site](https://www.eembc.org/coremark/)
- [CoreMark GitHub Repository](https://github.com/eembc/coremark)
- [CoreMark Whitepaper](https://www.eembc.org/techlit/articles/coremark-whitepaper.pdf)

### RISC-V Resources
- [RISC-V ISA Specification](https://riscv.org/technical/specifications/)
- [RISC-V Privileged Spec](https://github.com/riscv/riscv-isa-manual) (for CSR details)

### Computer Architecture Textbooks
- **Hennessy & Patterson:** "Computer Architecture: A Quantitative Approach"
- **Patterson & Hennessy:** "Computer Organization and Design: The Hardware/Software Interface"

### Performance Counter Documentation
- **mcycle:** Counts clock cycles (CSR 0xC00/0xC80)
- **minstret:** Counts retired instructions (CSR 0xC02/0xC82)
- For RV32: Use `mcycleh` and `minstreth` for upper 32 bits

---

## 📝 License

This project is based on EEMBC CoreMark, which is licensed under the Apache License 2.0.

The enhanced performance analysis features are provided for educational purposes.

---

## 💡 Tips for study

### Understanding the Results

**Q: Why is my IPC so low?**

A: This is normal! Real-world code has:
- Data dependencies (need to wait for previous results)
- Memory latency (loads take multiple cycles)
- Branch mispredictions (wrong path executed)
- Resource conflicts (structural hazards)

**Q: Can IPC be > 1.0?**

A: Yes! With superscalar or out-of-order execution:
- Multiple instructions issue per cycle
- IPC can exceed 2.0 in modern processors
- But harder to implement in hardware

**Q: Why does the benchmark call Matrix/State indirectly?**

A: To create realistic, unpredictable control flow:
- Tests branch prediction under real conditions
- Prevents compiler from over-optimizing
- Mimics actual application behavior

### Common Misconceptions

**❌ MIPS = Performance**
- Different architectures have different instruction counts for the same task
- RISC vs CISC comparison issues
- Use CoreMark/MHz for fair comparison

**❌ Higher Clock = Better Performance**
- A 1 GHz processor with IPC=0.5 performs worse than 500 MHz with IPC=1.0
- Architecture efficiency matters more than raw frequency
- Power consumption increases with frequency

**❌ CRC Failure = Code Bug**
- Often indicates hardware implementation issue
- Check pipeline hazards first
- Compiler optimizations can expose HW bugs

---

## 🎯 Learning Objectives

After completing exercises with this benchmark, you are expexted to understand:

1. **Performance Metrics:**
   - How to measure processor performance
   - Relationship between CPI, IPC, and throughput
   - Importance of frequency-normalized metrics

2. **Pipeline Hazards:**
   - Data hazards and forwarding
   - Control hazards and branch prediction
   - Structural hazards and resource conflicts

3. **Hardware-Software Interaction:**
   - How compiler optimizations affect performance
   - Role of hardware performance counters
   - Validation through CRC checking

4. **Real-World Benchmarking:**
   - Why standard benchmarks matter
   - How to interpret benchmark results
   - Limitations of synthetic benchmarks

---

**Happy Benchmarking! 🚀**

*Remember: Understanding WHY performance differs is more important than the numbers themselves.*

