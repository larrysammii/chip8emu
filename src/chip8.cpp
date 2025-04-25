#include "chip8.hpp"

#include <SDL3/SDL_keycode.h>

#include <fstream>

const unsigned int START_ADDRESS = 0x200;
// FONTSET_SIZE = 80 because there are 16 characters, 5 bytes each.
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

Chip8::Chip8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
  // Initialize Program Counter (PC)
  pc = START_ADDRESS;

  // Load fonts into memory
  for (const unsigned char i : fontset) {
    memory[FONTSET_START_ADDRESS + 1] = i;
  }

  // Initialize RNG
  randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

  // Setup function pointer table
  // ========== Main Table ==========
  table[0x0] = &Chip8::Table0;
  table[0x1] = &Chip8::OP_1nnn;
  table[0x2] = &Chip8::OP_2nnn;
  table[0x3] = &Chip8::OP_3xkk;
  table[0x4] = &Chip8::OP_4xkk;
  table[0x5] = &Chip8::OP_5xy0;
  table[0x6] = &Chip8::OP_6xkk;
  table[0x7] = &Chip8::OP_7xkk;
  table[0x8] = &Chip8::Table8;
  table[0x9] = &Chip8::OP_9xy0;
  table[0xA] = &Chip8::OP_Annn;
  table[0xB] = &Chip8::OP_Bnnn;
  table[0xC] = &Chip8::OP_Cxkk;
  table[0xD] = &Chip8::OP_Dxyn;
  table[0xE] = &Chip8::TableE;
  table[0xF] = &Chip8::TableF;

  // ========== Initialize Tables with array size 0xE ==========
  for (size_t i = 0; i <= 0xE; i++) {
    table0[i] = &Chip8::OP_NULL;
    table8[i] = &Chip8::OP_NULL;
    tableE[i] = &Chip8::OP_NULL;
  }

  // ========== Table for $00EN ==========
  table0[0x0] = &Chip8::OP_00E0;
  table0[0xE] = &Chip8::OP_00EE;

  // ========== Table for $8xyN ==========
  table8[0x0] = &Chip8::OP_8xy0;
  table8[0x1] = &Chip8::OP_8xy1;
  table8[0x2] = &Chip8::OP_8xy2;
  table8[0x3] = &Chip8::OP_8xy3;
  table8[0x4] = &Chip8::OP_8xy4;
  table8[0x5] = &Chip8::OP_8xy5;
  table8[0x6] = &Chip8::OP_8xy6;
  table8[0x7] = &Chip8::OP_8xy7;
  table8[0xE] = &Chip8::OP_8xyE;

  // ========== Table for $ExNN ==========
  tableE[0x1] = &Chip8::OP_ExA1;
  tableE[0xE] = &Chip8::OP_Ex9E;

  // ========== Initialize Tables with array size 0x65 + 1 ==========
  for (size_t i = 0; i <= 0x65; i++) {
    tableF[i] = &Chip8::OP_NULL;
  }

  // ========== Table for $FxNN ==========
  tableF[0x07] = &Chip8::OP_Fx07;
  tableF[0x0A] = &Chip8::OP_Fx0A;
  tableF[0x15] = &Chip8::OP_Fx15;
  tableF[0x18] = &Chip8::OP_Fx18;
  tableF[0x1E] = &Chip8::OP_Fx1E;
  tableF[0x29] = &Chip8::OP_Fx29;
  tableF[0x33] = &Chip8::OP_Fx33;
  tableF[0x55] = &Chip8::OP_Fx55;
  tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::LoadROM(const char *filename) {
  // Open file as a stream of binary and move file pointer to the end(ate = at
  // end).
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (file.is_open()) {
    // Get size of file and allocate a buffer to hold content
    std::streampos size = file.tellg();
    char *buffer = new char[size];

    // Go back to the beginning of the file.
    file.seekg(0, std::ios::beg);

    // read() reads the ROM into memory/buffer.
    // Fill "size" bytes into the "buffer"(destination).
    file.read(buffer, size);

    // Already loaded the ROM into buffer, close the file.
    file.close();

    // Load ROM contents into CHIP-8's memory, starting from 0x200.
    for (long i = 0; i < size; i++) {
      memory[START_ADDRESS + i] = buffer[i];
    }

    delete[] buffer;
  }
}

