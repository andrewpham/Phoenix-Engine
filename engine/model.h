#pragma once
#include <assimp/scene.h>

#include <engine/mesh.h>

#include <string>

namespace phoenix
{
	class Model
	{
	public:
		std::vector<Mesh> _meshes;

		Model(const std::string&);

		void render();

	private:
		void processNode(const aiScene*, const aiNode*);
		Mesh processMesh(const aiScene*, const aiMesh*);
	};
}