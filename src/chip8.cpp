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
  for (unsigned int i = 0; i < FONTSET_SIZE; i++) {
    memory[FONTSET_START_ADDRESS + 1] = fontset[i];
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

void Chip8::OP_3xkk() {}

void Chip8::OP_4xkk() {}

void Chip8::OP_5xy0() {}

void Chip8::OP_6xkk() {}

void Chip8::OP_7xkk() {}

void Chip8::OP_8xy0() {}

void Chip8::OP_8xy1() {}

void Chip8::OP_8xy2() {}

void Chip8::OP_8xy3() {}

void Chip8::OP_8xy4() {}

void Chip8::OP_8xy5() {}

void Chip8::OP_8xy6() {}

void Chip8::OP_8xy7() {}

void Chip8::OP_8xyE() {}

void Chip8::OP_9xy0() {}

void Chip8::OP_Annn() {}

void Chip8::OP_Bnnn() {}

void Chip8::OP_Cxkk() {}

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
