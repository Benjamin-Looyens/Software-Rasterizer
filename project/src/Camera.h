#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera final
	{
		Camera() = default;
		Camera(const Vector3& _origin, float _fovAngle, int _width, int _height):
			origin{_origin},
			fovAngle{_fovAngle},
			aspectRatio{ static_cast<float>(_width) / static_cast<float>(_height) }
		{}

		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		float aspectRatio{};

		float movSpeed{ 10.0f };
		float rotSpeed{ 0.6f };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		bool viewDirty = true;

		Matrix projectMatrix{};
		bool projectDirty = true;

		float nearPlane{ 1.0f };
		float farPlane{ 1000.0f };

		void Initialize(int _width, int _height, float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);
		}

		void SetOrigin(const Vector3& newPos) {
			origin = newPos;
			viewDirty = true;
		}

		void ResetOrientation()
		{
			forward = Vector3::UnitZ;
			up = Vector3::UnitY;
			right = Vector3::UnitX;

			totalYaw = 0.0f;
			totalPitch = 0.0f;

			viewDirty = true;
			projectDirty = true;
		}

		void CalculateViewMatrix()
		{
			if (viewDirty) {
				invViewMatrix = CalculateCameraToWorld();
				viewMatrix = Matrix::Inverse(invViewMatrix);
				viewDirty = false;
			}
		}

		void CalculateProjectionMatrix()
		{
			if (projectDirty) {
				const float farToNear = farPlane - nearPlane;
				const float a = farPlane / farToNear;
				const float b = -(farPlane * nearPlane) / farToNear;

				projectMatrix = {
					{ 1 / (aspectRatio * fov), 0, 0, 0},
					{ 0, 1 / fov, 0, 0},
					{ 0, 0, a, 1 }, // Left handed (so is {0, 0, a, 1} not {0, 0, a, -1}
					{ 0, 0, b, 0 }
				};

				projectDirty = false;
			}
		}

		Matrix CalculateCameraToWorld()
		{
			// Leftover from Raytracer
			// If I genuinely need to use this, apply the dirty pattern again
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			return { right, up, forward, origin };
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			Vector3 movement{};
			Vector3 rotation{};

			bool movDirty{};
			bool rotDirty{};
			float sprintModifier{ 1.0f };

			// Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP]) { movement += forward; movDirty = true; }
			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN]) { movement -= forward; movDirty = true; }
			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT]) { movement += right; movDirty = true; }
			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT]) { movement -= right; movDirty = true; }

			if (pKeyboardState[SDL_SCANCODE_SPACE] || pKeyboardState[SDL_SCANCODE_E]) { movement += Vector3::UnitY; movDirty = true; }
			if (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_Q]) { movement -= Vector3::UnitY; movDirty = true; }
			if (pKeyboardState[SDL_SCANCODE_LCTRL]) sprintModifier = 2.0f;

			// Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			const bool rDown = mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT);
			const bool lDown = mouseState & SDL_BUTTON(SDL_BUTTON_LEFT);

			if (rDown && lDown) {
				movement.y -= mouseY;
				movement.x += mouseX;
				movDirty = true;
			}
			else if (rDown) {
				rotation.y += mouseX;
				rotation.x -= mouseY;
				rotDirty = true;
			}
			else if (lDown) {
				movement -= forward * static_cast<float>(mouseY);
				rotation.y += mouseX;

				movDirty = true;
				rotDirty = true;
			}

			if (movDirty && movement.Normalize() > 0.0f) { // NOTE : Ideally, the mouse movement shouldn't be normalized but making a bool just for that seems messy
				origin += movement * movSpeed * deltaTime * sprintModifier;
				viewDirty = true;
			}
			if (rotDirty && rotation.Magnitude() > 0.0f) { // NOTE : In theory I could do SqrMagnitude?
				const Vector3 scaledRotation = rotation * rotSpeed * deltaTime;

				constexpr float OFFSET = 0.05f;
				totalYaw += scaledRotation.x;
				totalYaw = std::clamp(totalYaw, -PI * 0.5f + OFFSET, PI * 0.5f - OFFSET);

				// Ensure always positive between 0 -> 6.28...
				totalPitch = std::fmodf(std::fmodf(scaledRotation.y + totalPitch, PI_2) + PI_2, PI_2);

				const Matrix fullRotMatrix = Matrix::CreateRotationX(totalYaw) * Matrix::CreateRotationY(totalPitch);
				forward = fullRotMatrix.TransformVector(Vector3::UnitZ);
				viewDirty = true;
			}

			//Update Matrices
			if (viewDirty) CalculateViewMatrix();
			if (projectDirty) CalculateProjectionMatrix();	// TODO : Figure out when to mark projectDirty
															//		  comment from starter files: Try to optimize this (CalculateProjectionMatrix),
															//									  should only be called once or when fov/aspectRatio changes
		}
	};
}
