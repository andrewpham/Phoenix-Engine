#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <engine/renderer.h>
#include <engine/utils.h>

namespace phoenix
{
	class VoxelConeTracing
	{
	public:
		void run();

		VoxelConeTracing();
		~VoxelConeTracing();

	private:
		RenderMode _renderMode = RenderMode::DEFAULT;
		GLFWwindow* _window;
		Utils* _utils;
		VoxelConeTracingScene* _scene;
		Renderer* _renderer;

		void processInput();

		VoxelConeTracing(VoxelConeTracing const&) = delete;
		void operator=(VoxelConeTracing const&) = delete;
	};
}