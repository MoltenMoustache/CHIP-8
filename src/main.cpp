#include <iostream>
#include <SDL3/SDL.h>

#include <vector>
#include <map>
#include <array>
#include <stack>
#include <functional>
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

#include "Chip8.h"

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
	}

	static void Debug_Shutdown()
	{
		ImGui_ImplSDLRenderer3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}
}
#endif

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
}

void Render(uint64_t ticks)
{
#ifdef DEBUG
	ImGui::Render();
#endif
	SDL_RenderClear(gSDLRenderer);

#ifdef DEBUG
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gSDLRenderer);
#endif
	SDL_RenderPresent(gSDLRenderer);
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

	emu->LoadROM("C:\\Users\\Molte\\Downloads\\IBMLogo.ch8");
	emu->Process();
	emu->Process();
	emu->Process();

	gDone = 0;
	while (!gDone)
	{
		Update();
#ifdef DEBUG
		Debug::Debug_Draw();
		emu->DrawDebug();
#endif
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