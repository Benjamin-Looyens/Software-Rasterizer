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
#include <execution>

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
	m_pDepthBuffer = new float[m_Width * m_Height];

	// Initialize Meshes
	m_Meshes.push_back({});
	if (!Utils::ParseOBJ("resources/vehicle.obj", m_Meshes[0].vertices, m_Meshes[0].indices)) {
		assert(false && "Mesh Could not be Initialized!");
	}
	m_Meshes[0].primitiveTopology = PrimitiveTopology::TriangleList;
	m_Meshes[0].textureData = { Texture::LoadFromFile("resources/vehicle_diffuse.png"), 
								Texture::LoadFromFile("resources/vehicle_normal.png"), 
								Texture::LoadFromFile("resources/vehicle_specular.png"),
								Texture::LoadFromFile("resources/vehicle_gloss.png") };
	m_Meshes[0].worldMatrixTransform.SetTranslation(m_MeshPos);

	//Initialize Camera
	m_Camera.Initialize(m_Width, m_Height, 45.0f, m_CameraStartPos);
	m_Camera.farPlane = 0.1f;
	m_Camera.farPlane = 100.0f;
}

Renderer::~Renderer()
{
	delete[] m_pDepthBuffer;

	// HACK : Feels really stupid but it was somehow freeing memory when just attaching these deletes to the destructor
	for (Mesh& mesh : m_Meshes) {
		mesh.textureData.FreeMemory();
	}
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
	if (m_Camera.updatedMatricesThisFrame) m_VerticesDirty = true;

	if (m_DoRotation) {
		m_RotCounter += std::fmod(pTimer->GetElapsed(), PI_2);

		for (Mesh& mesh : m_Meshes) {
			mesh.worldMatrixTransform.SetRotationY(m_RotCounter);
		}
		m_VerticesDirty = true;
	}
}

