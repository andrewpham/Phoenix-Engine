#include <engine/model.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>

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

		processNode(scene, scene->mRootNode);
	}

	void Model::render()
	{
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			_meshes[i].render();
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

	Mesh Model::processMesh(const aiScene* scene, const aiMesh* mesh)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		for (size_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex;

			vertex._position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertex._normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
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

		return Mesh(vertices, indices);
	}
}