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

	void LoadROM(const char* romPath, uint16_t cyclesPerSecond = 700);
	void Update(const double deltaTime);
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

	inline const uint32_t* GetDisplay() { return mDisplay.data(); }
	inline const uint8_t GetDisplayWidth() { return DISPLAY_WIDTH; }
	inline const uint8_t GetDisplayHeight() { return DISPLAY_HEIGHT; }

	inline const bool IsPaused() { return mIsPaused; }

private:
	// Op Codes
	void OpCode_ClearScreen(uint16_t instruction);			// 00E0
	void OpCode_Jump(uint16_t instruction);					// 1NNN
	void OpCode_SetVxToNn(uint16_t instruction);			// 6XNN
	void OpCode_SetIndexRegister(uint16_t instruction);		// ANNN
	void OpCode_AddNnToVx(uint16_t instruction);			// 7XNN
	void OpCode_Display(uint16_t instruction);				// DXYN

	void OpCode_PushSubroutine(uint16_t instruction);		// 00EE
	void OpCode_PopSubroutine(uint16_t instruction);		// 2NNN
	void OpCode_SkipIfVxNn(uint16_t instruction);			// 3XNN
	void OpCode_SkipIfVxNotNn(uint16_t instruction);		// 4XNN
	void OpCode_SkipVxVyEqual(uint16_t instruction);		// 5XY0
	void OpCode_SkipVxVyNotEqual(uint16_t instruction);		// 9XY0
	void OpCode_Add(uint16_t instruction);					// 7XNN

	void OpCode_JumpWithOffset(uint16_t instruction);		// BNNN
	void OpCode_Random(uint16_t instruction);				// CXNN

	void OpCode_Set(uint16_t instruction);					// 8XY0
	void OpCode_BinaryOR(uint16_t instruction);				// 8XY1
	void OpCode_BinaryAND(uint16_t instruction);			// 8XY2
	void OpCode_LogicalXOR(uint16_t instruction);			// 8XY3
	void OpCode_AddWithCarry(uint16_t instruction);			// 8XY4
	void OpCode_SubtractVyFromVx(uint16_t instruction);		// 8XY5
	void OpCode_SubtractVxfromVy(uint16_t instruction);		// 8XY7
	void OpCode_ShiftRight(uint16_t instruction);			// 8XY6
	void OpCode_ShiftLeft(uint16_t instruction);			// 8XYE

	void OpCode_SkipIfKeyPressed(uint16_t instruction);		// EX9E
	void OpCode_SkipIfKeyNotPressed(uint16_t instruction);	// EXA1

	void OpCode_CacheDelayTimer(uint16_t instruction);		// FX07
	void OpCode_SetDelayTimer(uint16_t instruction);		// FX15
	void OpCode_SetSoundTimer(uint16_t instruction);		// FX18

	void OpCode_AddToIndexRegister(uint16_t instruction);	// FX1E
	void OpCode_GetKey(uint16_t instruction);				// FX0A
	void OpCode_SetFontCharacter(uint16_t instruction);		// FX29
	void OpCode_BinaryToDecimal(uint16_t instruction);		// FX33

	void OpCode_StoreMemory(uint16_t instruction);			// FX55
	void OpCode_LoadMemory(uint16_t instruction);			// FX65

private:
	std::array<uint8_t, 4096> mMemory = { 0 };
	std::unordered_map<uint16_t, std::function<void(uint16_t)>> mInstructions;
	std::array<uint8_t, 16> mVariableRegisters = { 0 };

	// display (64 x 32, or 128x64 for SUPER-CHIP)
	// Consuming more memory and using a 32 bit uint here makes SDL texture manipulation easier
	// Each element will either be 0x00000000 (off) or 0xFFFFFFFF (on)
	std::array<uint32_t, DISPLAY_WIDTH * DISPLAY_HEIGHT> mDisplay = { 0 };

	std::stack<uint16_t> mAddressStack;
	uint16_t mIndexRegister = 0;
	uint16_t mProgramCounter = 0x200;

	double mTimer = 0;
	uint8_t mDelayTimer = 0;
	uint8_t mSoundTimer = 0;

	const uint16_t mStartingProgramCounter = 0x200;
	uint8_t mRomSize = 0;
	double mSecondsPerCycle = 0;
	double mCycleTimer = 0;

#ifdef DEBUG
	uint16_t mPreviousInstruction = 0;
	uint16_t mNextInstruction = 0;

	bool mIsPaused = true;
#else
	const bool mIsPaused = false;
#endif
};