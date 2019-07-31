#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <engine/utils.h>
#include <engine/model.h>
#include <engine/camera.h>

namespace phoenix
{
	// Uniform names
	static const std::string G_DIFFUSE_TEXTURE = "gDiffuseTexture";
	static const std::string G_SHADOW_MAP = "gShadowMap";
	static const std::string G_LIGHT_COLOR = "gLightColor";
	static const std::string G_DEPTH_MAP = "gDepthMap";

	// Less tweakable parameters
	static const unsigned int SHADOW_MAP_WIDTH = 1024, SHADOW_MAP_HEIGHT = 1024;
	static const glm::vec3 OBJ_SCALE(2.5f, 2.5f, 2.5f);
	static const glm::vec3 TARGET(-0.26f, 0.0f, -0.94f);

	// Tweakable parameters
	static const float CALIBRATED_LIGHT_SIZE = 30.0f;
	static const glm::vec3 LIGHT_COLOR(0.3f);
	static const glm::vec3 LIGHT_POS(-1.45814f, 6.23186f, -1.12249f); // Starting position of our light source

	class ShadowCommon
	{
	public:
		glm::vec3 _lightPos;

		float _deltaTime = 0.0f, _lastTimestamp = 0.0f;

		unsigned int _renderMode = 0, _floorTexture = 0, _objectTexture = 0, _altObjTexture = 0;

		ShadowCommon(glm::vec3 lightPos = LIGHT_POS) : _lightPos(lightPos) {}

		void processInput(GLFWwindow*, Camera*, bool);
		// Assumes that the color texture is always bound to unit 0
		void changeColorTexture(unsigned int);
		void renderObject(const Utils*, const Shader&, Model&, glm::vec3, float);
		void renderScene(Utils*, const Shader&, Model&);
		void setUniforms(const Shader&, const Camera*);
		void renderDebugLines(const Shader&, Utils*);
		void setLightSpaceVP(const Shader&, const glm::vec3&, const glm::vec3&);

	private:
		unsigned int _debugLinesVAO = 0, _debugLinesVBO = 0;
	};
}