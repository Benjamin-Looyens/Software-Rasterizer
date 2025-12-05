//External includes
#include "SDL.h"
#include "SDL_surface.h"
#undef main

//Standard includes
#include <iostream>

//Project includes
#include "Timer.h"
#include "Renderer.h"
#if defined(_DEBUG)
#include "LeakDetector.h"
#endif

using namespace dae;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

void PrintInputs()
{
	std::cout <<
		"************** INPUTS **************\n" <<
		"H  - Print Input Mappings\n" <<
		"R  - Reset Camera Position\n" <<
		"X  - Take Screenshot\n" <<
		"B  - Toggle Bounding Box Rendering\n" <<
		"F  - Toggle Wireframe Rendering\n" <<
		"F4 - Toggle Depth Buffer Rendering\n" <<
		"F5 - Toggle Mesh Rotation\n" <<
		"F6 - Toggle Sampling Normal Map\n" <<
		"F7 - Cycle Through Lighting Modes\n" <<
		"F8 - Start Performance Benchmark\n"
		"************************************\n";
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	// Leak detection
	#if defined(_DEBUG)
		LeakDetector detector{};
	#endif

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"Rasterizer - **Benjamin Looyens 2DAE11**",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();

	// Print Starter Inputs
	PrintInputs();

	float printTimer = 0.f;
	bool isLooping = true;
	bool takeScreenshot = false;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				if (e.key.keysym.scancode == SDL_SCANCODE_H)
					PrintInputs();
				if (e.key.keysym.scancode == SDL_SCANCODE_R)
					pRenderer->ResetCamera();
				if (e.key.keysym.scancode == SDL_SCANCODE_X)
					takeScreenshot = true;
				if (e.key.keysym.scancode == SDL_SCANCODE_B)
					pRenderer->ToggleBoundsRender();
				if (e.key.keysym.scancode == SDL_SCANCODE_F)
					pRenderer->ToggleWireFrameRender();
				if (e.key.keysym.scancode == SDL_SCANCODE_F4)
					pRenderer->ToggleDepthRender();
				if (e.key.keysym.scancode == SDL_SCANCODE_F5)
					pRenderer->ToggleRotation();
				if (e.key.keysym.scancode == SDL_SCANCODE_F6)
					pRenderer->ToggleNormalMap();
				if (e.key.keysym.scancode == SDL_SCANCODE_F7)
					pRenderer->CycleLightingMode();
				if (e.key.keysym.scancode == SDL_SCANCODE_F8)
					pTimer->StartBenchmark();
				break;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			if (!pTimer->IsDoingBenchmark()) std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}

		//Save screenshot after full render
		if (takeScreenshot)
		{
			if (!pRenderer->SaveBufferToImage())
				std::cout << "Screenshot saved!" << std::endl;
			else
				std::cout << "Something went wrong. Screenshot not saved!" << std::endl;
			takeScreenshot = false;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}