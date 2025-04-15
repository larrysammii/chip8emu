#include "chip8.hpp"
#include <fstream>

const unsigned int START_ADDRESS = 0x200;
// FONTSET_SIZE = 80 because there are 16 characters, 5 bytes each.
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontset[FONTSET_SIZE] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0,// 0
		0x20, 0x60, 0x20, 0x20, 0x70,// 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0,// 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0,// 3
		0x90, 0x90, 0xF0, 0x10, 0x10,// 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0,// 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0,// 6
		0xF0, 0x10, 0x20, 0x40, 0x40,// 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0,// 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0,// 9
		0xF0, 0x90, 0xF0, 0x90, 0x90,// A
		0xE0, 0x90, 0xE0, 0x90, 0xE0,// B
		0xF0, 0x80, 0x80, 0x80, 0xF0,// C
		0xE0, 0x90, 0x90, 0x90, 0xE0,// D
		0xF0, 0x80, 0xF0, 0x80, 0xF0,// E
		0xF0, 0x80, 0xF0, 0x80, 0x80 // F
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

  // Open file as a stream of binary and move file pointer to the end(ate = at end).
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