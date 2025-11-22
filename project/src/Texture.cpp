#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* pSurface = IMG_Load(path.c_str());

		if (!pSurface) {
			SDL_FreeSurface(pSurface);
			return nullptr;
		}

		return new Texture(pSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		const int& width = m_pSurface->w;
		const int& height = m_pSurface->h;

		const int x = static_cast<int>(static_cast<float>(width) * uv.x);
		const int y = static_cast<int>(static_cast<float>(height) * uv.y);

		Uint8 R;
		Uint8 G;
		Uint8 B;

		SDL_GetRGB(m_pSurfacePixels[x + (y * width)], m_pSurface->format, &R, &G, &B); // Can crash here due to out of bounds access

		return {
			static_cast<float>(R) / 255.0f,
			static_cast<float>(G) / 255.0f,
			static_cast<float>(B) / 255.0f
		};
	}
}