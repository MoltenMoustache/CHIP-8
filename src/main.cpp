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

	emu->LoadROM("C:\\Users\\Molte\\Downloads\\IBMLogo.ch8");
	emu->Process();
	emu->Process();
	emu->Process();

	gDone = 0;
	while (!gDone)
	{
		Update();
#ifdef DEBUG
		emu->DrawDebug();

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gSDLRenderer);
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