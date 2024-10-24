//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	float aspectRatio{ static_cast<float>(m_Width) / m_Height };
	
	float FOV{ tanf(camera.fovAngle/2) };
	const Matrix cameraToWorld = camera.CalculateCameraToWorld();

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			
			Vector3 rayDirection{ (2 * ((px+0.5f) / m_Width) - 1) * aspectRatio*FOV,(1 - (2 * ((py+0.5f)) / m_Height))*FOV,1 };
			rayDirection.Normalize();
			
			
			//Ray hitRay{ {0,0,0},rayDirection };
			//ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };

			Ray viewRay{ camera.origin,cameraToWorld.TransformVector(rayDirection) };
			ColorRGB finalColor{};
			HitRecord closestHit{};

			pScene->GetClosestHit(viewRay, closestHit);
			closestHit.normal.Normalize();

			if (closestHit.didHit)
			{
				const Vector3 offset{ closestHit.origin + closestHit.normal*0.001f };

				for (const Light& currentLight : lights)
				{
					Vector3 lightDirection{ LightUtils::GetDirectionToLight(currentLight,offset) };
					float maxDistance{ lightDirection.Normalize() };

					if (m_ShadowsEnabled)
					{
						Ray lightRay{ offset,lightDirection,0.0001f,maxDistance };
						if (pScene->DoesHit(lightRay))
						{
							//finalColor *= 0.5f;
							continue;
						}
					}

					const float observedArea{ Vector3::Dot(lightDirection,closestHit.normal)};
					ColorRGB radiance{ LightUtils::GetRadiance(currentLight,closestHit.origin) };

					switch (m_CurrentLightingMode)
					{
					case dae::Renderer::LightingMode::ObservedArea:
						if (observedArea < 0)
							continue;
						finalColor += ColorRGB{ observedArea,observedArea,observedArea };
						break;
					case dae::Renderer::LightingMode::Radiance:
						finalColor += radiance;
						break;
					case dae::Renderer::LightingMode::BRDF:
						finalColor += materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, -rayDirection);
						break;
					case dae::Renderer::LightingMode::Combined:
						if (observedArea < 0)
							continue;
						finalColor += radiance * observedArea * materials[closestHit.materialIndex]->Shade(closestHit,lightDirection,-rayDirection);
						break;
					}

				}
			}

			//ColorRGB finalColor{ gradient,gradient,gradient };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::CycleLightingMode()
{
	int currentLightingMode = static_cast<int>(m_CurrentLightingMode);
	const int maxLightingMode{ static_cast<int>(LightingMode::Combined) + 1 };

	m_CurrentLightingMode = static_cast<LightingMode>(++currentLightingMode % maxLightingMode);

	if (m_CurrentLightingMode == LightingMode::ObservedArea)
	{
		std::cout << "observedarea"<<std::endl;
	}
	if (m_CurrentLightingMode == LightingMode::BRDF)
	{
		std::cout << "BRDF" << std::endl;
	}
	if (m_CurrentLightingMode == LightingMode::Combined)
	{
		std::cout << "Combined" << std::endl;
	}
	if (m_CurrentLightingMode == LightingMode::Radiance)
	{
		std::cout << "Radiance" << std::endl;
	}
}
