#include <engine/model.h>
#include <engine/utils.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>

namespace phoenix
{
	Model::Model(const std::string& pFile)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(pFile, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cerr << "Assimp error: " << importer.GetErrorString() << "\n";
			return;
		}

		_directory = pFile.substr(0, pFile.find_last_of("/"));

		processNode(scene, scene->mRootNode);
	}

	void Model::render()
	{
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			_meshes[i]->render();
		}
	}

	void Model::render(const Shader& shader)
	{
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			_meshes[i]->render(shader);
		}
	}

	void Model::processNode(const aiScene* scene, const aiNode* node)
	{
		for (size_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			_meshes.push_back(processMesh(scene, mesh));
		}
		// Recurse
		for (size_t i = 0; i < node->mNumChildren; ++i)
		{
			processNode(scene, node->mChildren[i]);
		}
	}

	Mesh* Model::processMesh(const aiScene* scene, const aiMesh* mesh)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		for (size_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex;

			vertex._position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			if (mesh->mNormals)
			{
				vertex._normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}
			else
			{
				vertex._normal = vertex._position;
			}
			if (mesh->mTextureCoords[0])
			{
				// Assuming a single set of texture coordinates is bound to the specific mesh vertex
				vertex._texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}
			else
			{
				vertex._texCoords = glm::vec2(0.0f, 0.0f);
			}
			if (mesh->mTangents)
			{
				vertex._tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			}
			else
			{
				vertex._tangent = glm::vec3(0.0f);
			}
			if (mesh->mBitangents)
			{
				vertex._bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			}
			else
			{
				vertex._bitangent = glm::vec3(0.0f);
			}

			vertices.push_back(vertex);
		}

		for (size_t i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for (size_t j = 0; j < face.mNumIndices; ++j)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<Texture> diffuseTextures = loadTextures(material, aiTextureType_DIFFUSE, DIFFUSE);
		textures.insert(textures.end(), diffuseTextures.begin(), diffuseTextures.end());
		std::vector<Texture> specularTextures = loadTextures(material, aiTextureType_SPECULAR, SPECULAR);
		textures.insert(textures.end(), specularTextures.begin(), specularTextures.end());
		std::vector<Texture> ambientTextures = loadTextures(material, aiTextureType_AMBIENT, AMBIENT);
		textures.insert(textures.end(), ambientTextures.begin(), ambientTextures.end());
		std::vector<Texture> bumpTextures = loadTextures(material, aiTextureType_HEIGHT, HEIGHT);
		textures.insert(textures.end(), bumpTextures.begin(), bumpTextures.end());
		std::vector<Texture> opacityTextures = loadTextures(material, aiTextureType_OPACITY, OPACITY);
		textures.insert(textures.end(), opacityTextures.begin(), opacityTextures.end());

		return new Mesh(vertices, indices, textures);
	}

	std::vector<Texture> Model::loadTextures(const aiMaterial* material, aiTextureType assimpTextureType, TextureType textureType)
	{
		std::vector<Texture> textures;
		for (size_t i = 0; i < material->GetTextureCount(assimpTextureType); ++i)
		{
			aiString path;
			material->GetTexture(assimpTextureType, i, &path);
			bool cached = false;
			for (size_t j = 0; j < _cache.size(); ++j)
			{
				if (!std::strcmp(_cache[j]._key.data(), path.C_Str()))
				{
					textures.emplace_back(_cache[j]);
					cached = true;
					break;
				}
			}
			if (!cached)
			{
				_cache.emplace_back(Texture{ Utils::loadTexture((_directory + "/" + std::string(path.C_Str())).c_str()), textureType, path.C_Str() });
				textures.emplace_back(_cache.back());
			}
		}
		return textures;
	}
}