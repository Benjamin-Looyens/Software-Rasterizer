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

		void VertexTransformationFunction(const Matrix& worldMat, const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const;

		void ToggleBoundsRender();
		void ToggleDepthRender();
		void ToggleRotation();
		void ToggleNormalMap(); 

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
		void UpdateColor(const uint32_t& index, ColorRGB& col); // HACK : Not referenced as const, is kinda bad practice

		LightingMode m_CurMode{ LightingMode::Combined };

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

		bool m_DoBoundsRender{ false };
		bool m_DoDepthRender{ false };
		bool m_DoRotation{ false };
		bool m_UseNormalMap{ true };

		bool m_VerticesDirty{ true };

		const Vector3 m_CameraStartPos{ 0.0f, 0.0f, 0.0f };
		const Vector3 m_MeshPos{ 0.0f, 0.0f, 50.0f };
	};
}
