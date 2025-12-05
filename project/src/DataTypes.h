#pragma once
#include "Maths.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{colors::White};
		Vector2 uv{};
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};
	
	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct TextureData {
		Texture* pDiffuse;
		Texture* pNormal;
		Texture* pSpecular;
		Texture* pGloss;

		void FreeMemory()
		{
			delete pDiffuse;
			delete pNormal;
			delete pSpecular;
			delete pGloss;
		}
	};

	class Transform final
	{
	public:
		Transform() = default;
		Transform(const Vector3& _translation, const Vector3& _rotation, const Vector3& _scale)
			: m_Translation{ Matrix::CreateTranslation(_translation) }, m_Rotation{ Matrix::CreateRotation(_rotation) }, m_Scale{ Matrix::CreateScale(_scale) }
		{}

		void SetTranslation(const Vector3& _translation)
		{
			m_Translation = Matrix::CreateTranslation(_translation);
			m_FullDirty = true;
		}
		void SetRotation(const Vector3& _rotation)
		{
			m_Rotation = Matrix::CreateRotation(_rotation);
			m_FullDirty = true;
		}
		void SetRotationX(const float& pitchAngle)
		{
			m_Rotation = Matrix::CreateRotationX(pitchAngle);
			m_FullDirty = true;
		}
		void SetRotationY(const float& yawAngle)
		{
			m_Rotation = Matrix::CreateRotationY(yawAngle);
			m_FullDirty = true;
		}
		void SetRotationZ(const float& rollAngle)
		{
			m_Rotation = Matrix::CreateRotationZ(rollAngle);
			m_FullDirty = true;
		}
		void SetScale(const Vector3& _scale)
		{
			m_Scale = Matrix::CreateScale(_scale);
			m_FullDirty = true;
		}
		void SetScale(const float& univScale)
		{
			m_Scale = Matrix::CreateScale({ univScale, univScale, univScale });
			m_FullDirty = true;
		}

		inline const Matrix& GetTranslation() const { return m_Translation; }
		inline const Matrix& GetRotation() const { return m_Rotation; }
		inline const Matrix& GetScale() const { return m_Scale; }

		const Matrix& GetFull()
		{
			if (m_FullDirty) {
				m_Full = m_Scale * m_Rotation * m_Translation;
				m_FullDirty = false;
			}

			return m_Full;
		}

	private:
		Matrix m_Translation{ Matrix::CreateIdentity() };
		Matrix m_Rotation{ Matrix::CreateIdentity() };
		Matrix m_Scale{ Matrix::CreateIdentity() };
		Matrix m_Full{ Matrix::CreateIdentity() };

		bool m_FullDirty{};
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };
		TextureData textureData;

		std::vector<Vertex_Out> vertices_out{};
		Transform worldMatrixTransform{};
	};
}
