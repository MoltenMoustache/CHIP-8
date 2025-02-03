#include "Display.h"

#ifdef DEBUG
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#endif

#ifdef DEBUG
namespace Debug {

	static bool gImGuiOpen = true;
	ImVec4 gClearColour = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	static void Debug_Init(SDL_Window* window, SDL_Renderer* renderer)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		ImGui::StyleColorsDark();

		ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
		ImGui_ImplSDLRenderer3_Init(renderer);
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

bool Display::Startup(const int windowWidth, const int windowHeight, const int textureWidth, const int textureHeight)
{
	mWindow = SDL_CreateWindow("CHIP-8 Emulator", windowWidth, windowHeight, 0);
	mRenderer = SDL_CreateRenderer(mWindow, NULL);
	mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

	// Enables 'pixel perfect' texture scaling
	SDL_SetTextureScaleMode(mTexture, SDL_SCALEMODE_NEAREST);

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		return false;
	}

#ifdef DEBUG
	Debug::Debug_Init(mWindow, mRenderer);
#endif
}

void Display::Shutdown()
{
#ifdef DEBUG
	Debug::Debug_Shutdown();
#endif

	SDL_DestroyTexture(mTexture);
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
}


bool Display::Update()
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
			return true;
		}
	}

	return false;
}

void Display::RenderBegin()
{
	SDL_RenderClear(mRenderer);

#ifdef DEBUG
	Debug::Debug_Draw();
#endif
}

void Display::RenderEnd(const uint32_t* pixelBuffer, const int rowWidth)
{
	const int pitch = sizeof(pixelBuffer[0]) * rowWidth;
	SDL_UpdateTexture(mTexture, nullptr, pixelBuffer, pitch);
	SDL_RenderTexture(mRenderer, mTexture, nullptr, nullptr);

#ifdef DEBUG

	ImGui::Render();
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);
#endif
	SDL_RenderPresent(mRenderer);
}