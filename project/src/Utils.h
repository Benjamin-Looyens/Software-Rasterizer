#pragma once
#include <cassert>
#include <fstream>
#include "Maths.h"
#include "DataTypes.h"


namespace dae
{
	namespace Utils
	{
		// Frustum Check for each Vertex
		bool FrustumCheck(const Vector4& v0, const Vector4& v1, const Vector4& v2)
		{
			// Frustum Culling Z
			if (v0.z <= 0.0f || v0.z >= 1.0f) return false;
			if (v1.z <= 0.0f || v1.z >= 1.0f) return false;
			if (v2.z <= 0.0f || v2.z >= 1.0f) return false;

			// Frustum culling X
			if (v0.x <= -1.0f || v0.x >= 1.0f) return false;
			if (v1.x <= -1.0f || v1.x >= 1.0f) return false;
			if (v2.x <= -1.0f || v2.x >= 1.0f) return false;

			// Frustum culling Y
			if (v0.y <= -1.0f || v0.y >= 1.0f) return false;
			if (v1.y <= -1.0f || v1.y >= 1.0f) return false;
			if (v2.y <= -1.0f || v2.y >= 1.0f) return false;

			return true;
		}

		// Apply Resolution onto NDC Coordinates
		inline Vector4 ApplyResolution(const Vector4& ndc, const int width, const int height)
		{
			return { ((ndc.x + 1.0f) / 2.0f) * width,
					 ((1.0f - ndc.y) / 2.0f) * height,
					 ndc.z, ndc.w
			};
		}

		// Function for Shading a Pixel
		// Will return false if "wrong" data was inputted or if out of bounds
		// Stores RGB data for pixel within color_OUT
		bool PixelShade(const Vertex_Out& vert, const TextureData& meshTexture, Renderer::LightingMode mode, const Vector3& lightDir, bool useNormalMap, ColorRGB& color_OUT)
		{
			// Shininess = 25.0f
			// Ambient = { 0.25f, 0.25f, 0.25f }

			// TODO : Check if the uv check is needed anymore after frustum culling
			if ((vert.uv.x <= 1.0f && vert.uv.x >= 0.0f) && (vert.uv.y <= 1.0f && vert.uv.y >= 0.0f)) {
				constexpr float lightIntensity = 7.0f; // TODO : Prob implement a Light Type that stores all the info for lights (direction, intensity, ambient)

				const Vector3 binormal = Vector3::Cross(vert.normal, vert.tangent);
				const Matrix tangentSpace = { vert.tangent, binormal, vert.normal, Vector3::Zero };

				Vector3 normalMapSample = { meshTexture.pNormal->Sample(vert.uv) };
				normalMapSample = 2.0f * normalMapSample - Vector3{ 1.0f, 1.0f, 1.0f };
				normalMapSample = tangentSpace.TransformVector(normalMapSample).Normalized();

				const Vector3& normal = useNormalMap ? normalMapSample : vert.normal;

				// Observed Area Calc
				const float observedArea = std::max(0.0f, Vector3::Dot(normal, lightDir));

				// Lambert calc
				constexpr float kd = 1.0f;
				const ColorRGB diffuse = meshTexture.pDiffuse->Sample(vert.uv);
				const ColorRGB lambert = ((kd / PI) * diffuse);

				// Specular (Phong) Calc
				constexpr float shininess = 25.0f;

				const ColorRGB specularColor = meshTexture.pSpecular->Sample(vert.uv); // NOTE : I can get rid of storing these (as they are not required later)
				const ColorRGB gloss = meshTexture.pGloss->Sample(vert.uv);			   // NOTE : I can get rid of storing these (as they are not required later)
				const float& ks = specularColor.r;
				const float& exp = gloss.r;

				const Vector3 reflect = lightDir - (2.0f * (Vector3::Dot(lightDir, normal) * normal));
				const float cosAngle = std::max(Vector3::Dot(reflect.Normalized(), vert.viewDirection), 0.0f);
				const float specular = ks * std::powf(cosAngle, exp * shininess);

				switch (mode)
				{
					case Renderer::LightingMode::ObservedArea:
						color_OUT = { observedArea };
					break;
					case Renderer::LightingMode::Diffuse:
						color_OUT = lightIntensity *
									lambert *
									observedArea;
						break;
					case Renderer::LightingMode::Specular:
						color_OUT = { specular };
						break;
					case Renderer::LightingMode::Combined:
						color_OUT = ((lightIntensity * lambert) +
									specular) *
									observedArea;


					break;
				}

				return true;
			}

			return false;
		}

		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, bool flipAxisAndWinding = true)
		{
#ifdef DISABLE_OBJ

			//TODO: Enable the code below after uncommenting all the vertex attributes of DataTypes::Vertex
			// >> Comment/Remove '#define DISABLE_OBJ'
			assert(false && "OBJ PARSER not enabled! Check the comments in Utils::ParseOBJ");

#else

			std::ifstream file(filename);
			if (!file)
				return false;

			std::vector<Vector3> positions{};
			std::vector<Vector3> normals{};
			std::vector<Vector2> UVs{};

			vertices.clear();
			indices.clear();

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;

					positions.emplace_back(x, y, z);
				}
				else if (sCommand == "vt")
				{
					// Vertex TexCoord
					float u, v;
					file >> u >> v;
					UVs.emplace_back(u, 1 - v);
				}
				else if (sCommand == "vn")
				{
					// Vertex Normal
					float x, y, z;
					file >> x >> y >> z;

					normals.emplace_back(x, y, z);
				}
				else if (sCommand == "f")
				{
					//if a face is read:
					//construct the 3 vertices, add them to the vertex array
					//add three indices to the index array
					//add the material index as attibute to the attribute array
					//
					// Faces or triangles
					Vertex vertex{};
					size_t iPosition, iTexCoord, iNormal;

					uint32_t tempIndices[3];
					for (size_t iFace = 0; iFace < 3; iFace++)
					{
						// OBJ format uses 1-based arrays
						file >> iPosition;
						vertex.position = positions[iPosition - 1];

						if ('/' == file.peek())//is next in buffer ==  '/' ?
						{
							file.ignore();//read and ignore one element ('/')

							if ('/' != file.peek())
							{
								// Optional texture coordinate
								file >> iTexCoord;
								vertex.uv = UVs[iTexCoord - 1];
							}

							if ('/' == file.peek())
							{
								file.ignore();

								// Optional vertex normal
								file >> iNormal;
								vertex.normal = normals[iNormal - 1];
							}
						}

						vertices.push_back(vertex);
						tempIndices[iFace] = uint32_t(vertices.size()) - 1;
						//indices.push_back(uint32_t(vertices.size()) - 1);
					}

					indices.push_back(tempIndices[0]);
					if (flipAxisAndWinding) 
					{
						indices.push_back(tempIndices[2]);
						indices.push_back(tempIndices[1]);
					}
					else
					{
						indices.push_back(tempIndices[1]);
						indices.push_back(tempIndices[2]);
					}
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');
			}