void Chip8::Cycle() {
  // Fetch current opcode
  opcode = (memory[pc] << 8u) | memory[pc + 1];

  // Increment PC before any further instructions.
  pc += 2;

  // Decode and Execute:
  // (opcode & 0xF000u) >> 12u: Gets the index (0 to 15).
  // table[index]: Retrieves the function pointer (e.g., &Chip8::OP_1nnn).
  // this->*: Applies the function pointer to the current Chip8 instance.
  // (): Calls the function.
  (this->*(table[(opcode & 0xF000u) >> 12u]))();

  // Chip-8 has two timers: delayTimer and soundTimer, both 8-bit values that
  // decrement at 60 Hz when non-zero. In an emulator, the Cycle() function
  // might run faster or slower than 60 Hz, but a simple approach is to
  // decrement them once per cycle and assume the emulator runs Cycle() at
  // approximately 60 Hz for timing purposes. (In a real implementation, you’d
  // separate timer updates into a 60 Hz loop, but let’s keep it simple for
  // now.)
  if (delayTimer > 0) {
    // delayTimer: Used for game timing (e.g., Fx15 sets it, Fx07 reads it).
    --delayTimer;
  }

  if (soundTimer > 0) {
    // soundTimer: Triggers a beep when non-zero and decrements;
    // it stops at zero.
    --soundTimer;
  }
}

// "this" keyword is a pointer to the current Chip8 object instance.
// "(*this)" or "(this)->" deference "this"(pointer) to get the object.
// These dispatch functions get the emulator instance,
// and calls the subtable functions based on the opcodes' relevant digits,
// by filtering (bitwise AND) the last or last 2 digits of the opcode.
// ie. Table0 would get the table0[opcode * 0x000Fu] from object,
// then () calls the opcode function eg. OP_0xNNN().

// ".*(...)()" calls a member function pointer on an object.
// (...) groups the expression.
// the final () calls the underlying function (eg. OP_8xy4()).
void Chip8::Table0() { ((this)->*(table0[opcode & 0x000Fu]))(); }
void Chip8::Table8() { ((this)->*(table8[opcode & 0x000Fu]))(); }
void Chip8::TableE() { ((this)->*(tableE[opcode & 0x000Fu]))(); }
void Chip8::TableF() { ((this)->*(tableF[opcode & 0x00FFu]))(); }

// Dummy function, would be called if a non-existent opcode gets called.
void Chip8::OP_NULL() {}

// CLS. Clears the screen.
void Chip8::OP_00E0() { memset(video, 0, sizeof(video)); }

// RET. minus 1 stack level.
void Chip8::OP_00EE() {
  // Requires safety check for stack level below 0,
  // unsigned int wraparound if out of 0 to 255.
  // if (sp == 0) {
  //   pc = stack[sp];  // Needs error handling.
  // } else {
  //   // Move sp down.
  sp--;
  pc = stack[sp];
  // }
}

// Jump to address nnn
void Chip8::OP_1nnn() {
  // extracting last 12-bit (nnn) of the opcode with bitwise AND,
  // example:
  // opcode:  0001 0010 1010 0000  (0x12A0)
  // 0x0FFF:  0000 1111 1111 1111  (0x0FFF)
  //          -------------------
  // Result:  0000 0010 1010 0000  (0x02A0)

  uint16_t addr = opcode & 0x0FFFu;
  pc = addr;
}

// Call subroutine at address nnn
// Analogy: You were told to visit a place (nnn),
// but you plan to return where you are now.
void Chip8::OP_2nnn() {
  // Put current PC onto the top of the stack.
  // Analogy: Before leaving your current place,
  // you write down the next place's address in your notebook(stack)
  stack[sp] = pc;

  // Move SP up one level.
  // Safety check to prevent overflow & wrap around.
  // Analogy: You start a new page on your notebook (sp++),
  if (sp >= STACK_LEVELS) {
    sp = STACK_LEVELS;  // Needs error handling.
  } else {
    sp++;
  }

  // PC jumps to the address nnn
  // Analogy: then you move to the new place (nnn).
  uint16_t addr = opcode & 0x0FFFu;
  pc = addr;

  // Analogy: The notebook has record of your last address,
  // so you can return to it with RET(00EE).
}

