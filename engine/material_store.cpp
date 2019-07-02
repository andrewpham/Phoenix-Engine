#include <engine/material_store.h>
#include <iostream>

namespace phoenix
{
	MaterialStore::MaterialStore()
	{
		_materials.emplace("voxelize", new Shader("../Resources/Shaders/voxel_cone_tracing/voxelize.vs", "../Resources/Shaders/voxel_cone_tracing/voxelize.gs", "../Resources/Shaders/voxel_cone_tracing/voxelize.fs"));
		_materials.emplace("world_position_output", new Shader("../Resources/Shaders/voxel_cone_tracing/world_position_output.vs", "../Resources/Shaders/voxel_cone_tracing/world_position_output.fs"));
		_materials.emplace("visualize_voxels", new Shader("../Resources/Shaders/voxel_cone_tracing/visualize_voxels.vs", "../Resources/Shaders/voxel_cone_tracing/visualize_voxels.fs"));
		_materials.emplace("render", new Shader("../Resources/Shaders/voxel_cone_tracing/render.vs", "../Resources/Shaders/voxel_cone_tracing/render.fs"));
	}

	MaterialStore& MaterialStore::getInstance()
	{
		static MaterialStore instance;
		return instance;
	}

	Shader* MaterialStore::getMaterial(const std::string name)
	{
		if (_materials.find(name) != _materials.cend())
		{
			return _materials[name];
		}
		std::cerr << "Could not find material with the name " << name << "!\n";
		return nullptr;
	}

	MaterialStore::~MaterialStore()
	{
		for (auto it = _materials.begin(); it != _materials.end(); ++it)
		{
			delete it->second;
		}
	}
}