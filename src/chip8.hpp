#pragma once

#include <cstdint>
#include <random>

const unsigned int KEY_COUNT = 16;
const unsigned int MEM_SIZE = 4096;
const unsigned int REGISTER_COUNT = 16;
const unsigned int STACK_LEVELS = 16;
const unsigned int PX_HEIGHT = 32;
const unsigned int PX_WIDTH = 64;

class Chip8 {
 public:
  Chip8();
  void Cycle();
  void LoadROM(char const *filename);

  uint8_t keypad[KEY_COUNT]{};
  uint32_t video[PX_WIDTH * PX_HEIGHT]{};

 private:
  // Function Pointer Table instead of switch statements.

  // These 4 are dispatch functions,
  // for selecting appropriate opcode handler function from their subtables,
  // namely table0, table8, tableE, tableF based on part of the opcode.
  void Table0();
  void Table8();
  void TableE();
  void TableF();

  typedef void (Chip8::*Chip8Func)();
  // 16 units array
  // from instructions that starts with $0 to $F(inclusive so +1).
  Chip8Func table[0xF + 1];

  // 15 units array.
  // for $00E0 & $00EE, only the fourth digit is unique,
  // therefore, 0x000E + 1 (inclusive of $00EE).
  Chip8Func table0[0xE + 1];

  // 15 units array.
  // for those starts with $8xy, only the fourth digit is unique,
  // same principle as above, instructions goes to $8xyE.
  // therefore, 0x000E + 1 (inclusive of $8xyE).
  Chip8Func table8[0xE + 1];

  // 15 units array.
  // for $Exa1 & $Ex9E, although last 2 digits are different,
  // these are the only 2 instructions that need to be dealt with.
  // So a table from 0x0 to 0xE + 1 is enough.
  Chip8Func tableE[0xE + 1];

  // 106 units array.
  // for those that start with $F and last 2 digits are unique.
  // $Fx goes from $Fx07 to $Fx65.
  // For the ease of indexing, the array size should be 0x00 to 0x65(inclusive).
  Chip8Func tableF[0x65 + 1];

  // In the case of invalid opcodes are called (opcodes that don't exist),
  // it calls OP_NULL.

  // =====================================
  // ========== Instruction Set ==========
  // =====================================
  // There are total of 34 instructions for CHIP-8
  // (35 if you include the first one that is useless.)
  void OP_NULL();

  // 00E0: CLS (Clear the display).
  // Set the entire video buffer to zeroes.
  void OP_00E0();

  // 00EE: RET (Return from a subroutine).
  // The top of the stack has the address of one instruction past the one that
  // called the subroutine, so we can put that back into the PC. Note that this
  // overwrites our preemptive pc += 2 earlier.
  void OP_00EE();

  // 1nnn: JP addr (Jump to location nnn).
  // Sets the PC to nnn.
  void OP_1nnn();

  // 2nnn: CALL addr (Starts a subroutine at address nnn).
  // Save the current PC in your stack, then set PC to nnn.
  void OP_2nnn();

  // 3xkk: SE Vx, byte.
  // Skips the next instruction if register Vx equals kk(a number).
  void OP_3xkk();

  // 4xkk: SNE Vx, byte.
  // Skips the next instruction if Vx does not equal to kk.
  void OP_4xkk();

  // 5xy0: SE Vx, Vy.
  // Skips the next instruction if Vx equals Vy.
  void OP_5xy0();

  // 6xkk: LD Vx, byte.
  // Puts the number kk into register Vx.
  void OP_6xkk();

  // 7xkk: ADD Vx, byte.
  // Adds kk to Vx (no extra flag stuff)
  void OP_7xkk();

  // 8xy0: LD Vx, Vy.
  // Copies Vy into Vx.
  void OP_8xy0();

  // 8xy1: OR Vx, Vy.
  // Bitwise OR between Vx and Vy, puts the result in Vx.
  void OP_8xy1();

  // 8xy2: AND Vx, Vy.
  // Bitwise AND, stores it in Vx.
  void OP_8xy2();

  // 8xy3: XOR Vx, Vy.
  // Bitwise XOR, stores it in Vx.
  void OP_8xy3();

  // 8xy4: AND Vx, Vy.
  // Adds Vy to Vx, sets VF to 1 if it overflows(goes over 255)
  void OP_8xy4();

