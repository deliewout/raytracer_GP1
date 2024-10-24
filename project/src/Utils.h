#pragma once
#include <fstream>
#include "Maths.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//uses the analytical solution
			Vector3 sphereToOrigin{ ray.origin - sphere.origin };
			float A = Vector3::Dot(ray.direction, ray.direction);
			float B = Vector3::Dot((2 * ray.direction), sphereToOrigin);
			float C = Vector3::Dot(sphereToOrigin, sphereToOrigin) - Square(sphere.radius);
			float D = Square(B) - (4 * A * C);

			//check only if the discriminant is bigger then zero(full intersection), we dont care if the ray just scratches the sphere
			//because of edge cases
			if (D > 0)
			{
				float t = (-B - sqrtf(D)) / (2 * A);
				//we only will use (-B + sqrtf(D)) / (2 * A) if t
				if (t < ray.min)
				{
					t = (-B + sqrtf(D)) / (2 * A);

				}
				//if t bigger then the min length of the ray and smaller then the max length of the ray
				//check if we want to store information in the hitrecord and return
				if (t > ray.min && t < ray.max)
				{
					if (!ignoreHitRecord)
					{
						hitRecord.t = t;
						hitRecord.didHit = true;
						hitRecord.materialIndex = sphere.materialIndex;
						hitRecord.origin = ray.origin +  ray.direction* hitRecord.t;
						hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
					}	
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float t = Vector3::Dot((plane.origin - ray.origin), plane.normal) / Vector3::Dot(ray.direction, plane.normal);

			if (t > ray.min && t < ray.max)
			{
				if (t < hitRecord.t)
				{
					if (!ignoreHitRecord)
					{
						hitRecord.t = t;
						hitRecord.origin = ray.origin + hitRecord.t * ray.direction;
						hitRecord.didHit = true;
						hitRecord.materialIndex = plane.materialIndex;
						hitRecord.normal = plane.normal;
					}
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			//throw std::runtime_error("Not Implemented Yet");
			float trianglDot{ Vector3::Dot(triangle.normal,ray.direction) };
			TriangleCullMode cullingMode{ triangle.cullMode };

			if( trianglDot == 0 )
				return false;
			if (trianglDot > 0)
				cullingMode = TriangleCullMode::BackFaceCulling;
			if (trianglDot < 0)
				cullingMode = TriangleCullMode::FrontFaceCulling;

				Vector3 planeOrigin{ (triangle.v0 + triangle.v1 + triangle.v2) / 3.f };
				Vector3 L{ planeOrigin - ray.origin };
				float t{ Vector3::Dot(L,triangle.normal) / Vector3::Dot(ray.direction,triangle.normal) };

				if (t < ray.min || t > ray.max)
					return false;

				Vector3 P{ ray.origin + ray.direction * t };
				Vector3 e{ triangle.v1 - triangle.v0 };
				Vector3 p{ P - triangle.v0 };
				if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
					return false;

				e = triangle.v2 - triangle.v1;
				p = P - triangle.v1;
				if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
					return false;

				e = triangle.v0 - triangle.v2;
				p = P - triangle.v2;
				if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
					return false;
				if (!ignoreHitRecord)
				{
					hitRecord.t = t;
					hitRecord.origin = ray.origin + hitRecord.t * ray.direction;
					hitRecord.didHit = true;
					hitRecord.materialIndex = triangle.materialIndex;
					hitRecord.normal = triangle.normal;
				}
				return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			//throw std::runtime_error("Not Implemented Yet");
			return false;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			//todo W3
			switch (light.type)
			{
			case LightType::Point:
				return light.origin - origin;
				break;
			case LightType::Directional:
				return -light.direction;
				break;
			}
			return {};
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			//todo W3
			//throw std::runtime_error("Not Implemented Yet");
			switch (light.type)
			{
			case LightType::Point:
				return{ light.color * (light.intensity / (light.origin - target).SqrMagnitude()) };
				break;
			case LightType::Directional:
				return light.color * light.intensity;
				break;
			}
			return {};
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

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
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}