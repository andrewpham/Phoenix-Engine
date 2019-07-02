#pragma once
#include <engine/texture3D.h>
#include <engine/framebuffer.h>
#include <engine/model.h>
#include <engine/voxel_cone_tracing_scene.h>

namespace phoenix
{
	enum RenderMode
	{
		VOXEL = 0,
		DEFAULT = 1
	};

	// Graphics context
	class Renderer
	{
	public:
		void render(VoxelConeTracingScene*, RenderMode = RenderMode::DEFAULT);

		Renderer();
		~Renderer();

	private:
		Shader* _renderShader;
		// Voxelization variables
		Shader* _voxelizeShader;
		Texture3D* _voxelTexture;
		int _voxelTextureRes = 64;
		// Voxel render mode variables
		Shader* _worldPositionOutputShader;
		Shader* _visualizeVoxelsShader;
		Framebuffer* _backfaceBuffer;
		Framebuffer* _frontfaceBuffer;
		Model* _cubeModel;
		Mesh* _quadMesh;

		// Default render mode function
		void renderScene(VoxelConeTracingScene*);

		// Voxelization functions
		void initVoxelization();
		void voxelize(VoxelConeTracingScene*);

		// Voxel render mode functions
		void initVoxelVisualization();
		void renderVoxelVisualization(VoxelConeTracingScene*);

		void setCameraUniforms(const Shader&, Camera*);
		void renderMeshes(const std::vector<Mesh*>&, const Shader&) const;
	};
}