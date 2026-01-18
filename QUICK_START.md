# Quick Start Guide



## The Core Idea

### Problem
```assembly
lw   x1, 0(x2)    # Load data into x1
add  x3, x1, x4   # Immediately use x1 → Pipeline STALLS!
```

**Why?** The `add` needs x1's value before `lw` writes it to the register file.

### Solution
**Forward the data directly from LSU output to ALU input** → No stall needed!

```
LSU Output ──(forwarding path)──► ALU Input
    └─(also)──► Register File
```

---

## What We Changed

### Two Files Modified:

1. **[core/e203_exu_disp.v](core/e203_exu_disp.v)** - Added forwarding logic
2. **[core/e203_exu.v](core/e203_exu.v)** - Connected LSU signals

### Three Key Changes:

#### Change 1: Detect When Forwarding is Possible
```verilog
wire rs1_fwd_match = lsu_o_valid              // LSU writing back
                   & lsu_o_wbck_rdwen          // It's a register write
                   & (rdidx == rs1idx)         // Registers match
                   & disp_i_rs1en              // Instruction uses RS1
                   & (~disp_i_rs1x0);          // Not x0!
```

#### Change 2: Don't Stall If We Can Forward
```verilog
// Old: Always stall on conflict
// wire raw_dep = oitfrd_match_disprs1 | oitfrd_match_disprs2;

// New: Stall only if we can't forward
wire raw_dep = (oitfrd_match_disprs1 & ~rs1_fwd_match) |
               (oitfrd_match_disprs2 & ~rs2_fwd_match);
```

#### Change 3: Select Forwarded Data
```verilog
// Old: Always use register file
// assign disp_o_alu_rs1 = disp_i_rs1_msked;

// New: Use LSU data if forwarding
assign disp_o_alu_rs1 = rs1_fwd_match ? lsu_o_wbck_wdat
                                       : disp_i_rs1_msked;
```

---

## Critical Detail: The x0 Trap!

**RISC-V Rule:** Register `x0` **always reads as zero**.

**Why the `~disp_i_rs1x0` check matters:**

```assembly
lw   x0, 0(sp)    # Load into x0 (value is 0x1234)
add  a1, x0, a2   # Read x0 → MUST get 0, NOT 0x1234!
```

**Without the check:**
- Forwarding logic sees: "LSU writing reg 0, instruction reading reg 0 → FORWARD!"
- Result: `a1 = 0x1234 + a2` ❌ WRONG!

**With the check:**
- Forwarding logic sees: "Reading x0 → NEVER FORWARD!"
- Result: `a1 = 0 + a2` ✅ CORRECT!

---

## Expected Results

### Before Optimization
```
Total Cycles: 220,093,603
CPI: 1.919271
CoreMark: 36.35
```

### After Optimization
```
Total Cycles: 212,707,102  (-3.35%)
CPI: 1.854859              (-3.35%)
CoreMark: 37.61            (+3.47%)
```

### Validation
```
List CRC:   0xe714 ✓
Matrix CRC: 0x1fd7 ✓
State CRC:  0x8e3a ✓
```

---

## Integration Steps

### 1. Replace Core Files
Copy the modified Verilog files to your E203 project:
```bash
cp core/e203_exu_disp.v /path/to/your/e203/rtl/e203_exu_disp.v
cp core/e203_exu.v      /path/to/your/e203/rtl/e203_exu.v
```

### 2. Rebuild the Hardware
```bash
# Using Vivado (example)
cd /path/to/your/e203/fpga
make clean
make bitstream
```

### 3. Flash to FPGA
```bash
make upload
# or use your board's programming tool
```

### 4. Run CoreMark
```bash
cd benchmark
make clean
make
make upload

# Monitor via serial
screen /dev/ttyUSB0 115200
```

### 5. Verify Results
- Check that CRC values match expected values
- Confirm CPI decreased
- Verify CoreMark score increased

---

## Troubleshooting

### Problem: CRC Validation Fails

**Likely cause:** x0 forwarding not properly suppressed

**Check:**
```verilog
// Look for this in e203_exu_disp.v around line 190
wire rs1_fwd_match = lsu_o_valid
                   & lsu_o_wbck_rdwen
                   & (lsu_o_wbck_rdidx == disp_i_rs1idx)
                   & disp_i_rs1en
                   & (~disp_i_rs1x0);  // ← THIS LINE IS CRITICAL!
```

**Fix:** Ensure `& (~disp_i_rs1x0)` is present for BOTH rs1_fwd_match AND rs2_fwd_match.

---

### Problem: No Performance Improvement

**Possible causes:**
1. Forwarding signals not connected in `e203_exu.v`
2. RAW dependency logic not updated
3. Optimization disabled during synthesis

**Check:**
```verilog
// In e203_exu.v, verify these connections exist:
.lsu_o_valid         (lsu_o_valid),
.lsu_o_wbck_rdidx    (lsu_o_wbck_rdidx),
.lsu_o_wbck_wdat     (lsu_o_wbck_wdat),
.lsu_o_wbck_rdwen    (lsu_o_wbck_rdwen),
```

---

### Problem: Synthesis Errors

**Common issue:** Signal width mismatches

**Check:**
- `lsu_o_wbck_rdidx` should be `[E203_RFIDX_WIDTH-1:0]` (usually 5 bits)
- `lsu_o_wbck_wdat` should be `[E203_XLEN-1:0]` (32 bits for RV32)

---

## Understanding the Code

### Signal Flow Diagram

```
┌─────────────┐
│   LSU       │
│  (Load)     │
└──────┬──────┘
       │
       │ lsu_o_wbck_wdat (loaded data)
       │ lsu_o_wbck_rdidx (destination reg)
       │ lsu_o_valid (write enable)
       │
       ▼
┌─────────────────────┐
│  Forwarding Logic   │
│  (rs1_fwd_match)    │
│                     │
│  Checks:            │
│  1. LSU writing?    │
│  2. Reg match?      │
│  3. Not x0?         │
└──────┬──────────────┘
       │
       ▼
┌─────────────────────┐
│   Data MUX          │
│                     │
│  rs1 = match ?      │
│        LSU_data :   │
│        RegFile_data │
└──────┬──────────────┘
       │
       ▼
┌─────────────────────┐
│      ALU            │
│  (uses rs1, rs2)    │
└─────────────────────┘
```

### When Does Forwarding Trigger?

**Example 1: Forwarding Occurs** ✅
```assembly
lw   x5, 0(x10)   # Cycle N:   LSU loads into x5
add  x6, x5, x7   # Cycle N+1: ALU uses x5 → FORWARD!
```

**Example 2: No Forwarding (Different Register)** ❌
```assembly
lw   x5, 0(x10)   # Cycle N:   LSU loads into x5
add  x6, x8, x7   # Cycle N+1: Uses x8, not x5 → No forwarding
```

**Example 3: No Forwarding (x0 Protection)** ❌
```assembly
lw   x0, 0(x10)   # Cycle N:   LSU loads into x0 (discarded)
add  x6, x0, x7   # Cycle N+1: Reads x0 → Must get 0, not LSU data
```

**Example 4: No Forwarding (Store Instruction)** ❌
```assembly
sw   x5, 0(x10)   # Cycle N:   Store doesn't write register
add  x6, x5, x7   # Cycle N+1: x5 comes from register file
```

