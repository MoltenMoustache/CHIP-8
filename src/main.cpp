#include "Chip8.h"
#include "Display.h"
#include <SDL3/SDL.h>

static bool gDone;
// CHIP-8 has a 2:1 aspect ratio
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = WINDOW_WIDTH / 2;

std::array<uint8_t, 16> gKeymap = {
	SDLK_X,
	SDLK_1,
	SDLK_2,
	SDLK_3,
	SDLK_Q,
	SDLK_W,
	SDLK_E,
	SDLK_A,
	SDLK_S,
	SDLK_D,
	SDLK_Z,
	SDLK_C,
	SDLK_4,
	SDLK_R,
	SDLK_F,
	SDLK_V,
};

void HandleInput(const SDL_Event& e, bool* keys)
{
	// Exit app on pressing ESC
	if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_ESCAPE))
	{
		gDone = true;
	}

	// process input if its in our designated keymap
	for (int i = 0; i < gKeymap.size(); ++i)
	{
		const bool isKeyUp = e.type == SDL_EVENT_KEY_UP ? 1 : 0;
		if (e.key.key == gKeymap[i])
		{
			keys[i] = isKeyUp;
		}
	}
}

int main()
{
	Display* display = new Display();
	CHIP* emu = new CHIP();

	display->Startup(WINDOW_WIDTH, WINDOW_HEIGHT, emu->GetDisplayWidth(), emu->GetDisplayHeight());

	// hardcoded path and speed for testing
	//emu->LoadROM("C:\\Users\\Molte\\Downloads\\IBMLogo.ch8", 700);
	emu->LoadROM("C:\\Users\\Molte\\Downloads\\6-keypad.ch8", 700);

	gDone = false;
	uint64_t lastCounter = SDL_GetPerformanceCounter();
	const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
	
	while (!gDone)
	{
		const uint64_t newCounter = SDL_GetPerformanceCounter();
		const double deltaTime = (newCounter - lastCounter) / frequency;
		lastCounter = newCounter;

		SDL_Event e;
		if (SDL_PollEvent(&e))
		{
			display->Update(&e);
			HandleInput(e, emu->GetKeypad());
		}

		emu->Update(deltaTime);

		display->RenderBegin();
#ifdef DEBUG
		emu->DrawDebug();
#endif
		display->RenderEnd(emu->GetDisplay(), emu->GetDisplayWidth());
	}

	display->Shutdown();

	delete display;
	delete emu;

	return 0;
}