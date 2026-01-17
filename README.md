# 18-Bit Multi-Cycle CPU Architecture

## 📌 Introduction
This project implements a fully functional **18-bit Multi-Cycle Processor** designed from scratch using **Logisim**. The architecture features a custom Instruction Set (ISA), a hardware-level Control Unit based on a **High-Level State Machine (HLSM)**, and a dedicated Assembler written in **C**.

The project demonstrates advanced concepts of Computer Organization, including data path design, micro-operations, and low-level control logic implementation.

## 🚀 Key Features
* **18-bit Word Size:** Custom architecture for both instructions and data.
* **Multi-Cycle Execution:** Instructions are executed in multiple clock cycles (Fetch, Decode, Execute, Memory, WriteBack) managed by an FSM.
* **Custom ISA:** Supports Arithmetic, Logic, Memory Access, Stack, and Flow Control operations.
* **Dual-Port Register File:** 8 General Purpose Registers (R0-R7) allowing simultaneous read operations.
* **Hardware Control Unit:** Implemented as a Mealy Machine using a State Counter (T0-Tn) and combinational logic.
* **Custom Assembler:** A CLI tool written in C to convert Assembly code into machine-readable Hex format for Logisim ROM.
* **Stack Support:** Native hardware support for `PUSH`, `POP`, and `JAL` (Function Calls).

## 🛠️ Instruction Set Architecture (ISA)

The processor supports the following operations:

| Category | Mnemonics | Description |
| :--- | :--- | :--- |
| **Arithmetic** | `ADD`, `SUB` | Register-based addition and subtraction. |
| **Logic** | `NAND`, `NOR`, `AND`, `OR` | Bitwise logical operations. |
| **Immediate** | `ADDI`, `SUBI`, `LUI` | Operations with immediate values (supports Sign Extension). |
| **Shift** | `SRL`, `SRA` | Logical and Arithmetic shifts (preserves sign bit). |
| **Memory** | `LD`, `ST` | Load word from RAM / Store word to RAM. |
| **Stack** | `PUSH`, `POP` | Stack pointer manipulation. |
| **Control Flow** | `JUMP`, `JAL` | Unconditional jumps and Subroutine calls. |
| **Conditional** | `CMOV` | Conditional Move based on Zero Flag. |

## 🏗️ Architecture Overview

### 1. The Data Path
The system is built around an 18-bit Data Path connecting:
* **ALU:** Handles all arithmetic and logic computations.
* **Register File:** 8 registers (R0-R7) with dual read ports and single write port.
* **Sign Extender:** Handles 5-bit to 18-bit conversion for immediate values.
* **Program Counter (PC):** Managed by MUXes for standard increment, branching, or jumps.

### 2. The Control Unit (HLSM)
The Control Unit functions as the "brain" of the CPU. It uses a **State Counter** to track the execution stage of each instruction:
* **T0 (Fetch):** Fetch instruction from ROM, Increment PC.
* **T1 (Decode/Execute):** Decode Opcode, read registers, perform ALU operation.
* **T2+ (Mem/WB):** Access Memory (Load/Store) or Write Back result to Register File.

## 💻 The Assembler
A custom assembler is included to simplify programming the CPU. It parses the assembly syntax and generates a `.hex` file compatible with Logisim.

### How to Build & Run
1.  **Compile the Assembler:**
    ```bash
    gcc assembler.c -o asm
    ```

2.  **Create an Assembly File (e.g., `test.asm`):**
    ```assembly
    ADDI R1, R0, 10   ; R1 = 10
    ADDI R2, R0, 5    ; R2 = 5
    ADD  R3, R1, R2   ; R3 = 15
    ST   R3, 0        ; RAM[0] = 15
    JUMP 3            ; Infinite Loop
    ```

3.  **Run the Assembler:**
    ```bash
    ./asm test.asm output.hex
    ```

4.  **Load into Logisim:**
    * Open Logisim.
    * Right-click on the **ROM** component.
    * Select **"Load Image"** and choose `output.hex`.

## 📂 Project Structure
* `cpu.circ` - Main Logisim project file.
* `assembler.c` - Source code for the custom assembler.
* `tests/` - Example Assembly programs for testing different instructions.
* `docs/` - Circuit diagrams and documentation.

## 🤝 Author
**Ebubekir** Computer Engineering Student @ Marmara University
