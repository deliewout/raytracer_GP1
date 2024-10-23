#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"


namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{ 90.f };

		Vector3 forward{ Vector3::UnitZ };
		//Vector3 forward{ 0.266f,-0.453f,0.860f };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};

		const float movementSpeed{ 5.0f };
		const float rotationSpeed{ M_PI / 60 };
		Matrix rotationMatrix;

		Matrix CalculateCameraToWorld()
		{
			//todo: W2
			Vector3 worldUp{ 0,1,0 };

			right = Vector3::Cross(worldUp, forward);
			right.Normalized();
			up = Vector3::Cross(forward, right);
			up.Normalized();
			Matrix cameraMatrix{ right,up,forward,origin };
			return cameraMatrix;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();
			
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				//forward += Matrix::CreateTranslation(Vector3{ 0,0,movementSpeed * deltaTime });
				origin += Vector3{ 0,0,movementSpeed * deltaTime };
			}
			else if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= Vector3{ 0,0,movementSpeed * deltaTime };
			}
			else if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= Vector3{ movementSpeed * deltaTime,0,0 };
			}
			else if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += Vector3{ movementSpeed * deltaTime,0,0 };
			}


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				totalPitch += -mouseY * deltaTime * rotationSpeed;
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				totalYaw+= mouseX * deltaTime * rotationSpeed;
			}
			
			rotationMatrix = Matrix::CreateRotation(Vector3{ totalPitch,totalYaw,0 });
			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();
		}
	};
}
