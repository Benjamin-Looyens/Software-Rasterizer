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
#include <iostream>

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

	// Reset Depth buffer
	std::fill(begin(m_DepthBuffer), end(m_DepthBuffer), std::numeric_limits<float>::max());

	// Temporary test mesh
	std::vector<Mesh> meshes{
		{
			// Vertices
			{
				{ { -3.0f,  3.0f, -2.0f }, colors::White },	// 0
				{ {  0.0f,  3.0f, -2.0f }, colors::White },	// 1
				{ {  3.0f,  3.0f, -2.0f }, colors::White },	// 2
				{ { -3.0f,  0.0f, -2.0f }, colors::White },	// 3
				{ {  0.0f,  0.0f, -2.0f }, colors::White },	// 4
				{ {  3.0f,  0.0f, -2.0f }, colors::White },	// 5
				{ { -3.0f, -3.0f, -2.0f }, colors::White },	// 6
				{ {  0.0f, -3.0f, -2.0f }, colors::White },	// 7
				{ {  3.0f, -3.0f, -2.0f }, colors::White },	// 8
			},

			// Indices (List)
			{
				3, 0, 4,
				0, 1, 4,
				4, 1, 5,
				1, 2, 5,
				6, 3, 7,
				3, 4, 7,
				7, 4, 8,
				4, 5, 8,
			},
			
			PrimitiveTopology::TriangleList
			
			//// Indices (Strip)
			//{
			//	3, 0, 4,
			//	1, 5, 2,
			//
			//	2, 6,
			//
			//	6, 3, 7,
			//	4, 8, 5,
			//},
			//
			//PrimitiveTopology::TriangleStrip
		}
	};

	// Convert to Screen Space
	VertexTransformationFunction(meshes[0].vertices, meshes[0].vertices_out);

	//RENDER LOGIC
	for (const Mesh& mesh : meshes) {
		const auto& indices = mesh.indices;
		const auto& vertices = mesh.vertices_out;

		int loopAdd;
		int sizeCheck;
		switch (mesh.primitiveTopology) {
		case PrimitiveTopology::TriangleList:
			loopAdd = 3;
			sizeCheck = 0;
			break;
		case PrimitiveTopology::TriangleStrip:
			loopAdd = 1;
			sizeCheck = 2;
			break;
		}

		for (size_t index{ }; index < indices.size() - sizeCheck; index += loopAdd) {
			const int i0 = indices[index];
			int i1 = indices[index + 1];
			int i2 = indices[index + 2];
		
			if (i0 == i1 || i1 == i2) continue; // Degenerate triangle
			if (index & 1 && sizeCheck) std::swap(i1, i2); // HACK : The && sizeCheck isnt ideal but works
		
			const Vertex_Out& v0 = vertices[i0];
			const Vertex_Out& v1 = vertices[i1];
			const Vertex_Out& v2 = vertices[i2];

			const Vector3 eV0 = v1.position - v0.position;
			const Vector3 eV1 = v2.position - v1.position;
			const Vector3 eV2 = v0.position - v2.position;

			constexpr float TRIANGLE_BOUNDS_OFFSET = 0.5f; // NOTE : Possibly check if needs to be bigger

			const Vector2 min{
				std::clamp(std::min(std::min(v0.position.x, v1.position.x), v2.position.x), 0.0f, static_cast<float>(m_Width))  - TRIANGLE_BOUNDS_OFFSET,
				std::clamp(std::min(std::min(v0.position.y, v1.position.y), v2.position.y), 0.0f, static_cast<float>(m_Height)) - TRIANGLE_BOUNDS_OFFSET
			};
			const Vector2 max{
				std::clamp(std::max(std::max(v0.position.x, v1.position.x), v2.position.x), 0.0f, static_cast<float>(m_Width))  + TRIANGLE_BOUNDS_OFFSET,
				std::clamp(std::max(std::max(v0.position.y, v1.position.y), v2.position.y), 0.0f, static_cast<float>(m_Height)) + TRIANGLE_BOUNDS_OFFSET
			};

			// TODO : Change to uint, there's no need for these to be int
			// HACK : I think having [px < static_cast<int>(max.x)] might cause it to cast for int every loop (which is icky)
			for (int px{ static_cast<int>(min.x) }; px < static_cast<int>(max.x); ++px) {
				for (int py{ static_cast<int>(min.y) }; py < static_cast<int>(max.y); ++py) {
					const int pixelIndex = px + (py * m_Width);

					if (m_RenderBounds) {
						if ((px == static_cast<int>(min.x) || py == static_cast<int>(min.y)) || 
							(px == static_cast<int>(max.x) - 1 || py == static_cast<int>(max.y) - 1)) {
							ColorRGB finalColor = (v0.color + v1.color + v2.color) / 3; // Average
							UpdateColor(pixelIndex, finalColor);
							continue;
						}
					}

					const Vector2 pixel{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };
					const Vector3 pixel3{ pixel.x, pixel.y, 0.0f };

					const Vector3 cV0 = pixel3 - v0.position;
					const float in0 = Vector3::CrossZ(eV0, cV0);

					const Vector3 cV1 = pixel3 - v1.position;
					const float in1 = Vector3::CrossZ(eV1, cV1);

					const Vector3 cV2 = pixel3 - v2.position;
					const float in2 = Vector3::CrossZ(eV2, cV2);

					if (in0 >= 0 && in1 >= 0 && in2 >= 0) {
						const float magnitude = in0 + in1 + in2;

						const float w0 = in1 / magnitude; // For some reason shifting the inside checks over by 1 vertex fixed my clipping issue ????
						const float w1 = in2 / magnitude; 
						const float w2 = in0 / magnitude; 

						const float depth = 
							w0 * v0.position.z +
							w1 * v1.position.z +
							w2 * v2.position.z;
					
						//if (depth < 0.0f) continue;

						if (depth < m_DepthBuffer[pixelIndex]) {
							m_DepthBuffer[pixelIndex] = depth;

							ColorRGB finalColor =	(w0 * v0.color) +
													(w1 * v1.color) +
													(w2 * v2.color);
						
							UpdateColor(pixelIndex, finalColor);
						}
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

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());
	
	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };

	std::transform(begin(vertices_in), end(vertices_in), begin(vertices_out),
		[&](const Vertex& vert) {
			const Vector3 view = m_Camera.viewMatrix.TransformPoint(vert.position);
			
			const Vector3 clip = {	view.x / (view.z * aspectRatio * m_Camera.fov),
									view.y / (view.z * m_Camera.fov),
									view.z
			}; 

			return Vertex_Out{ Vector4{
				((clip.x + 1.0f) / 2.0f) * m_Width,
				((1.0f - clip.y) / 2.0f) * m_Height,
				clip.z,
				1.0f
			}, 
				vert.color
			};
		}
	);
}

void dae::Renderer::UpdateColor(const int& index, ColorRGB& col)
{
	//Update Color in Buffer
	col.MaxToOne();

	m_pBackBufferPixels[index] = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(col.r * 255),
		static_cast<uint8_t>(col.g * 255),
		static_cast<uint8_t>(col.b * 255));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