// Check if Vx == kk.
// If true, jump forward an extra 2 bytes (total pc += 4).
// If false, move to the next instruction (pc += 2).
//
void Chip8::OP_3xkk() {
  // Example:
  // Instruction: 0x3A45 (3xkk where x = A, kk = 0x45).
  // If register VA holds 0x45, skip the next instruction (pc += 4).
  // If VA holds any other value (e.g., 0x20), proceed normally (pc += 2).
  // Diagram:
  // Binary: 0011 xxxx kkkk kkkk
  // +------+------+------+------+
  // | 0011 | xxxx | kkkk kkkk    |
  // +------+------+--------------+
  //   ^      ^         ^
  //   |      |         |
  //   |      |         8-bit value (kk)
  //   |     4-bit register index (x)
  //   Opcode (3)
  // 3: The first nibble (4 bits) identifies the instruction.
  // x: The second nibble specifies the register (V0 to VF).
  // kk: The last 8 bits are the value to compare against.
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  if (registers[x] == kk) {
    pc += 2;
  }
}

void Chip8::OP_4xkk() {
  // Same as 3xkk but not equal.
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  if (registers[x] != kk) {
    pc += 2;
  }
}

// Compares the values in registers Vx and Vy.
// If Vx equals Vy, the program counter (pc) skips the next instruction by
// incrementing by 4 (since CHIP-8 instructions are 2 bytes, skipping one
// instruction means pc += 4). If Vx does not equal Vy, the program counter
// increments normally by 2 (pc += 2) to execute the next instruction.
void Chip8::OP_5xy0() {
  // 16-bit instruction: 0x5xy0
  // Binary: 0101 xxxx yyyy 0000
  // +------+------+------+------+
  // | 0101 | xxxx | yyyy | 0000 |
  // +------+------+------+------+
  //   ^      ^      ^      ^
  //   |      |      |      |
  //   |      |      |     Must be 0
  //   |      |     4-bit register index (y)
  //   |     4-bit register index (x)
  //   Opcode (5)
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 8u;

  if (registers[x] == registers[y]) {
    pc += 2;
  }
}

// Set Vx = kk.
void Chip8::OP_6xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  registers[x] = kk;
}

// Set Vx = Vx + kk.
void Chip8::OP_7xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  // registers[x] = registers[x] + kk;
  registers[x] += kk;
}

// Set Vx = Vy.
void Chip8::OP_8xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] = registers[y];
}

// Set Vx = Vx bitwise OR (combine) Vy.
void Chip8::OP_8xy1() {
  // V[x] = 0xA5 = 1010 0101
  // V[y] = 0x3C = 0011 1100
  // Check if at least 1 bit is 1.
  // OR            1011 1101 = 0xBD
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  // registers[x] = registers[x] | registers[y];
  registers[x] |= registers[y];
}

// Set Vx = Vx bitwise AND Vy.
void Chip8::OP_8xy2() {
  // V[x] = 0xA5 = 1010 0101
  // V[y] = 0x3C = 0011 1100
  // Check if both bits are 1.
  // AND           0010 0100 = 0x24
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] &= registers[y];
}

// Set Vx = Vx XOR Vy.
// 1 if bits are different.
void Chip8::OP_8xy3() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] ^= registers[y];
}

// Set Vx = Vx + Vy, set VF = carry.
// The values of Vx and Vy are added together. If the result is greater than 8
// bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of
// the result are kept, and stored in Vx. This is an ADD with an overflow flag.
//
// Carry means the sum of two numbers is too large to fit in the given number of
// bits. The extra bit “ carries over” to a higher position(like carrying a 1 in
// decimal addition).
//
// If the sum is greater than what can fit into a byte (255),
// register VF will be set to 1 as a flag.
void Chip8::OP_8xy4() {
  // 16-bit instruction: 0x8xy4
  // Binary: 1000 xxxx yyyy 0100
  // +------+------+------+------+
  // | 1000 | xxxx | yyyy | 0100 |
  // +------+------+------+------+
  //   ^      ^      ^      ^
  //   |      |      |      |
  //   |      |      |     Operation (4 = ADD)
  //   |      |     4-bit register index (y)
  //   |     4-bit register index (x)
  //   Opcode (8)
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  uint16_t sum = registers[x] + registers[y];

  // if (sum > 255U) {
  //   registers[0xF] = 1;
  // } else {
  //   registers[0xF] = 0;
  // }

  registers[0xF] = (sum > 255U) ? 1 : 0;

  // Only the lowest 8 bits of the result are kept, and stored in Vx.
  registers[x] = sum & 0x00FFu;
}

