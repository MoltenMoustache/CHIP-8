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

std::unordered_map<uint8_t, uint16_t> gOpcodeLookup =
{
	{0x8, 0xF00F},
	{0xF, 0xF0FF},
	{0xE, 0xF0FF},
	{0x0, 0xFFFF},
};

CHIP::CHIP()
	: mInstructions {
		{0x00E0, [this](const uint16_t opcode) { OpCode_ClearScreen(opcode); }},		// 00E0
		{0x1000, [this](const uint16_t opcode) { OpCode_Jump(opcode); }},				// 1NNN
		{0x6000, [this](const uint16_t opcode) { OpCode_SetVxToNn(opcode); }},			// 6XNN
		{0xA000, [this](const uint16_t opcode) { OpCode_SetIndexRegister(opcode); }},	// ANNN
		{0x7000, [this](const uint16_t opcode) { OpCode_AddNnToVx(opcode); }},			// 7XNN
		{0xD000, [this](const uint16_t opcode) { OpCode_Display(opcode); }},			// DXYN

		{0x2000, [this](const uint16_t opcode) { OpCode_PushSubroutine(opcode); }},		// 2NNN
		{0x00EE, [this](const uint16_t opcode) { OpCode_PopSubroutine(opcode); }},		// 00EE
		//{0x3000, [this](const uint16_t opcode) { OpCode_SkipIfVxNn(opcode); }},			// 3XNN
		//{0x4000, [this](const uint16_t opcode) { OpCode_SkipIfVxNotNn(opcode); }},		// 4XNN
		//{0x5000, [this](const uint16_t opcode) { OpCode_SkipVxVyEqual(opcode); }},		// 5XY0
		//{0x9000, [this](const uint16_t opcode) { OpCode_SkipVxVyNotEqual(opcode); }},	// 9XY0
		{0x7000, [this](const uint16_t opcode) { OpCode_Add(opcode); }},				// 7XNN

		//{0xB000, [this](const uint16_t opcode) { OpCode_JumpWithOffset(opcode); }},		// BNNN
		//{0xC000, [this](const uint16_t opcode) { OpCode_Random(opcode); }},				// CXNN

		{0x8000, [this](const uint16_t opcode) { OpCode_Set(opcode); }},				// 8XY0
		{0x8001, [this](const uint16_t opcode) { OpCode_BinaryOR(opcode); }},			// 8XY1
		{0x8002, [this](const uint16_t opcode) { OpCode_BinaryAND(opcode); }},			// 8XY2
		{0x8003, [this](const uint16_t opcode) { OpCode_LogicalXOR(opcode);	} },		// 8XY3
		{0x8004, [this](const uint16_t opcode) { OpCode_AddWithCarry(opcode); }},		// 8XY4
		{0x8005, [this](const uint16_t opcode) { OpCode_SubtractVyFromVx(opcode); }},	// 8XY5
		{0x8007, [this](const uint16_t opcode) { OpCode_SubtractVxfromVy(opcode); }},	// 8XY7
		//{0x8006, [this](const uint16_t opcode) { OpCode_ShiftRight(opcode); }},			// 8XY6
		//{0x800E, [this](const uint16_t opcode) { OpCode_ShiftLeft(opcode); }},			// 8XYE

		{0xE09E, [this](const uint16_t opcode) { OpCode_SkipIfKeyPressed(opcode); }},	// EX9E
		{0xE0A1, [this](const uint16_t opcode) { OpCode_SkipIfKeyNotPressed(opcode); }},// EXA1

		{0xF007, [this](const uint16_t opcode) { OpCode_CacheDelayTimer(opcode); }},	// FX07
		{0xF015, [this](const uint16_t opcode) { OpCode_SetDelayTimer(opcode); }},		// FX15
		{0xF018, [this](const uint16_t opcode) { OpCode_SetSoundTimer(opcode); }},		// FX18

		{0xF01E, [this](const uint16_t opcode) { OpCode_AddToIndexRegister(opcode); }},	// FX1E
		{0xF00A, [this](const uint16_t opcode) { OpCode_GetKey(opcode); }},				// FX0A
		//{0xF029, [this](const uint16_t opcode) { OpCode_SetFontCharacter(opcode); }},	// FX29
		//{0xF033, [this](const uint16_t opcode) { OpCode_BinaryToDecimal(opcode); }},	// FX33
		
		//{0xF055, [this](const uint16_t opcode) { OpCode_StoreMemory(opcode); }},	// FX55
		//{0xF065, [this](const uint16_t opcode) { OpCode_LoadMemory(opcode); }},	// FX65
	}
{
	memcpy(&mMemory[0x050], &gDefaultFont, sizeof(gDefaultFont));
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
	// Grab the first nibble and determine the opcode
	const uint8_t nibble = instruction >> 12;
	const uint16_t opcodeMask = gOpcodeLookup.contains(nibble) ? gOpcodeLookup[nibble] : 0xF000;
	
	return instruction & opcodeMask;
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
	std::fill(mDisplay.begin(), mDisplay.end(), 0);
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
	mVariableRegisters[GetX(instruction)] = GetNN(instruction);
}

