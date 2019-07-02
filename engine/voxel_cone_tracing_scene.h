#pragma once
#include <glm/glm.hpp>

#include <engine/light.h>
#include <engine/camera.h>
#include <engine/model.h>

namespace phoenix
{
	struct VoxelConeTracingScene
	{
		Camera* _camera;
		std::vector<Mesh*> _meshes;
		PointLight* _pointLight;
		Model* _lightSphere;

		VoxelConeTracingScene();
		~VoxelConeTracingScene();
	};
}