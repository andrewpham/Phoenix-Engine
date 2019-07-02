#include <engine/renderer.h>
#include <engine/material_store.h>
#include <engine/strings.h>
#include <engine/common.h>
#include <engine/utils.h>
#include <glm/gtc/matrix_transform.hpp>

namespace phoenix
{
	Renderer::Renderer()
	{
		glEnable(GL_MULTISAMPLE);
		_renderShader = MaterialStore::getInstance().getMaterial("render");
		initVoxelization();
		initVoxelVisualization();
	}

	void Renderer::render(VoxelConeTracingScene* scene, RenderMode renderMode)
	{
		voxelize(scene);

		switch (renderMode)
		{
		case RenderMode::VOXEL:
			renderVoxelVisualization(scene);
			break;
		case RenderMode::DEFAULT:
			renderScene(scene);
			break;
		}
	}

	void Renderer::renderScene(VoxelConeTracingScene* scene)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		_renderShader->use();

		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		setCameraUniforms(*_renderShader, scene->_camera);
		scene->_pointLight->setUniforms(*_renderShader);

		renderMeshes(scene->_meshes, *_renderShader);
	}

	void Renderer::initVoxelization()
	{
		_voxelizeShader = MaterialStore::getInstance().getMaterial("voxelize");
		const std::vector<float> data(4 * _voxelTextureRes * _voxelTextureRes * _voxelTextureRes, 0.0f);
		_voxelTexture = new Texture3D(data, _voxelTextureRes, _voxelTextureRes, _voxelTextureRes, true);
	}

	void Renderer::voxelize(VoxelConeTracingScene* scene)
	{
		std::array<float, 4> clearColor{ { 0.0f, 0.0f, 0.0f, 0.0f } };
		_voxelTexture->clear(clearColor);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		_voxelizeShader->use();

		glViewport(0, 0, _voxelTextureRes, _voxelTextureRes);

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		_voxelTexture->bind(*_voxelizeShader, phoenix::G_TEXTURE_3D, 0);
		glBindImageTexture(0, _voxelTexture->_textureID, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

		scene->_pointLight->setUniforms(*_voxelizeShader);

		renderMeshes(scene->_meshes, *_voxelizeShader);
		glGenerateMipmap(GL_TEXTURE_3D);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	void Renderer::initVoxelVisualization()
	{
		_worldPositionOutputShader = MaterialStore::getInstance().getMaterial("world_position_output");
		_visualizeVoxelsShader = MaterialStore::getInstance().getMaterial("visualize_voxels");
		_backfaceBuffer = new Framebuffer(SCREEN_HEIGHT, SCREEN_WIDTH);
		_frontfaceBuffer = new Framebuffer(SCREEN_HEIGHT, SCREEN_WIDTH);
		_cubeModel = new Model("../Resources/Objects/cube.obj");
		_quadMesh = Utils::createQuad();
	}

	void Renderer::renderVoxelVisualization(VoxelConeTracingScene* scene)
	{
		// Render a cube that will have each pixel store the world positions of its front
		// and back face. This cube is representative of our 3D voxel texture, and we will use
		// our positions to calculate corresponding pixel colors using ray marching.
		_worldPositionOutputShader->use();
		setCameraUniforms(*_worldPositionOutputShader, scene->_camera);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		glCullFace(GL_FRONT);
		glBindFramebuffer(GL_FRAMEBUFFER, _backfaceBuffer->_FBO);
		glViewport(0, 0, _backfaceBuffer->_width, _backfaceBuffer->_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::translate(world, _cubeModel->_meshes.back()->_translation);
		world = glm::rotate(world, _cubeModel->_meshes.back()->_rotation, _cubeModel->_meshes.back()->_rotationAxis);
		world = glm::scale(world, _cubeModel->_meshes.back()->_scale);
		_worldPositionOutputShader->setMat4(G_WORLD_MATRIX, world);
		_cubeModel->render();

		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, _frontfaceBuffer->_FBO);
		glViewport(0, 0, _frontfaceBuffer->_width, _frontfaceBuffer->_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_cubeModel->render();

		// Splat the 3D texture onto our screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		_visualizeVoxelsShader->use();
		setCameraUniforms(*_visualizeVoxelsShader, scene->_camera);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		// Ray march from the front to the back face world positions and accumulate/sample colors
		// from our voxel texture along the way to get an accurate picture/projection of the voxelized
		// scene in 2D
		_backfaceBuffer->bindTexture(*_visualizeVoxelsShader, "gBackfaceTexture", 0);
		_frontfaceBuffer->bindTexture(*_visualizeVoxelsShader, "gFrontfaceTexture", 1);
		_voxelTexture->bind(*_visualizeVoxelsShader, phoenix::G_TEXTURE_3D, 2);

		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_quadMesh->render();
	}

	Renderer::~Renderer()
	{
		if (_voxelTexture)
		{
			delete _voxelTexture;
		}
		if (_backfaceBuffer)
		{
			delete _backfaceBuffer;
		}
		if (_frontfaceBuffer)
		{
			delete _frontfaceBuffer;
		}
		if (_cubeModel)
		{
			delete _cubeModel;
		}
		if (_quadMesh)
		{
			delete _quadMesh;
		}
	}

	void Renderer::setCameraUniforms(const Shader& shader, Camera* camera)
	{
		shader.setMat4(G_VP, glm::perspective(glm::radians(camera->_FOV), static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT, PERSPECTIVE_NEAR_PLANE, PERSPECTIVE_FAR_PLANE) * camera->getViewMatrix());
		shader.setVec3(G_VIEW_POS, camera->_position);
	}

	void Renderer::renderMeshes(const std::vector<Mesh*>& meshes, const Shader& shader) const
	{
		for (auto& mesh : meshes)
		{
			glm::mat4 world = glm::mat4(1.0f);
			world = glm::translate(world, mesh->_translation);
			world = glm::rotate(world, mesh->_rotation, mesh->_rotationAxis);
			world = glm::scale(world, mesh->_scale);
			shader.setMat4(G_WORLD_MATRIX, world);
			shader.setMat3(G_NORMAL_MATRIX, glm::mat3(glm::transpose(glm::inverse(world))));

			if (mesh->_material)
			{
				mesh->_material->setUniforms(shader);
			}

			mesh->render();
		}
	}
}