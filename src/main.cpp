#include "Chip8.h"
#include "Display.h"

static bool gDone;
// CHIP-8 has a 2:1 aspect ratio
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = WINDOW_WIDTH / 2;

int main()
{
	Display* display = new Display();
	CHIP* emu = new CHIP();

	display->Startup(WINDOW_WIDTH, WINDOW_HEIGHT, emu->GetDisplayWidth(), emu->GetDisplayHeight());

	// hardcoded path for testing
	emu->LoadROM("C:\\Users\\Molte\\Downloads\\IBMLogo.ch8");

	gDone = false;
	while (!gDone)
	{
		if (display->Update())
		{
			gDone = true;
		}

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