void CHIP::OpCode_JumpWithOffset(uint16_t instruction)
{
	// TODO: Ambiguous instruction, add support for toggling quirk
}

void CHIP::OpCode_Set(uint16_t instruction)
{
	// VX is set to the value of VY.
	mVariableRegisters[GetX(instruction)] = mVariableRegisters[GetY(instruction)];
}

void CHIP::OpCode_BinaryOR(uint16_t instruction)
{
	// VX is set to the bitwise/binary logical disjunction (OR) of VX and VY. VY is not affected.
	const uint8_t vx = mVariableRegisters[GetX(instruction)];
	const uint8_t vy = mVariableRegisters[GetY(instruction)];
	mVariableRegisters[GetX(instruction)] = vx | vy;
}

void CHIP::OpCode_BinaryAND(uint16_t instruction)
{
	// VX is set to the bitwise/binary logical conjunction (AND) of VX and VY. VY is not affected.
	const uint8_t vx = mVariableRegisters[GetX(instruction)];
	const uint8_t vy = mVariableRegisters[GetY(instruction)];
	mVariableRegisters[GetX(instruction)] = vx & vy;
}

void CHIP::OpCode_LogicalXOR(uint16_t instruction)
{
	// VX is set to the bitwise/binary exclusive OR (XOR) of VX and VY. VY is not affected.
	const uint8_t vx = mVariableRegisters[GetX(instruction)];
	const uint8_t vy = mVariableRegisters[GetY(instruction)];
	mVariableRegisters[GetX(instruction)] = vx ^ vy;
}

void CHIP::OpCode_AddWithCarry(uint16_t instruction)
{
	// TODO: Remove conditional
	// VX is set to the value of VX plus the value of VY. VY is not affected.
	const uint8_t vx = mVariableRegisters[GetX(instruction)];
	const uint8_t vy = mVariableRegisters[GetY(instruction)];
	mVariableRegisters[GetX(instruction)] = vx + vy;

	// Unlike 7XNN, this addition will affect the carry flag.
	// If the result is larger than 255 (and thus overflows the 8-bit register VX), the flag register VF is set to 1.
	// If it doesn’t overflow, VF is set to 0.
	mVariableRegisters[0xF] = vx + vy > 255 ? 1 : 0;
}

