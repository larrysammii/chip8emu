#include "chip8.hpp"

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
}

void Chip8::OP_NULL() {}

// CLS
void Chip8::OP_00E0() { memset(video, 0, sizeof(video)); }

// RET
void Chip8::OP_00EE() {
  // Requires safety check for stack level below 0,
  // unsigned int wraparound if out of 0 to 255.
  if (sp == 0) {
    pc = stack[sp];  // Needs error handling.
  } else {
    // Move sp down.
    sp--;
    pc = stack[sp];
  }
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

void Chip8::OP_Dxyn() {}

void Chip8::OP_Ex9E() {}

void Chip8::OP_ExA1() {}

void Chip8::OP_Fx07() {}

void Chip8::OP_Fx0A() {}

void Chip8::OP_Fx15() {}

void Chip8::OP_Fx18() {}

void Chip8::OP_Fx1E() {}

void Chip8::OP_Fx29() {}

void Chip8::OP_Fx33() {}

void Chip8::OP_Fx55() {}

void Chip8::OP_Fx65() {}
