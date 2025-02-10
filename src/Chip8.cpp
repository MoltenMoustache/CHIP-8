#include "Chip8.h"
#include <iostream>

#include <vector>

#include <fstream>
#include <algorithm>
#include <sstream>

#include <cassert>

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
	mInstructions.reserve(64);
	mInstructions.emplace(0x00E0, [this](const uint16_t opcode) { OpCode_ClearScreen(opcode); });		// 00E0
	mInstructions.emplace(0x1000, [this](const uint16_t opcode) { OpCode_Jump(opcode); });				// 1NNN
	mInstructions.emplace(0x6000, [this](const uint16_t opcode) { OpCode_SetVxToNn(opcode); });			// 6XNN
	mInstructions.emplace(0xA000, [this](const uint16_t opcode) { OpCode_SetIndexRegister(opcode); });	// ANNN
	mInstructions.emplace(0x7000, [this](const uint16_t opcode) { OpCode_AddNnToVx(opcode); });			// 7XNN
	mInstructions.emplace(0xD000, [this](const uint16_t opcode) { OpCode_Display(opcode); });			// DXYN
	mInstructions.emplace(0x2000, [this](const uint16_t opcode) { OpCode_PushSubroutine(opcode); });
	mInstructions.emplace(0x00EE, [this](const uint16_t opcode) { OpCode_PopSubroutine(opcode); });
}

void CHIP::LoadROM(const char* romPath, uint16_t cyclesPerSecond /* = 700 */)
{
	mSecondsPerCycle = 1.0 / cyclesPerSecond;

	mProgramCounter = mStartingProgramCounter;
	std::ifstream rom(romPath, std::ios::binary);
	rom.read(reinterpret_cast<char*>(mMemory.data() + mProgramCounter), sizeof(mMemory) - mProgramCounter);
	mRomSize = rom.gcount();
	rom.close();
}

void CHIP::Update(const double deltaTime)
{
	if (IsPaused())
	{
		return;
	}

	mTimer += deltaTime;
	mCycleTimer += deltaTime;

	// Timers need to be decremented by 1 every second
	const int timerDecrement = static_cast<int>(mTimer);
	mDelayTimer = std::clamp(mDelayTimer - timerDecrement, 0, 255);
	mSoundTimer = std::clamp(mSoundTimer - timerDecrement, 0, 255);
	mTimer -= timerDecrement;

	// Ensure no missed instructions between updates.
	const int cyclesToRun = static_cast<int>(mCycleTimer / mSecondsPerCycle);
	for (int i = 0; i < cyclesToRun; ++i)
	{
		Process();
	}

	// Only subtract time used, to ensure no lost time between updates.
	mCycleTimer -= cyclesToRun * mSecondsPerCycle;
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

	// Read two successive bytes and combine into one 16-bit instruction
	mProgramCounter += 2;

#ifdef DEBUG
	mPreviousInstruction = instruction;
	mNextInstruction = (static_cast<uint16_t>(mMemory[mProgramCounter]) << 8) ^ static_cast<uint16_t>(mMemory[mProgramCounter + 1]);
#endif

	return instruction;
}

uint16_t CHIP::Decode(uint16_t instruction)
{
	// Would like to find a solution that removes these conditionals from the main loop
	// Multiple arrays with the opcode as indexes?

	// Grab the first nibble and determine the opcode
	const uint8_t nibble = instruction >> 12;
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
		return (instruction & 0xF0FF);
	// For these cases, the other nibbles are instructions and the first nibble is the op code.
	default:
		return (instruction & 0xF000);
	}
}

void CHIP::Execute(uint16_t opcode, uint16_t instruction)
{
	assert(mInstructions.contains(opcode), "Opcode not found in instruction set.");
	auto& func = mInstructions.at(opcode);
	func(instruction);
}

// Used to lookup the variable register at this position
uint8_t GetX(uint16_t instruction)
{
	return (instruction & 0x0F00) >> 8;
}

// Used to lookup the variable register at this position
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

std::string GetHexString(uint16_t num)
{
	std::stringstream output;
	output << std::hex << num;
	return output.str();
}


#ifdef DEBUG
void CHIP::DrawDebug()
{
	ImGui::Begin("CHIP-8 Debug Controls");
	ImGui::Text("Last Instruction: %s", GetHexString(mPreviousInstruction).c_str());
	ImGui::Text("Next Instruction: %s", GetHexString(mNextInstruction).c_str());
	if (ImGui::Button("Process Next Instruction"))
	{
		Process();
	}

	ImGui::Checkbox("Pause Emulation", &mIsPaused);
	ImGui::End();

	if (mRomSize > 0)
	{
		const size_t bytesPerRow = 8;
		ImGui::Begin("ROM Viewer");

		// Loop through rows
		for (int i = 0; i < mRomSize; ++i)
		{
			const size_t romIndex = mStartingProgramCounter + i;
			const bool isCurrentPC = mProgramCounter == romIndex;
			if (i % bytesPerRow != 0)
			{
				ImGui::SameLine();
			}

			if (isCurrentPC)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

			ImGui::Text("%s", GetHexString(mMemory[romIndex]).c_str());

			if (isCurrentPC)
				ImGui::PopStyleColor();
		}

		// End ImGui window
		ImGui::End();
	}
}
#endif


void CHIP::OpCode_ClearScreen(uint16_t instruction)
{
	std::cout << "=== Opcode 00E0: Clear Screen ===" << std::endl;
	// This is pretty simple: It should clear the display, turning all pixels off to 0.
	std::for_each(mDisplay.begin(), mDisplay.end(), [](uint32_t& pixel) {
		pixel &= 0x0;
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

void CHIP::OpCode_AddNnToVx(uint16_t instruction)
{
	// Add the value NN to VX.
	mVariableRegisters[GetX(instruction)] += GetNN(instruction);
}

void CHIP::OpCode_Display(uint16_t instruction)
{
	std::cout << "=== Opcode DXYN: Display ===" << std::endl;

	const uint8_t xPos = mVariableRegisters[GetX(instruction)] % DISPLAY_WIDTH;
	const uint8_t yPos = mVariableRegisters[GetY(instruction)] % DISPLAY_HEIGHT;

	for (uint8_t row = 0; row < GetN(instruction); ++row)
	{
		for (uint8_t col = 0; col < 8; ++col)
		{
			//If the current pixel in the sprite row is on and the pixel at coordinates X,Y on the screen is also on, turn off the pixel and set VF to 1
			const uint8_t spritePixel = mMemory[mIndexRegister + row] & (0b10000000 >> col);
			uint32_t* displayPixel = &mDisplay[(yPos + row) * DISPLAY_WIDTH + xPos + col];

			// If it's non-zero, this will flip the result to 0, and then flip it again to 1; then negate it as -1 equates to 0xFFFFFFFF
			const uint32_t spriteMask = -!!spritePixel;

			// collision will either be 0x00000000 if either pixels are off, or 0xFFFFFFFF if both pixels are on
			const uint32_t collision = spriteMask & *displayPixel;
			
			// by shifting collision to the right, we end up with 1 bit that is either 0 or 1
			const bool bothPixelsOn = (collision >> 31) & 1;

			// Bitwise OR'ing it means VF will be set to 1 if VF is currently zero
			mVariableRegisters[0xF] |= bothPixelsOn;

			*displayPixel ^= spriteMask;
		}
	}
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
	// Returning from a subroutine is done with 00EE, and it does this by removing (�popping�) the last address from the stack and setting the PC to it.
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
	// For this instruction, this is not the case. If V0 contains FF and you execute 7001, the CHIP - 8�s flag register VF is not affected.
}