void CHIP::OpCode_SubtractVyFromVx(uint16_t instruction)
{
	// 8XY5 sets VX to the result of VX - VY.
	const uint8_t vx = mVariableRegisters[GetX(instruction)];
	const uint8_t vy = mVariableRegisters[GetY(instruction)];
	mVariableRegisters[GetX(instruction)] = vx - vy;

	//This subtraction will also affect the carry flag, but note that it’s opposite from what you might think.
	// If the minuend (the first operand) is larger than the subtrahend (second operand), VF will be set to 1.
	// If the subtrahend is larger, and we “underflow” the result, VF is set to 0.
	// Another way of thinking of it is that VF is set to 1 before the subtraction, and then the subtraction either borrows from VF (setting it to 0) or not.
	mVariableRegisters[0xF] = vx > vy ? 1 : 0;
}

void CHIP::OpCode_SubtractVxfromVy(uint16_t instruction)
{
	// 8XY5 sets VX to the result of VY - VX.
	const uint8_t vx = mVariableRegisters[GetX(instruction)];
	const uint8_t vy = mVariableRegisters[GetY(instruction)];
	mVariableRegisters[GetX(instruction)] = vy - vx;

	//This subtraction will also affect the carry flag, but note that it’s opposite from what you might think.
	// If the minuend (the first operand) is larger than the subtrahend (second operand), VF will be set to 1.
	// If the subtrahend is larger, and we “underflow” the result, VF is set to 0.
	// Another way of thinking of it is that VF is set to 1 before the subtraction, and then the subtraction either borrows from VF (setting it to 0) or not.
	mVariableRegisters[0xF] = vy > vx ? 1 : 0;
}

void CHIP::OpCode_ShiftRight(uint16_t instruction)
{
	// TODO: Ambiguous instruction, add support for toggling quirk
}

void CHIP::OpCode_ShiftLeft(uint16_t instruction)
{
	// TODO: Ambiguous instruction, add support for toggling quirk
}

void CHIP::OpCode_CacheDelayTimer(uint16_t instruction)
{
	// FX07 sets VX to the current value of the delay timer
	mVariableRegisters[GetX(instruction)] = mDelayTimer;
}

void CHIP::OpCode_SetDelayTimer(uint16_t instruction)
{
	// FX15 sets the delay timer to the value in VX
	mDelayTimer = mVariableRegisters[GetX(instruction)];
}

void CHIP::OpCode_SetSoundTimer(uint16_t instruction)
{
	// FX18 sets the sound timer to the value in VX
	mSoundTimer = mVariableRegisters[GetX(instruction)];
}

void CHIP::OpCode_AddToIndexRegister(uint16_t instruction)
{
	// The index register I will get the value in VX added to it.
	// TODO: Handle overflow for Spaceflight 2091!
	mIndexRegister += mVariableRegisters[GetX(instruction)];
}

void CHIP::OpCode_GetKey(uint16_t instruction)
{
	// This opcode will wait for a key to be pressed before incrementing the program counter
	// As we already incremented the program counter in the Fetch step, we decrement it here first.
	mProgramCounter -= 2;

	// This will increment if any button is currently being pressed, it will not increment WHEN a button is pressed.
	for (int i = 0; i < mKeypad.size(); ++i)
	{
		if (mKeypad[i])
		{
			mProgramCounter += 2;
			mVariableRegisters[GetX(instruction)] = i;
		}
	}
}

void CHIP::OpCode_SetFontCharacter(uint16_t instruction)
{
	// The index register I is set to the address of the hexadecimal character in VX.
}

void CHIP::OpCode_SkipIfKeyPressed(uint16_t instruction)
{
	// EX9E will skip one instruction (increment PC by 2) if the key corresponding to the value in VX is pressed.
	const uint8_t key = mVariableRegisters[GetX(instruction)];
	if (mKeypad[key])
	{
		mProgramCounter += 2;
	}
}

void CHIP::OpCode_SkipIfKeyNotPressed(uint16_t instruction)
{
	// EXA1 skips if the key corresponding to the value in VX is not pressed.
	const uint8_t key = mVariableRegisters[GetX(instruction)];
	if (!mKeypad[key])
	{
		mProgramCounter += 2;
	}
}