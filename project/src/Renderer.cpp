//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

// Standard includes
#include <limits>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	m_DepthBuffer.resize(m_Width * m_Height);
}

Renderer::~Renderer()
{
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	// Clear BackBuffer
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 50, 50, 50));

	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Temporary test vertices 
	std::vector<Vertex> verticesWorld {
		// Tri 0
		{ {  0.0f,  4.0f, 2.0f }, colors::Red },
		{ {  3.0f, -2.0f, 2.0f }, colors::Green },
		{ { -3.0f, -2.0f, 2.0f }, colors::Blue },

		// Tri 1
		{ {  0.0f,  2.0f, 0.0f }, colors::Red },
		{ {  1.5f, -1.0f, 0.0f }, colors::Red },
		{ { -1.5f, -1.0f, 0.0f }, colors::Red },
	};
	
	// Convert to Screen Space
	std::vector<Vertex> verticesScreen;
	VertexTransformationFunction(verticesWorld, verticesScreen);

	const size_t maxTri{ verticesScreen.size() / 3 };

	// Reset Depth buffer
	std::fill(begin(m_DepthBuffer), end(m_DepthBuffer), std::numeric_limits<float>::max());

	//RENDER LOGIC
	for (size_t triNum{ }; triNum < maxTri; triNum++) {
		const Vertex& v0 = verticesScreen[(triNum * 3) + 0];
		const Vertex& v1 = verticesScreen[(triNum * 3) + 1];
		const Vertex& v2 = verticesScreen[(triNum * 3) + 2];

		const Vector3 eV0 = v1.position - v0.position;
		const Vector3 eV1 = v2.position - v1.position;
		const Vector3 eV2 = v0.position - v2.position;

		const Vector2 min{
			std::clamp(std::min(std::min(v0.position.x, v1.position.x), v2.position.x), 0.0f, static_cast<float>(m_Width)),
			std::clamp(std::min(std::min(v0.position.y, v1.position.y), v2.position.y), 0.0f, static_cast<float>(m_Height))
		};
		const Vector2 max{
			std::clamp(std::max(std::max(v0.position.x, v1.position.x), v2.position.x), 0.0f, static_cast<float>(m_Width)),
			std::clamp(std::max(std::max(v0.position.y, v1.position.y), v2.position.y), 0.0f, static_cast<float>(m_Height))
		};

		// TODO : Change to uint, there's no need for these to be int
		for (int px{ static_cast<int>(min.x) }; px < static_cast<int>(max.x); ++px) {
			for (int py{ static_cast<int>(min.y) }; py < static_cast<int>(max.y); ++py) {
				const int index = px + (py * m_Width);
				const Vector2 pixel{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
				const Vector3 pixel3{ pixel.x, pixel.y, 1.0f };

				const Vector3 cV0 = pixel3 - v0.position;
				const float in0 = Vector3::Cross(eV0, cV0).z;

				const Vector3 cV1 = pixel3 - v1.position;
				const float in1 = Vector3::Cross(eV1, cV1).z;

				const Vector3 cV2 = pixel3 - v2.position;
				const float in2 = Vector3::Cross(eV2, cV2).z;

				if (in0 >= 0 && in1 >= 0 && in2 >= 0) {
					//const float magnitude = Vector3::Cross(eV0, -eV2).z; //-eV2 = vertices_screen[2].position - vertices_screen[0].position
					//const float magnitude = Vector3::Cross(eV0, v2.position - v0.position).z;
					const float magnitude = in0 + in1 + in2;

					const float w0 = in0 / magnitude;
					const float w1 = in1 / magnitude;
					const float w2 = in2 / magnitude;
					
					//const float inv0 = 1 / v0.position.z;
					//const float inv1 = 1 / v1.position.z;
					//const float inv2 = 1 / v2.position.z;
					//
					//const float depth = 1 / (
					//	w0 * inv0 +
					//	w1 * inv1 +
					//	w2 * inv2
					//);

					const float depth = 
						w0 * v0.position.z +
						w1 * v1.position.z +
						w2 * v2.position.z;
					
					//if (depth < 0.0f) continue;

					if (depth < m_DepthBuffer[index]) {
						m_DepthBuffer[index] = depth;

						ColorRGB finalColor =	(w0 * v0.color) +
												(w1 * v1.color) +
												(w2 * v2.color);
						
						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[index] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());
	
	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
	
	std::transform(begin(vertices_in), end(vertices_in), begin(vertices_out),
		[&](const Vertex& vert) {
			const Vector3 view = m_Camera.viewMatrix.TransformPoint(vert.position);
			
			const Vector3 ndc = {	view.x / (view.z * aspectRatio * m_Camera.fov),
									view.y / (view.z * m_Camera.fov),
									vert.position.z }; // Is this correct ???
	
			return Vertex{ Vector3{
				((ndc.x + 1.0f) / 2.0f) * m_Width,
				((1.0f - ndc.y) / 2.0f) * m_Height,
				ndc.z
			}, 
				vert.color
			};
		}
	);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