  // 8xy5: SUB Vx, Vy.
  // Subtracts Vy from Vx, sets VF to 1 if no borrow (Vx >= Vy).
  void OP_8xy5();

  // 8xy6: SHR Vx.
  // If the least-significant bit of Vx is 1,
  // then VF is set to 1, otherwise 0.
  // Then Vx is divided by 2.
  void OP_8xy6();

  // 8xy7: SUBN Vx, Vy.
  // Set Vx = Vy - Vx, set VF = NOT borrow.
  // If Vy > Vx, then VF is set to 1, otherwise 0.
  // Then Vx is subtracted from Vy, and the results stored in Vx.
  void OP_8xy7();

  // 8xyE: SHL Vx {, Vy}.
  // Vx left shift 1, most significant bit is saved in register VF.
  void OP_8xyE();

  // 9xy0: SNE Vx, Vy.
  // Skips to the next instruction if Vx != Vy.
  void OP_9xy0();

  // Annn: LD I, addr.
  // Sets I(Index) to address nnn.
  void OP_Annn();

  // Bnnn: JP V0, addr.
  // Jumps to nnn + V0.
  void OP_Bnnn();

  // Cxkk: RND Vx, byte.
  // Puts a random number (0-255) bitwise ANDed with kk, stores into Vx.
  void OP_Cxkk();

  // Dxyn: DRW Vx, Vy, nibble.
  // Draws an N-byte sprite from memory (at I(Index)) onto the screen at (Vx,
  // Vy). VF = 1 if pixels collide.
  void OP_Dxyn();

  // Ex9E: SKP Vx.
  // Skips next instruction if the key with the value Vx is pressed.
  void OP_Ex9E();

  // ExA1: SKNP Vx.
  // Skips next instruction if the key with the value Vx is not pressed.
  void OP_ExA1();

  // Fx07: LD Vx, DT.
  // Puts the delay timer value into Vx.
  void OP_Fx07();

  // Fx0A: LD Vx, K.
  // Waits for a key press, then puts the key's value into Vx.
  void OP_Fx0A();

  // Fx15: LD DT, Vx.
  // Sets the delay timer to Vx.
  void OP_Fx15();

  // Fx18: LD ST, Vx.
  // Sets the sound timer to Vx.
  void OP_Fx18();

  // Fx1E: ADD I, Vx.
  // Adds Vx to I(Index).
  void OP_Fx1E();

  // Fx29: LD F, Vx.
  // Sets I(Index) to the address of a sprite (0-9) for Vx.
  void OP_Fx29();

  // Fx33: LD B, Vx.
  // Stores Vx as 3 digits (hundreds, tens, units) at I, I+1, I+2.
  void OP_Fx33();

  // Fx55: LD[I], Vx.
  // Copies V0 to Vx into memory starting at I.
  void OP_Fx55();

  // Fx65: LD Vx, [I].
  // Loads V0 to Vx from memory starting at I.
  void OP_Fx65();

  // 16x 8-bit registers, from V0 to VF, holds 0x00 to 0xFF.
  // Denoted as Vx in comments.
  uint8_t registers[REGISTER_COUNT]{};

  // Address space from 0x000 to 0xFFF.
  // But instructions of ROM will be started at 0x200
  // since 0x000-0x1FF reserved for CHIP-8's interpreter.
  uint8_t memory[MEM_SIZE]{};

  // Index register. Store memory address for use in operations
  // 16-bit due to the max memory address (0xFFF) is too big for an 8-bit
  // register.
  uint16_t index{};

  // 16-bit Program Counter (PC)
  // Starts at 0x200, for CPU to keep track of which instruction to execute
  // next.
  uint16_t pc{};

  // Stack order of execution.
  // CHIP-8's 16-levels of stack, meaning it can hold 16 different PCs.
  uint16_t stack[STACK_LEVELS]{};

  // 8-bit stack pointer
  // Keep track of where in memory the CPU is executing in the 16-levels.
  uint8_t sp{};

  // The CHIP-8 has a simple timer used for timing.
  // If the timer value is zero, it stays zero.
  // If it is loaded with a value, it will decrement at a rate of 60Hz (or
  // whatever rate the cycle clock set to).
  uint8_t delayTimer{};

  uint8_t soundTimer{};

  uint16_t opcode{};

  // Random Number Generation
  std::default_random_engine randGen;
  std::uniform_int_distribution<uint8_t> randByte;
};