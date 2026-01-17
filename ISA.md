## 🛠️ Instruction Set Architecture (ISA) Specifications

This processor utilizes a custom **18-bit RISC-based ISA**. All instructions are fixed-length (18 bits).

### 1. Architectural Overview
| Feature | Size / Description |
| :--- | :--- |
| **Instruction Length** | 18 bits |
| **Data Width** | 18 bits |
| **Address Width** | 12 bits (PC generates 12-bit addresses) |
| **Registers** | 16 General Purpose Registers (R0 - R15) |
| **Register Width** | 18 bits |
| **Program Counter (PC)** | 12 bits |

---

### 2. Opcode Table & Instruction Types
The ISA supports **18 unique instructions** categorized by operation type.

| Mnemonic | Opcode (Bin) | Opcode (Hex) | Type | Description |
| :--- | :--- | :--- | :--- | :--- |
| **ADD** | `00000` | `0x00` | R-Type | Addition |
| **SUB** | `00001` | `0x01` | R-Type | Subtraction |
| **NAND** | `00002` | `0x02` | R-Type | Bitwise NAND |
| **NOR** | `00003` | `0x03` | R-Type | Bitwise NOR |
| **SRL** | `00100` | `0x04` | R-Type | Shift Right Logical |
| **SRA** | `00101` | `0x05` | R-Type | Shift Right Arithmetic |
| **ADDI** | `00110` | `0x06` | I-Type | Add Immediate |
| **SUBI** | `00111` | `0x07` | I-Type | Subtract Immediate |
| **NANDI** | `01000` | `0x08` | I-Type | NAND Immediate |
| **NORI** | `01001` | `0x09` | I-Type | NOR Immediate |
| **JUMP** | `01010` | `0x0A` | Jump | Unconditional Jump |
| **JAL** | `01011` | `0x0B` | Jump | Jump and Link |
| **LD** | `01100` | `0x0C` | Mem | Load from Memory |
| **ST** | `01101` | `0x0D` | Mem | Store to Memory |
| **LUI** | `01110` | `0x0E` | LUI | Load Upper Immediate |
| **CMOV** | `01111` | `0x0F` | R-Type | Conditional Move (if R1==0) |
| **PUSH** | `10000` | `0x10` | Stack | Push to Stack |
| **POP** | `10010` | `0x11` | Stack | Pop from Stack |

---

### 3. Instruction Formats
The 18-bit instruction word is divided as follows based on the instruction type:

#### **R-Type** (Arithmetic, Logic, Shift, CMOV)
Used for operations involving registers.
`[Opcode: 5] [Dest: 4] [Src1: 4] [Src2: 4] [Unused: 1]`

#### **I-Type** (Immediate Operations)
Used for arithmetic/logic with immediate values (5-bit immediate).
`[Opcode: 5] [Dest: 4] [Src1: 4] [Imm: 5]`

#### **Memory Type** (LD, ST) & Stack (PUSH, POP)
Used for memory access. Uses a 9-bit address.
`[Opcode: 5] [Reg: 4] [Address: 9]`

#### **Jump Type** (JUMP, JAL)
Used for control flow with PC-relative offset.
`[Opcode: 5] [Offset: 13]`

#### **LUI Type**
Loads a 9-bit immediate into the upper half of the register.
`[Opcode: 5] [Dest: 4] [Imm: 9]`

---

### 4. Instruction Semantics
Detailed logic for key operations:

* **ADD / SUB:** `DST = R1 ± R2`
* **NAND / NOR:** `DST = ~(R1 & R2)` / `DST = ~(R1 | R2)`
* **SRL / SRA:** Logical vs. Arithmetic Shift of R1 by R2 amount.
* **ADDI / SUBI:** `DST = R1 ± Imm5` (Sign Extended)
* **LD:** `DST = MEM[Addr]`
* **ST:** `MEM[Addr] = SRC`
* **JUMP:** `PC = (PC + 1) + Offset`
* **JAL:** `R15 = PC + 1; PC = (PC + 1) + Offset` (Stores return address in R15)
* **CMOV:** `if (R1 == 0) R2 = R3`
* **PUSH:** `MEM[SP] = Reg; SP--`
* **POP:** `Reg = MEM[SP + 1]; SP++`

---

### 5. Register Map
The CPU features 16 general-purpose registers (4-bit addressable):

* **R0 - R14:** General Purpose
* **R15:** Link Register (used by JAL for return address)
