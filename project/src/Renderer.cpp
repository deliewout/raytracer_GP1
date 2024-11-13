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
//#include "Matrix.h"
#include <execution>
#include "Vector3.h"

#include <iostream>
#define PARALLEL_EXECUTION

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
	const Matrix cameraToWorld = camera.CalculateCameraToWorld();

	float aspectRatio{ static_cast<float>(m_Width) / m_Height };
	
	const float fovAngle{ camera.fovAngle * TO_RADIANS };
	const float fov{ tanf(camera.fovAngle/2) };
	
#if defined(PARALLEL_EXECUTION)
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
	std::vector<uint32_t> pixelIndices{};
	pixelIndices.reserve(amountOfPixels);
	for (uint32_t index{}; index < amountOfPixels; ++index)
		pixelIndices.emplace_back(index);
	std::for_each(std::execution::par, pixelIndices.begin(), pixelIndices.end(), [&](int i)
		{
			RenderPixel(pScene, i, fov, aspectRatio, cameraToWorld, camera.origin);
		});
	

	
#else
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };

	for (uint32_t pixelIndex{}; pixelIndex < amountOfPixels; pixelIndex++)
	{
		RenderPixel(pScene, pixelIndex, fov, aspectRatio, cameraToWorld, camera.origin);
	}
#endif


	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderPixel(Scene* pScene, int pixelIndex, float fov, float aspectRatio, const Matrix cameraToWorld, const Vector3 cameraOrigin) const
{
	auto materials{ pScene->GetMaterials() };
	auto& lights = pScene->GetLights();
	const int px{ pixelIndex % m_Width }, py{ pixelIndex / m_Width };

	float rx{ px + 0.5f }, ry{ py + 0.5f };
	float cx{ (2 * (rx / float(m_Width)) - 1) * aspectRatio * fov };
	float cy{ (1 - (2 * (ry / float(m_Height)))) * fov };

	Vector3 rayDirection{ (2 * ((px + 0.5f) / m_Width) - 1) * aspectRatio * fov,(1 - (2 * ((py + 0.5f)) / m_Height)) * fov,1 };
	rayDirection.Normalize();

	Ray viewRay{ cameraOrigin,cameraToWorld.TransformVector(rayDirection) };
	ColorRGB finalColor{};
	HitRecord closestHit{};

	pScene->GetClosestHit(viewRay, closestHit);
	closestHit.normal.Normalize();

	if (closestHit.didHit)
	{
		const Vector3 offset{ closestHit.origin + closestHit.normal * 0.001f };

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

			const float observedArea{ Vector3::Dot(lightDirection,closestHit.normal) };
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
				finalColor += radiance * observedArea * materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, -rayDirection);
				break;
			}
		}
	}
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
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
