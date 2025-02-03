#pragma once

#include <SDL3/SDL.h>

class Display {
public:
	bool Startup(const int windowWidth, const int windowHeight, const int textureWidth, const int textureHeight);
	void Shutdown();

	bool Update();
	void RenderBegin();
	void RenderEnd(const uint32_t* pixelBuffer, const int pitch);

private:
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	SDL_Texture* mTexture;
};