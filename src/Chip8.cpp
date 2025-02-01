#include "Chip8.h"
#include <iostream>

#include <vector>
#include <bitset>

#include <fstream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifdef DEBUG
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#endif


// TODO: Use DREAM-6800 font, as it was an Australian CHIP-8 console
const std::array<uint8_t, 80> gDefaultFont =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

CHIP::CHIP()
{
	memcpy(&mMemory[0x050], &gDefaultFont, sizeof(gDefaultFont));

	// Intiailize the instruction table
	// Minimum for IBM logo is:
	// - 00E0 (clear screen)
	// - 1NNN(jump)
	// - 6XNN(set register VX)
	// - 7XNN(add value to register VX)
	// - ANNN(set index register I)
	// - DXYN(display / draw)
	mInstructions.emplace(0x00E0, [this](const uint16_t opcode) { OpCode_ClearScreen(opcode); });			// 00E0
	mInstructions.emplace(0x1000, [this](const uint16_t opcode) { OpCode_Jump(opcode); });				// 1NNN
	mInstructions.emplace(0x6000, [this](const uint16_t opcode) { OpCode_SetVxToNn(opcode); });			// 6XNN
	mInstructions.emplace(0xA000, [this](const uint16_t opcode) { OpCode_SetIndexRegister(opcode); });	// ANNN
	mInstructions.emplace(0x2000, [this](const uint16_t opcode) { OpCode_PushSubroutine(opcode); });
	mInstructions.emplace(0x00EE, [this](const uint16_t opcode) { OpCode_PopSubroutine(opcode); });
}

void CHIP::LoadROM(const char* romPath)
{
	mProgramCounter = mStartingProgramCounter;
	std::ifstream rom(romPath, std::ios::binary);
	rom.read(reinterpret_cast<char*>(mMemory.data() + mProgramCounter), sizeof(mMemory) - mProgramCounter);
	mRomSize = rom.gcount();
	rom.close();
}

void CHIP::Process()
{
	const uint16_t instruction = Fetch();
	const uint16_t opcode = Decode(instruction);
	Execute(opcode, instruction);
}

uint16_t CHIP::Fetch()
{
	// Each instruction is two bytes, we want to shift the first byte to the most-significant slot, so we can fit in the second byte.
	const uint16_t instruction = (static_cast<uint16_t>(mMemory[mProgramCounter]) << 8) ^ static_cast<uint16_t>(mMemory[mProgramCounter + 1]);

	std::cout << "=== FETCH ===\n";
	std::cout << "First byte:" << std::bitset<8>(mMemory[mProgramCounter]) <<
		std::endl << "Second byte: " << std::bitset<8>(mMemory[mProgramCounter + 1]) <<
		std::endl << "Instruction: " << std::bitset<16>(instruction) << std::endl;

	// Read two successive bytes and combine into one 16-bit instruction
	mProgramCounter += 2;
	return instruction;
}

uint16_t CHIP::Decode(uint16_t instruction)
{
	// Grab the first nibble and determine the opcode
	const uint8_t nibble = instruction >> 12;

	std::cout << "=== DECODE ===\n";
	std::cout << std::bitset<4>(nibble) << std::endl;

	switch (nibble)
	{
	// Any case starting with 0x0 is explicit, so the entire instruction is the opcode.
	case 0x0:
		return instruction;
	// These opcodes are made distinct by the last literal nibble
	case 0x8:
		return (instruction & 0xF00F);
	// These opcodes are made distinct by literal 1st, 3rd and 4th nibbles
	case 0xF:
	case 0xE:
		return (instruction & 0x1011);
	// For these cases, the other nibbles are instructions and the first nibble is the op code.
	default:
		return (instruction & 0xF000);
	}
}

void CHIP::Execute(uint16_t opcode, uint16_t instruction)
{
	// Get X, Y, NN, NNN and pass to funcs
	std::cout << "=== EXECUTE ===\n";
	std::cout << "OpCode: " << std::bitset<16>(opcode) << std::endl;
	std::cout << "Instruction: " << std::bitset<16>(instruction) << std::endl;

	auto func = mInstructions.at(opcode);
	if (func)
	{
		func(instruction);
	}
	else
	{
		std::cout << "Error: Failed to find matching opcode executor.\n";
	}
}

uint8_t GetX(uint16_t instruction)
{
	return (instruction & 0x0F00) >> 8;
}

