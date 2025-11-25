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
	struct Vertex_Out;
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

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldMat) const;

		void ToggleBoundsRender() { m_RenderBounds = !m_RenderBounds; }
		void ToggleDepthRender() { m_RenderDepth = !m_RenderDepth; }
		void ToggleRotation() { m_DoRotation = !m_DoRotation; }

		void CycleLightingMode();

		void ResetCamera();

		enum class LightingMode : uint8_t
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined,

			ModeAmount
		}; 
		// Does it need a depth render mode?

	private:
		void UpdateColor(const int& index, ColorRGB& col); // HACK : Not referenced as const, is kinda bad practice

		LightingMode m_CurMode{ LightingMode::Diffuse };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		
		float m_RotCounter{};
		Matrix m_WorldMatrix{};

		std::vector<Mesh> m_Meshes;
		
		float* m_pDepthBuffer{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		bool m_RenderBounds{ false };
		bool m_RenderDepth{ false };
		
		bool m_DoRotation{ false };

		const Vector3 m_CameraStartPos{ 0.0f, 0.0f, -50.0f }; // TODO : Change to be 0.0f in the Z, and have the mesh be at 50.0f at Z
	};
}
