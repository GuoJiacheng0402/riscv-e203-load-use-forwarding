# E203 RISC-V Processor: Load-Use Data Forwarding Optimization

[![RISC-V](https://img.shields.io/badge/ISA-RISC--V-blue.svg)](https://riscv.org/)
[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](LICENSE)
[![University](https://img.shields.io/badge/University-Your%20University-9cf.svg)](SCUT)

*Developed by **Jiacheng Guo**.*

A final project for the *Computer Architecture & Organization* course (Experiment 4), implementing hardware data forwarding to optimize the E203 RISC-V processor pipeline.

---

## 📖 Project Overview

This repository contains a Verilog implementation of **Load-Use Data Forwarding** for the Hummingbird E203 two-stage pipelined processor. By introducing a hardware forwarding path, this project eliminates pipeline stalls caused by Read-After-Write (RAW) dependencies, demonstrating a key microarchitectural optimization technique.

---

## 📊 Performance Results

We benchmarked the optimization using **CoreMark 1.0** (500 iterations) on an FPGA platform.

| Metric | Baseline | Optimized (Forwarding) | Improvement |
|--------|--------------------|--------------------|-------------|
| **Total Cycles** | 220,093,603 | **212,707,102** | **-3.35%** (Faster) ⬇️ |
| **CPI** | 1.919 | **1.855** | **-3.35%** (Better) ⬇️ |
| **IPC** | 0.521 | **0.539** | **+3.47%** (Better) ⬆️ |
| **CoreMark Score** | 36.35 | **37.61** | **+3.47%** ⬆️ |
| **Validation** | PASS | **PASS** | Functional Correctness ✅ |

**Impact:** Eliminated approximately **7.4 million stall cycles**, improving execution efficiency without altering the processor's clock frequency or stage depth.

---

## 🎯 The Challenge: Load-Use Hazards

In the standard E203 pipeline, a **Load-Use hazard** forces a stall cycle when an instruction requires data immediately after a Load instruction:

```assembly
lw   x1, 0(x2)      # Cycle 1: MEM Stage (Data arrives late)
add  x3, x1, x4     # Cycle 2: EX Stage (Needs data immediately) -> STALL
```

### The Solution: Forwarding (Bypassing)
Instead of stalling to wait for the Register File writeback, we implemented a **forwarding path** that routes data directly from the LSU (Load-Store Unit) output to the ALU input.

```
         ┌──────────────┐
         │  LSU Output  │ (Data just loaded from memory)
         │ (lsu_o_wbck) │
         └──────┬───────┘
                │
         Forwarding Path (NEW!)
                │
                ▼
         ┌──────────────┐
         │  ALU Input   │ (Operand for next instruction)
         │ (disp_o_alu) │
         └──────────────┘
```

*Diagram: Forwarding logic bypasses the Register File for dependent instructions.*

---

## 🏗️ E203 Processor Architecture Overview

The E203 is a **two-stage pipeline** RISC-V processor core designed for embedded applications:

```
┌─────────────────────────────────────────────────────────────┐
│                        Stage 1: IF/ID                       │
│  ┌──────────────┐        ┌──────────────┐                   │
│  │     IFU      │───────▶│   Decoder    │                   │
│  │ (Instruction │        │              │                   │
│  │    Fetch)    │        │              │                   │
│  └──────────────┘        └──────────────┘                   │
└─────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────┐
│                        Stage 2: EX/WB                       │
│  ┌──────────────┐        ┌──────────────┐                   │
│  │  Dispatcher  │───────▶│     ALU      │                   │
│  │ (e203_exu_   │        │              │                   │
│  │   disp)      │        │     LSU      │                   │
│  └──────────────┘        └──────────────┘                   │
│         │                        │                          │
│         │                        ▼                          │
│         │                ┌──────────────┐                   │
│         │                │   Writeback  │                   │
│         │                └──────────────┘                   │
│         │                        │                          │
│         └────────────────────────┘                          │
│              (OITF tracking for long-latency ops)           │
└─────────────────────────────────────────────────────────────┘
```

**Key Modules:**
- **e203_exu_disp**: Instruction dispatcher with RAW (Read-After-Write) hazard detection
- **e203_exu**: Execution unit containing ALU and LSU
- **OITF**: Outstanding Instruction Tracking FIFO for multi-cycle operations

---
## 🔧 Implementation Details

### Modified Files

We modified **two core Verilog files** to implement the forwarding logic:

1. **[core/e203_exu_disp.v](core/e203_exu_disp.v)** - Instruction Dispatcher
   - Added LSU writeback signal inputs
   - Implemented forwarding match detection logic
   - Modified RAW hazard detection to account for forwarding
   - Added data multiplexers for forwarded operands

2. **[core/e203_exu.v](core/e203_exu.v)** - Execution Unit
   - Connected LSU writeback signals to dispatcher
   - Routed writeback data and control signals

### Key Modifications

#### 1. New Input Signals (e203_exu_disp.v)

```verilog
// Added to module port list
input  lsu_o_valid,                      // LSU writeback valid signal
input  [`E203_RFIDX_WIDTH-1:0] lsu_o_wbck_rdidx,  // Destination register index
input  [`E203_XLEN-1:0] lsu_o_wbck_wdat,          // Writeback data (32-bit)
input  lsu_o_wbck_rdwen,                          // Write enable (prevents Store forwarding)
```

#### 2. Forwarding Detection Logic

```verilog
// RS1 forwarding match detection
wire rs1_fwd_match = lsu_o_valid                   // LSU is writing back
                   & lsu_o_wbck_rdwen               // It's a register write (not Store)
                   & (lsu_o_wbck_rdidx == disp_i_rs1idx)  // Register index matches
                   & disp_i_rs1en                   // Current instruction reads RS1
                   & (~disp_i_rs1x0);               // Not reading x0 (hardwired zero)

// RS2 forwarding match detection (similar logic)
wire rs2_fwd_match = lsu_o_valid
                   & lsu_o_wbck_rdwen
                   & (lsu_o_wbck_rdidx == disp_i_rs2idx)
                   & disp_i_rs2en
                   & (~disp_i_rs2x0);
```

**Critical Conditions Explained:**

| Condition | Purpose | Why It Matters |
|-----------|---------|----------------|
| `lsu_o_valid` | LSU writeback is active | Ensures data is available |
| `lsu_o_wbck_rdwen` | Write enable is asserted | Prevents forwarding from Store instructions (they don't write registers) |
| Register index match | Destination == Source | Only forward when registers actually match |
| `disp_i_rs1en` | RS1 read enable | Current instruction actually uses this operand |
| `~disp_i_rs1x0` | Not reading x0 | **CRITICAL:** RISC-V x0 must always read as zero! |

#### 3. Modified RAW Hazard Detection

```verilog
// Original logic (always stalls on OITF conflict):
// wire raw_dep = (oitfrd_match_disprs1) | (oitfrd_match_disprs2) | (oitfrd_match_disprs3);

// Optimized logic (stalls only if forwarding is not possible):
wire raw_dep = ((oitfrd_match_disprs1 & ~rs1_fwd_match)) |  // RS1: Conflict AND can't forward
               ((oitfrd_match_disprs2 & ~rs2_fwd_match)) |  // RS2: Conflict AND can't forward
               (oitfrd_match_disprs3);                      // RS3: Floating-point (no forwarding)
```

**Truth Table:**

| OITF Conflict | Can Forward | Result | Explanation |
|---------------|-------------|--------|-------------|
| ❌ No | - | No Stall | No hazard exists |
| ✅ Yes | ✅ Yes | **No Stall** | **Optimization:** Forward data instead of stalling |
| ✅ Yes | ❌ No | Stall | Must wait for instruction to complete |

#### 4. Data Multiplexer for Forwarding

```verilog
// Original: Always use register file data
// assign disp_o_alu_rs1 = disp_i_rs1_msked;

// Optimized: Select forwarded data when available
assign disp_o_alu_rs1 = rs1_fwd_match ? lsu_o_wbck_wdat : disp_i_rs1_msked;
assign disp_o_alu_rs2 = rs2_fwd_match ? lsu_o_wbck_wdat : disp_i_rs2_msked;
```

**Data Flow:**
```
lsu_o_wbck_wdat ────┐
                    │  rs1_fwd_match
                    ├──────────────► disp_o_alu_rs1 ──► ALU
                    │    (MUX)
disp_i_rs1_msked ───┘
```

---

## ⚠️ Critical Edge Case: The `x0` Register

A significant challenge during implementation was the **RISC-V `x0` invariant**. Register `x0` is hardwired to zero, but the forwarding logic initially treated it like any other register.

**The Bug:**
If a standard Load instruction wrote to `x0` (e.g., `lw x0, 0(sp)`), the forwarding logic would incorrectly forward the *loaded value* to a subsequent instruction reading `x0`, instead of forwarding `0`. This caused CRC checksum failures in CoreMark.

**The Fix:**
We explicitly disabled forwarding when the source register is `x0`:
```verilog
& (~disp_i_rs1x0); // Enforce x0 always reads as zero
```
This ensures strict adherence to the RISC-V ISA specification.

---


## 📁 Repository Structure

```
.
├── README.md                    # Main documentation
├── QUICK_START.md               # 10-minute getting started guide
│
├── core/                        # Modified E203 Verilog files
│   ├── e203_exu_disp.v          # Dispatcher with forwarding logic
│   ├── e203_exu.v               # Execution unit with signal routing
│   └── README_Load_Use_Forwarding.md  # Chinese detailed explanation
│
└── benchmark/                   # CoreMark with educational enhancements
    ├── README.md                # Benchmark documentation
    └── coremark/                # CoreMark source code

```

---

## 🚀 Getting Started

1.  Clone the repository.
2.  Replace the original E203 RTL files with the modified versions in `core/`.
3.  Run the simulation or synthesis using your standard E203 flow (Vivado/Verilator).

## 📄 License & Acknowledgments

* Based on the [Hummingbirdv2 E203](https://github.com/riscv-mcu/e203_hbirdv2) open-source core.
* Course: Computer Architecture & Organization.
* Licensed under Apache 2.0.

---
*I hope this project serves as a helpful reference for your studies.*

*Wish you a productive and enjoyable experiment time!*
