//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

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
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	// Clear buffer
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));

	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Temporary vertices 
	std::vector<Vertex> vertices_world {
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
	std::vector<Vertex> vertices_screen;
	VertexTransformationFunction(vertices_world, vertices_screen);

	const size_t maxTri{ vertices_screen.size() / 3 };

	// Depth buffer
	std::vector<float> depthBuffer(m_Width * m_Height);
	std::fill(begin(depthBuffer), end(depthBuffer), FLT_MAX);
	//std::fill_n(begin(depthBuffer), m_Width * m_Height, FLT_MAX);

	//RENDER LOGIC
	for (size_t triNum{ }; triNum < maxTri; triNum++) {
		const Vertex& v0 = vertices_screen[(triNum * 3) + 0];
		const Vertex& v1 = vertices_screen[(triNum * 3) + 1];
		const Vertex& v2 = vertices_screen[(triNum * 3) + 2];

		const Vector3 eV0 = v1.position - v0.position;
		const Vector3 eV1 = v2.position - v1.position;
		const Vector3 eV2 = v0.position - v2.position;

		for (int px{}; px < m_Width; ++px) {
			for (int py{}; py < m_Height; ++py) {
				ColorRGB finalColor{ };
				const Vector2 pixel{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
				const Vector3 pixel3{ pixel.x, pixel.y, 0.0f };

				const Vector3 cV0 = pixel3 - v0.position;
				const float in0 = Vector3::Cross(eV0, cV0).z;

				const Vector3 cV1 = pixel3 - v1.position;
				const float in1 = Vector3::Cross(eV1, cV1).z;

				const Vector3 cV2 = pixel3 - v2.position;
				const float in2 = Vector3::Cross(eV2, cV2).z;

				if (in0 > 0 && in1 > 0 && in2 > 0) {
					const float magnitude = Vector3::Cross(eV0, -eV2).z; //-eV2 = vertices_screen[2].position - vertices_screen[0].position

					const float w0 = in0 / magnitude;
					const float w1 = in1 / magnitude;
					const float w2 = in2 / magnitude;

					const float depth = 
						w0 * (v0.position.z / 3) +
						w1 * (v1.position.z / 3) +
						w2 * (v2.position.z / 3);

					if (depth < depthBuffer[px + (py * m_Width)]) {
						depthBuffer[px + (py * m_Width)] = depth;

						finalColor =	(w0 * v0.color) +
										(w1 * v1.color) +
										(w2 * v2.color);

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
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

	// Steps:
	// Transform with Camera VIEW matrix
	// Apply perspective divide
	// Convert NDC > Screen space
	
	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };

	std::transform(begin(vertices_in), end(vertices_in), begin(vertices_out),
		[&](const Vertex& vert) {
			const Vector3 vertToCam = m_Camera.viewMatrix.TransformPoint(vert.position);
			
			const Vector3 project {
				vertToCam.x / vertToCam.z,
				vertToCam.y / vertToCam.z,
				vertToCam.z
			};
			
			const Vector3 ndc = {	project.x / (aspectRatio * m_Camera.fov),
									project.y / (m_Camera.fov),
									project.z };

			return Vertex{ Vector3{
				((ndc.x + 1.0f) / 2.0f)* m_Width,
				((1.0f - ndc.y) / 2.0f)* m_Height,
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