uint8_t GetY(uint16_t instruction)
{
	return (instruction & 0x00F0) >> 4;
}

uint8_t GetN(uint16_t instruction)
{
	return instruction & 0x000F;
}

uint8_t GetNN(uint16_t instruction)
{
	return instruction & 0x00FF;
}

uint16_t GetNNN(uint16_t instruction)
{
	return instruction & 0x0FFF;
}


#ifdef DEBUG
void CHIP::DrawDebug()
{
	constexpr int bytesPerRow = 8; // Number of bytes per row
	int numRows = static_cast<int>(mMemory.size() + bytesPerRow - 1) / bytesPerRow; // Calculate total rows

	// Start ImGui window
	ImGui::Begin("Hex Viewer");

	// Loop through rows
	for (int row = 0; row < numRows; ++row) {
		std::ostringstream rowStream;

		const bool fontRows = (row * bytesPerRow < 0x200);
		const bool romRows = (row * bytesPerRow < 0x200 + mRomSize);

		if (fontRows)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
		else if (romRows)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

		// Loop through columns in the row
		for (int col = 0; col < bytesPerRow; ++col) {
			int index = row * bytesPerRow + col;
			if (index < static_cast<int>(mMemory.size())) {
				// Add byte in hex format
				rowStream << std::hex << static_cast<int>(mMemory[index]) << " ";
			}
		}

		// Display the row in ImGui
		ImGui::Text("%s", rowStream.str().c_str());

		if (fontRows || romRows)
			ImGui::PopStyleColor();
	}

	// End ImGui window
	ImGui::End();
}
#endif


void CHIP::OpCode_ClearScreen(uint16_t instruction)
{
	std::cout << "=== Opcode 00E0: Clear Screen ===" << std::endl;
	// This is pretty simple: It should clear the display, turning all pixels off to 0.
	std::for_each(mDisplay.begin(), mDisplay.end(), [](bool pixel) {
		pixel &= 0;
		});
}

void CHIP::OpCode_Jump(uint16_t instruction)
{
	// This instruction should simply set PC to NNN, causing the program to jump to that memory location. Do not increment the PC afterwards, it jumps directly there.
	mProgramCounter = GetNNN(instruction);
}

void CHIP::OpCode_SetVxToNn(uint16_t instruction)
{
	std::cout << "=== Opcode 6XNN: Set VX to NN ===" << std::endl;
	// Simply set the register VX to the value NN.
	mVariableRegisters[GetX(instruction)] = GetNN(instruction);
}

void CHIP::OpCode_SetIndexRegister(uint16_t instruction)
{
	std::cout << "=== Opcode ANNN: Set Index Register ===" << std::endl;
	mIndexRegister = GetNNN(instruction);
}

void CHIP::OpCode_PushSubroutine(uint16_t instruction)
{
	// 2NNN calls the subroutine at memory location NNN. In other words, just like 1NNN, you should set PC to NNN. 
	// However, the difference between a jump and a call is that this instruction should first push the current PC to the stack, so the subroutine can return later.
	mAddressStack.push(mProgramCounter);
	mProgramCounter = GetNNN(instruction);
}

void CHIP::OpCode_PopSubroutine(uint16_t instruction)
{
	// Returning from a subroutine is done with 00EE, and it does this by removing (“popping”) the last address from the stack and setting the PC to it.
	mProgramCounter = mAddressStack.top();
	mAddressStack.pop();
}

void CHIP::OpCode_SkipIfVxNn(uint16_t instruction)
{
	// 3XNN will skip one instruction if the value in VX is equal to NN
}

void CHIP::OpCode_SkipIfVxNotNn(uint16_t instruction)
{
	// 4XNN will skip one instruction if the value in VX is NOT equal to NN
}

void CHIP::OpCode_SkipVxVyEqual(uint16_t instruction)
{
	// 5XY0 skips if the values in VX and VY are equal
}

void CHIP::OpCode_SkipVxVyNotEqual(uint16_t instruction)
{
	// 9XY0 skips if the values in VX and VY are not equal
}

void CHIP::OpCode_Add(uint16_t instruction)
{
	// Add the value NN to VX.

	// Note that on most other systems, and even in some of the other CHIP-8 instructions, this would set the carry flag if the result overflowed 8 bits; in other words, if the result of the addition is over 255.
	// For this instruction, this is not the case. If V0 contains FF and you execute 7001, the CHIP - 8’s flag register VF is not affected.
}