// Set Vx = Vx - Vy, set VF = NOT borrow.
// Borrow means the number being subtracted is larger than the number it’s
// subtracted from, requiring a “borrow” from a higher position to make the
// subtraction possible.
// example: 5-10 = -5 = requires borrow
//
// If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy
// is subtracted from Vx, and the results stored in Vx.
void Chip8::OP_8xy5() {
  // 16-bit instruction: 0x8xy5
  // Binary: 1000 xxxx yyyy 0101
  // +------+------+------+------+
  // | 1000 | xxxx | yyyy | 0101 |
  // +------+------+------+------+
  //   ^      ^      ^      ^
  //   |      |      |      |
  //   |      |      |     Operation (5 = SUB)
  //   |      |     4-bit register index (y)
  //   |     4-bit register index (x)
  //   Opcode (8)
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[0xF] = (registers[x] < registers[y]) ? 0 : 1;

  registers[x] -= registers[y];
  // WRONG:
  // uint16_t diff = registers[x] - registers[y];

  // registers[0xF] = (diff >= registers[y]) ? 0 : 1;

  // Only the lowest 8 bits of the result are kept, and stored in Vx.
  // registers[x] = diff & 0x00FFu;
}

// Set Vx = Vx SHR 1.
// If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0.
// Then Vx is divided by 2. A right shift is performed (division by 2), and the
// least significant bit is saved in Register VF.
void Chip8::OP_8xy6() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  // uint8_t lsb = registers[x] & 0x1u;
  // registers[0xF] = (lsb) ? 1 : 0;
  registers[0xF] = (registers[x] & 0x1u);
  registers[x] >>= 1;
}

// Set Vx = Vy - Vx, set VF = NOT borrow.
// If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy,
// and the results stored in Vx.
void Chip8::OP_8xy7() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;
  registers[0xF] = (registers[y] > registers[x]) ? 1 : 0;
  registers[x] = registers[y] - registers[x];
}

// Set Vx = Vx SHL 1.
// If the most-significant bit(MSB) of Vx is 1, then VF is set to 1, otherwise
// to 0. Then Vx is multiplied by 2. A left shift is performed (multiplication
// by 2), and the most significant bit is saved in Register VF.
void Chip8::OP_8xyE() {
  // 16-bit instruction: 0x8xyE
  // Binary: 1000 xxxx yyyy 1110
  // +------+------+------+------+
  // | 1000 | xxxx | yyyy | 1110 |
  // +------+------+------+------+
  //   ^      ^      ^      ^
  //   |      |      |      |
  //   |      |      |     Operation (E = SHL)(left shift)
  //   |      |     4-bit register index (y)
  //   |     4-bit register index (x)
  //   Opcode (8)
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  // Note on y:
  // In most modern CHIP-8 implementations,
  // 8xyE ignores Vy and uses Vx = Vx << 1.
  // If your emulator follows the original COSMAC VIP spec, it might use
  // Vx = Vy << 1. Let’s assume Vx = Vx << 1 (standard).

  // 0x80 = 128
  // V[x]: 1000 0000  (0x80)
  // MSB:  ^ (1, so VF = 1)
  // Shift: 0000 0000  (0x00)
  // V[x] after: 0000 0000
  registers[0xF] = (registers[x] & 0x0080u) ? 1 : 0;
  registers[x] <<= 1;
}

// Skip next instruction if Vx != Vy.
// Since our PC has already been incremented by 2 in Cycle(), we can just
// increment by 2 again to skip the next instruction.
void Chip8::OP_9xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  if (registers[x] != registers[y]) {
    pc += 2;
  }
}

// Set Index Register as opcode's address nnn.
void Chip8::OP_Annn() { index = opcode & 0x0FFFu; }

// Jump to location nnn + V0.
void Chip8::OP_Bnnn() {
  // Before:
  // V0 = 0x10
  // pc = 0x300
  // I = 0x200 (unchanged)
  // V1-VF = (any values)
  //
  // After:
  // V0 = 0x10 (unchanged)
  // pc = 0x210 (0x200 + 0x10)
  // I = 0x200 (unchanged)
  // V1-VF = (unchanged)
  pc = opcode & 0x0FFFu + registers[0];
}

