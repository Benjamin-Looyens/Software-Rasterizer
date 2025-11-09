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
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Temporary vertices 
	std::vector<Vertex> vertices_world {
		{ {  0.0f,  4.0f, 2.0f }, colors::Red },
		{ {  3.0f, -2.0f, 2.0f }, colors::Green },
		{ { -3.0f, -2.0f, 2.0f }, colors::Blue }
	};
	
	// Convert to Screen Space
	std::vector<Vertex> vertices_screen;
	VertexTransformationFunction(vertices_world, vertices_screen);

	// General logic outline:
	//		For each triangle:
	//			For each pixel:
	//				is the pixel within this triangle

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px) {
		for (int py{}; py < m_Height; ++py) {
			const Vector2 pixel{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
			ColorRGB finalColor{ };

			// Loop over all triangles with vertices defined in NDC space
			//	Loop over all pixels & check if pixel is in triangle, "using same technique as raytracer (Triangle Intersection Test)"
			//		If in triangle, color white

			const Vector3 pixel3{ pixel.x, pixel.y, 0.0f };

			const Vector3 eV0 = vertices_screen[1].position - vertices_screen[0].position;
			const Vector3 cV0 = pixel3 - vertices_screen[0].position;
			const float in0 = Vector3::Cross(eV0, cV0).z;

			const Vector3 eV1 = vertices_screen[2].position - vertices_screen[1].position;
			const Vector3 cV1 = pixel3 - vertices_screen[1].position;
			const float in1 = Vector3::Cross(eV1, cV1).z;

			const Vector3 eV2 = vertices_screen[0].position - vertices_screen[2].position;
			const Vector3 cV2 = pixel3 - vertices_screen[2].position;
			const float in2 = Vector3::Cross(eV2, cV2).z;

			if (in0 > 0 && in1 > 0 && in2 > 0) {
				const float magnitude = Vector3::Cross(eV0, -eV2).z; //-eV2 = vertices_screen[2].position - vertices_screen[0].position

				const float mag0 = in0 / magnitude;
				const float mag1 = in1 / magnitude;
				const float mag2 = in2 / magnitude;

				finalColor =	(mag0 * vertices_screen[0].color) + 
								(mag1 * vertices_screen[1].color) + 
								(mag2 * vertices_screen[2].color);
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
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
