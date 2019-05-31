#pragma once
#include <string>

namespace phoenix
{
	// Uniform names
	static const std::string G_WVP = "gWVP";
	static const std::string G_NORMAL_MATRIX = "gNormalMatrix";
	static const std::string G_WORLD_MATRIX = "gWorldMatrix";
	static const std::string G_LIGHT_SPACE_VP = "gLightSpaceVP";
	static const std::string G_VIEW_POS = "gViewPos";
	static const std::string G_CORRECTION_FACTOR = "gCorrectionFactor";

	// Error messages
	static const std::string GLFW_CREATE_WINDOW_ERROR = "Failed to create GLFW window!\n";
	static const std::string GLAD_LOAD_GL_LOADER_ERROR = "Failed to initialize GLAD!\n";
}