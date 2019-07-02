#include <engine/voxel_cone_tracing_scene.h>

namespace phoenix
{
	VoxelConeTracingScene::VoxelConeTracingScene()
	{
		Model* cornellBox = new Model("../Resources/Objects/cornell_box/cornell.obj");
		for (auto& mesh : cornellBox->_meshes)
		{
			_meshes.emplace_back(mesh);
		}
		_meshes[0]->_material = Material::red();
		_meshes[1]->_material = Material::white();
		_meshes[2]->_material = Material::white();
		_meshes[3]->_material = Material::blue();
		_meshes[4]->_material = Material::white();
		_meshes[5]->_material = Material::white();
		_meshes[6]->_material = Material::white();

		_lightSphere = new Model("../Resources/Objects/sphere.obj");
		_lightSphere->_meshes.back()->_scale = glm::vec3(0.05f);
		_lightSphere->_meshes.back()->_material = Material::defaultMaterial();
		_lightSphere->_meshes.back()->_material->_diffuseColor = glm::vec3(1.0f);
		_lightSphere->_meshes.back()->_material->_emissivity = 0.5f;
		_lightSphere->_meshes.back()->_material->_specularReflectivity = 0.0f;
		_lightSphere->_meshes.back()->_material->_diffuseReflectivity = 0.0f;
		_meshes.emplace_back(_lightSphere->_meshes.back());

		_pointLight = new PointLight();
		_pointLight->_position = _lightSphere->_meshes.back()->_translation;
		_pointLight->_color = _lightSphere->_meshes.back()->_material->_diffuseColor;

		Model* suzanne = new Model("../Resources/Objects/cornell_box/suzanne.obj");
		Mesh* suzanneMesh = suzanne->_meshes[0];
		suzanneMesh->_translation = glm::vec3(0.07f, -0.5f, 0.36f);
		suzanneMesh->_rotation = glm::radians(45.0f);
		suzanneMesh->_scale = glm::vec3(0.25f);
		suzanneMesh->_material = Material::defaultMaterial();
		suzanneMesh->_material->_specularColor = glm::vec3(0.8f, 0.8f, 1.0f);
		suzanneMesh->_material->_diffuseColor = suzanneMesh->_material->_specularColor;
		suzanneMesh->_material->_specularReflectivity = 0.8f;
		suzanneMesh->_material->_aperture = 0.21f;
		_meshes.emplace_back(suzanneMesh);

		Model* buddha = new Model("../Resources/Objects/cornell_box/buddha.obj");
		Mesh* buddhaMesh = buddha->_meshes[0];
		buddhaMesh->_translation = glm::vec3(-0.6f, 0.0f, 0.5f);
		buddhaMesh->_rotation = glm::radians(135.0f);
		buddhaMesh->_scale = glm::vec3(1.3f);
		buddhaMesh->_material = Material::defaultMaterial();
		buddhaMesh->_material->_specularColor = glm::vec3(0.0f, 0.66f, 0.42f);
		buddhaMesh->_material->_diffuseColor = buddhaMesh->_material->_specularColor;
		_meshes.emplace_back(buddhaMesh);

		_camera = new Camera();
	}

	VoxelConeTracingScene::~VoxelConeTracingScene()
	{
		if (_camera)
		{
			delete _camera;
		}
		for (auto& mesh : _meshes)
		{
			if (mesh)
			{
				delete mesh;
			}
		}
		if (_pointLight)
		{
			delete _pointLight;
		}
		if (_lightSphere)
		{
			delete _lightSphere;
		}
	}
}