// Set Vx = random byte bitwise AND kk.
// Generates a random number between 0 and 255 (8-bit, 0x00 to 0xFF).
// Performs a bitwise AND between the random number and kk (the mask).
// Stores the result in register Vx.
void Chip8::OP_Cxkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;
  uint8_t random = randByte(randGen);

  registers[x] = random & kk;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF =
// collision.
//
// We iterate over the sprite, row by row and column by column. We know there
// are eight columns because a sprite is guaranteed to be eight pixels wide.
//
// If a sprite pixel is on then there may be a collision with what’s already
// being displayed, so we check if our screen pixel in the same location is set.
// If so we must set the VF register to express collision.
//
// Then we can just XOR the screen pixel with 0xFFFFFFFF to essentially XOR it
// with the sprite pixel (which we now know is on). We can’t XOR directly
// because the sprite pixel is either 1 or 0 while our video pixel is either
// 0x00000000 or 0xFFFFFFFF.
void Chip8::OP_Dxyn() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;  // bits 8-11
  uint8_t y = (opcode & 0x00F0u) >> 4u;  // bits 4-7
  uint8_t height = opcode & 0x000Fu;     // bits 0-3

  // (0 to 255, but only 0-63 matters for 64-wide screen).
  uint8_t x_cord = registers[x] % PX_WIDTH;
  // (or x_coord & 0x3F) to stay in 0-63.

  // (0 to 255, 0-31 for 32-high screen).
  uint8_t y_cord = registers[y] % PX_HEIGHT;
  // (or y_coord & 0x1F) to stay in 0-31.

  // What it does: Clears the collision flag (VF, register 15) before drawing.
  // Purpose: VF will be set to 1 if any pixel flips from on (0xFFFFFFFF) to off
  // (0x00000000).
  registers[0xFu] = 0;

  // Analogy: Think of drawing a sprite as painting a grid on a canvas:
  // The sprite is a stencil with n rows, each 8 holes wide (bits).
  // You paint row-by-row, checking each hole (bit) to decide if you apply XOR
  // paint at the canvas position (Vx + bit, Vy + row).
  for (unsigned int row = 0; row < height; row++) {
    // Processes each row of the sprite,
    // stored in memory[I] to memory[I + n - 1].
    uint8_t spriteByte = memory[index + row];
    for (unsigned int col = 0; col < 8; col++) {
      // Bit Check: spritePx = spriteByte & (0x80u >> col);
      // 0x80u = 10000000 (MSB set).
      // 0x80u >> col: Shifts the 1 right (e.g., col = 0: 10000000, col = 1:
      // 01000000, ..., col = 7: 00000001). spriteByte & (0x80u >> col): Tests
      // if the col-th bit is 1 (non-zero if bit is 1, zero if bit is 0).

      // Example: For spriteByte = 0xF0 (11110000):
      // col = 0: 0xF0 & 0x80 = 11110000 & 10000000 = 10000000 (non-zero, bit 0
      // = 1). col = 1: 0xF0 & 0x40 = 11110000 & 01000000 = 01000000 (non-zero,
      // bit 1 = 1). col = 4: 0xF0 & 0x08 = 11110000 & 00001000 = 00000000
      // (zero, bit 4 = 0).
      uint8_t spritePx = spriteByte & (0x0080u >> col);

      // ================== Setting individual pixel =========================
      // Screen Position: uint32_t* screenPixel = &video[(yPos + row) *
      // VIDEO_WIDTH + (xPos + col)];

      // Calculates the video array index for pixel (xPos + col, yPos + row).
      // VIDEO_WIDTH = 64.
      // Formula: y * 64 + x maps 2D to 1D.
      // Example: For xPos = 10, yPos = 5, row = 0, col = 0:
      // x = 10 + 0 = 10, y = 5 + 0 = 5.
      // video[5 * 64 + 10] = video[330].
      // Uses a pointer (uint32_t*) to directly access and modify video[idx].

      // Purpose: Checks each bit of the byte to decide if a pixel needs
      // drawing, and computes the screen position.

      // Analogy: For each row’s stencil, check 8 holes (bits). If a hole is
      // open (bit = 1), paint at the corresponding canvas spot.

      // (yPos + row): The y-coordinate on the screen (row of the sprite).

      // (yPos + row) * VIDEO_WIDTH: Converts the y-coordinate to the start of
      // the screen row in the 1D array (e.g., y=5 → 5 * 64 = 320).

      // (xPos + col): The x-coordinate on the screen (column of the sprite).

      // (yPos + row) * VIDEO_WIDTH + (xPos + col): The index in video for pixel
      // (xPos + col, yPos + row).

      // The &: &video[idx] returns a pointer (uint32_t*) to the pixel’s memory
      // location, allowing direct modification (e.g., *screenPixel ^=
      // 0xFFFFFFFF).
      uint32_t *screenPx = &video[(y_cord + row) * PX_WIDTH + (x_cord + col)];

      // check sprite pixel is on/off
      //
      // What it does:
      // If the sprite bit is 1 (spritePixel non-zero),
      // XOR the screen pixel and check for collision.
      // Collision:
      // If video[idx] = 0xFFFFFFFF (on) and
      // XOR will turn it off (0xFFFFFFFF ^ 0xFFFFFFFF = 0),
      // set VF = 1.
      // XOR: *screenPixel ^= 0xFFFFFFFF flips the pixel (on to off, off to on).
      if (spritePx) {
        // check collision and if screen pixel is also on
        if (*screenPx == 0xFFFFFFFFu) {
          registers[0xFu] = 1;
        }

        *screenPx ^= 0xFFFFFFFFu;
      }
    }
  }
}

