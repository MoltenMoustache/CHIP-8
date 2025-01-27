#include <iostream>
#include <SDL3/SDL.h>

#include <vector>
#include <map>
#include <array>
#include <stack>
#include <functional>
#include <bitset>

#ifdef DEBUG
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#endif

// Imgui to visualise memory and decoding
#ifndef USING_SUPER_CHIP
constexpr uint8_t DISPLAY_WIDTH = 64;
constexpr uint8_t DISPLAY_HEIGHT = 32;
#else
constexpr uint8_t DISPLAY_WIDTH = 128;
constexpr uint8_t DISPLAY_HEIGHT = 64;
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

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

#ifdef DEBUG
namespace Debug {

	static bool gImGuiOpen = true;
	ImVec4 gClearColour = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	static void Debug_Init()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		ImGui::StyleColorsDark();

		ImGui_ImplSDL3_InitForSDLRenderer(gSDLWindow, gSDLRenderer);
		ImGui_ImplSDLRenderer3_Init(gSDLRenderer);
	}

	static void Debug_Draw()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow(&gImGuiOpen);
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &gImGuiOpen);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &gImGuiOpen);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&gClearColour); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gSDLRenderer);
	}

	static void Debug_Shutdown()
	{
		ImGui_ImplSDLRenderer3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}
}
#endif



class CHIP {
public:
	CHIP()
	{
		memcpy(&mMemory[0x050], &gDefaultFont, sizeof(gDefaultFont));
	}

	std::array<uint8_t, 4096> mMemory = { 0 };
	// display (64 x 32, or 128x64 for SUPER-CHIP)
	std::array<bool, DISPLAY_WIDTH * DISPLAY_HEIGHT> mDisplay = { 0 };
	// program counter
	uint8_t mProgramCounter;
	// 16-bit index register
	uint16_t mIndexRegister;
	// stack for 16-bit addresses
	std::stack<uint16_t> mStack;
	// 8-bit delay timer
	uint8_t mDelayTimer;
	// 8-bit sound timer
	uint8_t mSoundTimer;
	//16 8-bit variable registers
	std::array<uint8_t, 16> mVariableRegisters = { 0 };

	std::map<uint8_t, std::function<void(uint16_t)>> mInstructions;

	void Fetch()
	{
		// Each instruction is two bytes, we want to shift the first byte to the most-significant slot, so we can fit in the second byte.
		const uint16_t instruction = (static_cast<uint16_t>(mMemory[mProgramCounter]) << 8) ^ static_cast<uint16_t>(mMemory[mProgramCounter + 1]);

		std::cout << "=== FETCH ===\n";
		std::cout << "First byte:" << std::bitset<8>(mMemory[mProgramCounter]) <<
			std::endl << "Second byte: " << std::bitset<8>(mMemory[mProgramCounter + 1]) <<
			std::endl << "Instruction: " << std::bitset<16>(instruction) << std::endl;

		// Read two successive bytes and combine into one 16-bit instruction
		mProgramCounter += 2;
	}

	void Decode(uint16_t instruction)
	{
		// Grab the first nibble, lookup in map of function pointers
		const uint8_t nibble = (instruction & 0xF000) >> 12;
		std::cout << "=== DECODE ===\n";
		std::cout << std::bitset<4>(nibble) << std::endl;
	}
};


void Update()
{
	// Exit app on pressing ESC
	SDL_Event e;
	if (SDL_PollEvent(&e))
	{
#ifdef DEBUG
		ImGui_ImplSDL3_ProcessEvent(&e); // Let ImGui handle the event
#endif
		if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_ESCAPE))
		{
			gDone = 1;
		}
	}

#ifdef DEBUG
	Debug::Debug_Draw();
#endif

	SDL_RenderTexture(gSDLRenderer, gSDLTexture, NULL, NULL);
	SDL_RenderPresent(gSDLRenderer);
}

void Render(uint64_t ticks)
{
}

int main()
{
	CHIP* emu = new CHIP();
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		return -1;
	}


	// TODO: replace with display from CHIP8
	gFrameBuffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT];
	gSDLWindow = SDL_CreateWindow("CHIP-8 Emulator", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
	gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

	if (!gFrameBuffer || !gSDLWindow || !gSDLRenderer || !gSDLTexture)
		return -1;

#ifdef DEBUG
	Debug::Debug_Init();
#endif

	gDone = 0;
	while (!gDone)
	{
		Update();
		Render(SDL_GetTicks());

	}

	SDL_DestroyTexture(gSDLTexture);
	SDL_DestroyRenderer(gSDLRenderer);
	SDL_DestroyWindow(gSDLWindow);
	SDL_Quit();

#ifdef DEBUG
	Debug::Debug_Shutdown();
#endif

	delete emu;

	return 0;
}