#pragma once
#include <assimp/scene.h>

#include <engine/mesh.h>

namespace phoenix
{
	class Model
	{
	public:
		std::vector<Mesh*> _meshes;

		Model(const std::string&);

		void render();
		void render(const Shader&);

	private:
		std::string _directory;
		std::vector<Texture> _cache;

		void processNode(const aiScene*, const aiNode*);
		Mesh* processMesh(const aiScene*, const aiMesh*);
		std::vector<Texture> loadTextures(const aiMaterial*, aiTextureType, TextureType);
	};
}