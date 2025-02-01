#pragma once

#include <map>
#include <array>
#include <stack>
#include <functional>


constexpr uint8_t DISPLAY_WIDTH = 64;
constexpr uint8_t DISPLAY_HEIGHT = 32;

class CHIP {
public:
	CHIP();

	void LoadROM(const char* romPath);
	void Process();
	// Constructs a full instruction from the memory index of mProgramCounter
	uint16_t Fetch();
	// Constructs an opcode, based on the nibbles of the full instruction
	uint16_t Decode(uint16_t instruction);
	// Executes the instruction using the opcode
	void Execute(uint16_t opcode, uint16_t instruction);

#ifdef DEBUG
	void DrawDebug();
#endif

private:
	// Op Codes
	void OpCode_ClearScreen(uint16_t instruction);			// 00E0
	void OpCode_Jump(uint16_t instruction);					// 1NNN
	void OpCode_SetVxToNn(uint16_t instruction);			// 6XNN
	void OpCode_SetIndexRegister(uint16_t instruction);		// ANNN

	void OpCode_PushSubroutine(uint16_t instruction);		// 00EE
	void OpCode_PopSubroutine(uint16_t instruction);		// 2NNN
	void OpCode_SkipIfVxNn(uint16_t instruction);			// 3XNN
	void OpCode_SkipIfVxNotNn(uint16_t instruction);		// 4XNN
	void OpCode_SkipVxVyEqual(uint16_t instruction);		// 5XY0
	void OpCode_SkipVxVyNotEqual(uint16_t instruction);		// 9XY0
	void OpCode_Add(uint16_t instruction);					// 7XNN

private:
	std::array<uint8_t, 4096> mMemory = { 0 };
	std::unordered_map<uint16_t, std::function<void(uint16_t)>> mInstructions;
	std::array<uint8_t, 16> mVariableRegisters = { 0 };

	// display (64 x 32, or 128x64 for SUPER-CHIP)
	std::array<bool, DISPLAY_WIDTH* DISPLAY_HEIGHT> mDisplay = { 0 };

	uint16_t mIndexRegister;
	uint16_t mProgramCounter = 0x200;
	std::stack<uint16_t> mAddressStack;

	uint8_t mDelayTimer;
	uint8_t mSoundTimer;

	const uint16_t mStartingProgramCounter = 0x200;
	uint8_t mRomSize = 0;
};