// Skip next instruction if key with the value of Vx is pressed.
void Chip8::OP_Ex9E() {
  uint8_t x = (opcode * 0x0F00u) >> 8u;
  uint8_t key = registers[x];
  // if pressed
  if (keypad[key]) {
    pc += 2;
  }
}

// Skip next instruction if key with the value of Vx is not pressed.
void Chip8::OP_ExA1() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t key = registers[x];
  if (!keypad[key]) {
    pc += 2;
  }
}

// Set Vx = delay timer value.
void Chip8::OP_Fx07() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  registers[x] = delayTimer;
}

// Wait for a key press, store the value of the key in Vx.
//
// The easiest way to “wait” is to decrement the PC by 2 whenever a keypad value
// is not detected. This has the effect of running the same instruction
// repeatedly.
void Chip8::OP_Fx0A() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  // uint8_t key = registers[x];
  for (uint8_t i = 0; i < 16; i++) {
    if (keypad[i]) {
      registers[x] = i;
      pc += 2;
      break;
    }
    pc -= 2;
  }
}

// Set delay timer = Vx.
void Chip8::OP_Fx15() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  delayTimer = registers[x];
}

// Set sound timer = Vx.
void Chip8::OP_Fx18() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  soundTimer = registers[x];
}

// Set I = I + Vx.
void Chip8::OP_Fx1E() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  index += registers[x];
}

// Set I = location of sprite for digit Vx.
// We know the font characters are located at 0x50,
// and we know they’re five bytes each,
// so we can get the address of the first byte of any character
// by taking an offset from the start address.
void Chip8::OP_Fx29() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t digit = registers[x];

  index = FONTSET_START_ADDRESS + (5 * digit);
}

// Store BCD(binary-coded decimal) representation of Vx
// in memory locations I, I+1, and I+2.
//
// The interpreter takes the decimal value of Vx, and places the hundreds digit
// in memory at location in I, the tens digit at location I+1, and the ones
// digit at location I+2.
//
// We can use the modulus operator to get the right-most digit of a number, and
// then do a division to remove that digit. A division by ten will either
// completely remove the digit (340 / 10 = 34), or result in a float which will
// be truncated (345 / 10 = 34.5 = 34).
void Chip8::OP_Fx33() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;

  // read Vx
  uint8_t value = registers[x];

  // Ones
  memory[index + 2] = value % 10;
  value /= 10;

  // Tens
  memory[index + 1] = value % 10;
  value /= 10;

  // Hundreds
  memory[index] = value % 10;
}

// Store registers V0 through Vx in memory starting at location I.
void Chip8::OP_Fx55() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;

  for (uint8_t i = 0; i <= x; i++) {
    memory[index + i] = registers[i];
  }
}

// Read registers V0 through Vx from memory starting at location I.
void Chip8::OP_Fx65() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;

  for (uint8_t i = 0; i <= x; i++) {
    registers[i] = memory[index + i];
  }
}