			//Cheap Tangent Calculations
			for (uint32_t i = 0; i < indices.size(); i += 3)
			{
				uint32_t index0 = indices[i];
				uint32_t index1 = indices[size_t(i) + 1];
				uint32_t index2 = indices[size_t(i) + 2];

				const Vector3& p0 = vertices[index0].position;
				const Vector3& p1 = vertices[index1].position;
				const Vector3& p2 = vertices[index2].position;
				const Vector2& uv0 = vertices[index0].uv;
				const Vector2& uv1 = vertices[index1].uv;
				const Vector2& uv2 = vertices[index2].uv;

				const Vector3 edge0 = p1 - p0;
				const Vector3 edge1 = p2 - p0;
				const Vector2 diffX = Vector2(uv1.x - uv0.x, uv2.x - uv0.x);
				const Vector2 diffY = Vector2(uv1.y - uv0.y, uv2.y - uv0.y);
				float r = 1.f / Vector2::Cross(diffX, diffY);

				Vector3 tangent = (edge0 * diffY.y - edge1 * diffY.x) * r;
				vertices[index0].tangent += tangent;
				vertices[index1].tangent += tangent;
				vertices[index2].tangent += tangent;
			}

			//Fix the tangents per vertex now because we accumulated
			for (auto& v : vertices)
			{
				v.tangent = Vector3::Reject(v.tangent, v.normal).Normalized();

				if(flipAxisAndWinding)
				{
					v.position.z *= -1.f;
					v.normal.z *= -1.f;
					v.tangent.z *= -1.f;
				}

			}

			return true;
#endif
		}
#pragma warning(pop)
	}
}