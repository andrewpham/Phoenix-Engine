#include <engine/voxel_cone_tracing.h>
#include <engine/common.h>
#include <engine/strings.h>
#include <engine/material_store.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

float _lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float _lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool _calibratedCursor = false;

phoenix::VoxelConeTracingScene* _scenePtr;

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
	if (!_calibratedCursor)
	{
		_lastX = x;
		_lastY = y;
		_calibratedCursor = true;
	}

	float xOffset = x - _lastX;
	float yOffset = _lastY - y; // Reversed for y-coordinates

	_lastX = x;
	_lastY = y;

	if (_scenePtr && _scenePtr->_camera)
	{
		_scenePtr->_camera->processMouseMovement(xOffset, yOffset);
	}
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (_scenePtr && _scenePtr->_camera)
	{
		_scenePtr->_camera->processMouseScroll(yOffset);
	}
}

namespace phoenix
{
	VoxelConeTracing::VoxelConeTracing()
	{
		std::cout << "Initializing...\n";
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_SAMPLES, 0);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
		_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Voxel Cone Tracing", nullptr, nullptr);
		if (!_window)
		{
			std::cerr << phoenix::GLFW_CREATE_WINDOW_ERROR;
			glfwTerminate();
			return;
		}
		glfwMakeContextCurrent(_window);
		glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);
		glfwSetCursorPosCallback(_window, cursorPosCallback);
		glfwSetScrollCallback(_window, scrollCallback);
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSwapInterval(0);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cerr << phoenix::GLAD_LOAD_GL_LOADER_ERROR;
			return;
		}

		MaterialStore::getInstance();
		_renderer = new Renderer();
		std::cout << "Renderer initialized.\n";

		_scene = new VoxelConeTracingScene();
		_scenePtr = _scene;
		std::cout << "Scene initialized.\n";

		_utils = new Utils();
	}

	void VoxelConeTracing::run()
	{
		std::cout << "Running...\n";
		while (!glfwWindowShouldClose(_window))
		{
			_utils->_projection = glm::perspective(glm::radians(_scene->_camera->_FOV), static_cast<float>(phoenix::SCREEN_WIDTH) / phoenix::SCREEN_HEIGHT, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE);
			_utils->_view = _scene->_camera->getViewMatrix();

			float currentFrame = glfwGetTime();
			_utils->_deltaTime = currentFrame - _utils->_lastTimestamp;
			_utils->_lastTimestamp = currentFrame;

			processInput();

			_renderer->render(_scene, _renderMode);

			glfwSwapBuffers(_window);
			glfwPollEvents();
		}

		glfwTerminate();
	}

	VoxelConeTracing::~VoxelConeTracing()
	{
		delete _renderer;
		delete _scene;
		delete _utils;
	}

	void VoxelConeTracing::processInput()
	{
		if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(_window, true);
		}

		if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
		{
			_scene->_camera->processKeyPress(phoenix::FORWARD, _utils->_deltaTime);
		}
		if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
		{
			_scene->_camera->processKeyPress(phoenix::BACKWARD, _utils->_deltaTime);
		}
		if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
		{
			_scene->_camera->processKeyPress(phoenix::LEFT, _utils->_deltaTime);
		}
		if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
		{
			_scene->_camera->processKeyPress(phoenix::RIGHT, _utils->_deltaTime);
		}

		if (_scene->_lightSphere)
		{
			if (glfwGetKey(_window, GLFW_KEY_I) == GLFW_PRESS)
			{
				_scene->_lightSphere->_meshes.back()->_translation.y += MOVEMENT_SPEED * _utils->_deltaTime;
			}
			if (glfwGetKey(_window, GLFW_KEY_K) == GLFW_PRESS)
			{
				_scene->_lightSphere->_meshes.back()->_translation.y -= MOVEMENT_SPEED * _utils->_deltaTime;
			}
			if (glfwGetKey(_window, GLFW_KEY_J) == GLFW_PRESS)
			{
				_scene->_lightSphere->_meshes.back()->_translation.x -= MOVEMENT_SPEED * _utils->_deltaTime;
			}
			if (glfwGetKey(_window, GLFW_KEY_L) == GLFW_PRESS)
			{
				_scene->_lightSphere->_meshes.back()->_translation.x += MOVEMENT_SPEED * _utils->_deltaTime;
			}
			if (glfwGetKey(_window, GLFW_KEY_Y) == GLFW_PRESS)
			{
				_scene->_lightSphere->_meshes.back()->_translation.z -= MOVEMENT_SPEED * _utils->_deltaTime;
			}
			if (glfwGetKey(_window, GLFW_KEY_H) == GLFW_PRESS)
			{
				_scene->_lightSphere->_meshes.back()->_translation.z += MOVEMENT_SPEED * _utils->_deltaTime;
			}
			_scene->_pointLight->_position = _scene->_lightSphere->_meshes.back()->_translation;
		}

		if (glfwGetKey(_window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			_renderMode = RenderMode::DEFAULT;
		}
		if (glfwGetKey(_window, GLFW_KEY_E) == GLFW_PRESS)
		{
			_renderMode = RenderMode::VOXEL;
		}

		if (glfwGetKey(_window, GLFW_KEY_1) == GLFW_PRESS)
		{
			_renderMode = RenderMode::DEFAULT;
		}
		if (glfwGetKey(_window, GLFW_KEY_2) == GLFW_PRESS)
		{
			_renderMode = RenderMode::VOXEL;
		}
	}
}