#pragma once
#include <cstdint>
#include <vector>
#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;

		void ToggleBoundsRender() { m_RenderBounds = !m_RenderBounds; }

	private:
		void UpdateColor(const int& index, ColorRGB& col); // HACK : Not referenced as const, is kinda bad practice

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		std::vector<float> m_DepthBuffer{}; // HACK : For later use, prob change this to a raw array

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		bool m_RenderBounds{ false };
	};
}