void Renderer::Render()
{
	//@START
	// Clear BackBuffer
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 75, 75, 75));

	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Reset Depth buffer
	std::fill_n(m_pDepthBuffer, m_Width * m_Height, std::numeric_limits<float>::max());

	// Convert to NDC
	if (m_VerticesDirty) {
		for (Mesh& mesh : m_Meshes) {
			VertexTransformationFunction(mesh.worldMatrixTransform.GetFull(), mesh.vertices, mesh.vertices_out);
		}
		m_VerticesDirty = false;
	}

	//RENDER LOGIC
	for (const Mesh& mesh : m_Meshes) {
		const auto& indices = mesh.indices;
		const auto& vertices = mesh.vertices_out;

		int loopAdd{};
		int sizeCheck{};
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
			
			// TODO : Collapse a lot of checks to functions

			if (!Utils::FrustumCheck(v0.position, v1.position, v2.position)) continue;

			const Vector4 v0Pos = Utils::ApplyResolution(v0.position, m_Width, m_Height);
			const Vector4 v1Pos = Utils::ApplyResolution(v1.position, m_Width, m_Height);
			const Vector4 v2Pos = Utils::ApplyResolution(v2.position, m_Width, m_Height);

			const Vector3 eV0 = v1Pos - v0Pos;
			const Vector3 eV1 = v2Pos - v1Pos;
			const Vector3 eV2 = v0Pos - v2Pos;

			constexpr float TRIANGLE_BOUNDS_OFFSET = 1.5f; // NOTE : Possibly check if needs to be bigger

			const uint32_t minX = static_cast<uint32_t>(std::clamp(std::min(std::min(v0Pos.x, v1Pos.x), v2Pos.x) - TRIANGLE_BOUNDS_OFFSET, 0.0f, static_cast<float>(m_Width)));
			const uint32_t minY = static_cast<uint32_t>(std::clamp(std::min(std::min(v0Pos.y, v1Pos.y), v2Pos.y) - TRIANGLE_BOUNDS_OFFSET, 0.0f, static_cast<float>(m_Height)));
			const uint32_t maxX = static_cast<uint32_t>(std::clamp(std::max(std::max(v0Pos.x, v1Pos.x), v2Pos.x) + TRIANGLE_BOUNDS_OFFSET, 0.0f, static_cast<float>(m_Width)));
			const uint32_t maxY = static_cast<uint32_t>(std::clamp(std::max(std::max(v0Pos.y, v1Pos.y), v2Pos.y) + TRIANGLE_BOUNDS_OFFSET, 0.0f, static_cast<float>(m_Height)));

			for (uint32_t px{ minX }; px < maxX; ++px) {
				for (uint32_t py{ minY }; py < maxY; ++py) {
					const int pixelIndex = px + (py * m_Width);

					if (m_DoBoundsRender) {
						if ((px == minX || py == minY) || 
							(px == maxX - 1 || py == maxY - 1)) {
							ColorRGB finalColor = (v0.color + v1.color + v2.color) / 3; // Average
							UpdateColor(pixelIndex, finalColor);
							continue;
						}
					}

					const Vector3 pixel3{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f, 0.0f };

					const Vector3 cV0 = pixel3 - v0Pos;
					const float in0 = Vector3::CrossZ(eV0, cV0);

					const Vector3 cV1 = pixel3 - v1Pos;
					const float in1 = Vector3::CrossZ(eV1, cV1);

					const Vector3 cV2 = pixel3 - v2Pos;
					const float in2 = Vector3::CrossZ(eV2, cV2);

					if (in0 >= 0 && in1 >= 0 && in2 >= 0) {
						const float magnitude = in0 + in1 + in2;

						const float w0 = in1 / magnitude;
						const float w1 = in2 / magnitude; 
						const float w2 = in0 / magnitude; 

						const float zDepth = 1.0f / (
							w0 * (1 / v0Pos.z) +
							w1 * (1 / v1Pos.z) +
							w2 * (1 / v2Pos.z));

						// TODO : Potentially collapse the following "Stage" to a different function (Pixel sampling stage)
						if (zDepth < m_pDepthBuffer[pixelIndex]) {
							m_pDepthBuffer[pixelIndex] = zDepth;

							ColorRGB finalColor{ Utils::Remap(zDepth, 0.9f, 1.0f, 0.0f, 1.0f) };
							if (m_DoDepthRender) {
								UpdateColor(pixelIndex, finalColor);
								continue;
							}

							const float wDepth = 1 / (
								w0 * (1 / v0Pos.w) +
								w1 * (1 / v1Pos.w) +
								w2 * (1 / v2Pos.w));

							Vertex_Out interpolated{
								// Position
								{ 	
									1 / (
										w0 * (1 / v0Pos.x) +
										w1 * (1 / v1Pos.x) +
										w2 * (1 / v2Pos.x)),
									1 / (
										w0 * (1 / v0Pos.y) +
										w1 * (1 / v1Pos.y) +
										w2 * (1 / v2Pos.y)),
									zDepth, wDepth 
								},

								// Color
								(((v0.color / v0Pos.w) * w0) +
								((v1.color / v1Pos.w)  * w1) +
								((v2.color / v2Pos.w)  * w2))
								* wDepth,

								// UV
								(((v0.uv / v0Pos.w) * w0) +
								((v1.uv / v1Pos.w)  * w1) +
								((v2.uv / v2Pos.w)  * w2))
								* wDepth,

								// Normal
								((v0.normal * w0) +
								(v1.normal  * w1) +
								(v2.normal  * w2)).Normalized(),

								// Tangent
								((v0.tangent * w0) +
								(v1.tangent  * w1) +
								(v2.tangent  * w2)).Normalized(),

								// ViewDirection
								((((v0.viewDirection / v0Pos.w) * w0) +
								((v1.viewDirection / v1Pos.w)  * w1) +
								((v2.viewDirection / v2Pos.w)  * w2)) * wDepth).Normalized()
							};
							
							constexpr float LIGHT_VALUE = 0.577f;
							const Vector3 lightDir{ LIGHT_VALUE, -LIGHT_VALUE, LIGHT_VALUE };
							
							if (Utils::PixelShade(interpolated, mesh.textureData, m_CurMode, -lightDir.Normalized(), m_UseNormalMap, finalColor)) {
								UpdateColor(pixelIndex, finalColor);
							}
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

void Renderer::VertexTransformationFunction(const Matrix& worldMat, const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_OUT) const
{
	vertices_OUT.resize(vertices_in.size());
	
	const float& aspectRatio = m_Camera.aspectRatio;
	const Matrix worldViewProjectionMatrix = worldMat * m_Camera.viewMatrix * m_Camera.projectMatrix;

	std::transform(std::execution::par, begin(vertices_in), end(vertices_in), begin(vertices_OUT),
		[&](const Vertex& vert) {
			const Vector3 worldPos = worldMat.TransformPoint(vert.position);
			const Vector4 view = worldViewProjectionMatrix.TransformPoint({ vert.position, 1.0f });
			
			const Vector4 clip = {	view.x / view.w,
									view.y / view.w,
									view.z / view.w,
									view.w
			}; 

			return Vertex_Out{ clip, vert.color, vert.uv, 
				
				worldMat.TransformVector(vert.normal),
				worldMat.TransformVector(vert.tangent),
				(worldPos - m_Camera.origin).Normalized()
			};
		}
	);
}

void dae::Renderer::ToggleBoundsRender()
{
	m_DoBoundsRender = !m_DoBoundsRender;

	std::cout << "BoundingBox Rendering = ";
	switch (m_DoBoundsRender)
	{
	case true:
		std::cout << "ON\n";
		break;
	case false:
		std::cout << "OFF\n";
		break;
	}
}

void dae::Renderer::ToggleDepthRender()
{
	m_DoDepthRender = !m_DoDepthRender;

	std::cout << "Depth Buffer Rendering = ";
	switch (m_DoDepthRender)
	{
	case true:
		std::cout << "ON\n";
		break;
	case false:
		std::cout << "OFF\n";
		break;
	}
}

void dae::Renderer::ToggleRotation()
{
	m_DoRotation = !m_DoRotation;

	std::cout << "Mesh Rotation = ";
	switch (m_DoRotation)
	{
	case true:
		std::cout << "ON\n";
		break;
	case false:
		std::cout << "OFF\n";
		break;
	}
}

void dae::Renderer::ToggleNormalMap()
{
	m_UseNormalMap = !m_UseNormalMap;

	std::cout << "Normal Map Sampling = ";
	switch (m_UseNormalMap)
	{
	case true:
		std::cout << "ON\n";
		break;
	case false:
		std::cout << "OFF\n";
		break;
	}
}

void dae::Renderer::CycleLightingMode()
{
	// Yuck
	m_CurMode = static_cast<LightingMode>((static_cast<uint8_t>(m_CurMode) + 1) % static_cast<uint8_t>(LightingMode::ModeAmount)); 

	// Yuck part 2
	std::cout << "Rendering Mode = ";
	switch (m_CurMode)
	{
	case Renderer::LightingMode::ObservedArea:
		std::cout << "OBSERVED AREA\n";
		break;
	case Renderer::LightingMode::Diffuse:
		std::cout << "LAMBERT DIFFUSE\n";
		break;
	case Renderer::LightingMode::Specular:
		std::cout << "PHONG SPECULAR\n";
		break;
	case Renderer::LightingMode::Combined:
		std::cout << "COMBINED\n";
		break;
	}
}

void dae::Renderer::ResetCamera()
{
	m_Camera.SetOrigin(m_CameraStartPos);
	m_Camera.ResetOrientation();
}

void dae::Renderer::UpdateColor(const uint32_t& index, ColorRGB& col)
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
