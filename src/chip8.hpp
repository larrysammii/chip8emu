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
  void LoadROM(char const *filename);

  uint8_t keypad[KEY_COUNT]{};
  uint32_t video[PX_WIDTH * PX_HEIGHT]{};

 private:
  // =====================================
  // ========== Instruction Set ==========
  // =====================================
  // There are total of 34 instructions for CHIP-8

  // 00E0: CLS (Clear the display).
  // Set the entire video buffer to zeroes.
  void OP_00E0();

  // 00EE: RET (Return from a subroutine).
  // The top of the stack has the address of one instruction past the one that called the subroutine,
  // so we can put that back into the PC.
  // Note that this overwrites our preemptive pc += 2 earlier.
  void OP_00EE();

  // 1nnn: JP addr (Jump to location nnn).
  // Sets the PC to nnn.
  void OP_1nnn();

  // 16x 8-bit registers, from V0 to VF, holds 0x00 to 0xFF.
  uint8_t registers[REGISTER_COUNT]{};

  // Address space from 0x000 to 0xFFF.
  // But instructions of ROM will be started at 0x200
  // since 0x000-0x1FF reserved for CHIP-8's interpreter.
  uint8_t memory[MEM_SIZE]{};

  // Index register. Store memory address for use in operations
  // 16-bit due to the max memory address (0xFFF) is too big for an 8-bit register.
  uint16_t index{};

  // 16-bit Program Counter (PC)
  // Starts at 0x200, for CPU to keep track of which instruction to execute next.
  uint16_t pc{};

  // Stack order of execution.
  // CHIP-8's 16-levels of stack, meaning it can hold 16 different PCs.
  uint16_t stack[STACK_LEVELS]{};

  // 8-bit stack pointer
  // Keep track of where in memory the CPU is executing in the 16-levels.
  uint8_t sp{};

  // The CHIP-8 has a simple timer used for timing.
  // If the timer value is zero, it stays zero.
  // If it is loaded with a value, it will decrement at a rate of 60Hz (or whatever rate the cycle clock set to).
  uint8_t delayTimer{};

  uint8_t soundTimer{};

  uint16_t opcode;

  // Random Number Generation
  std::default_random_engine randGen;
  std::uniform_int_distribution<uint8_t> randByte;
};