#pragma once
#include <string>

namespace phoenix
{
	// Uniform names
	static const std::string G_WVP = "gWVP";
	static const std::string G_VP = "gVP";
	static const std::string G_NORMAL_MATRIX = "gNormalMatrix";
	static const std::string G_WORLD_MATRIX = "gWorldMatrix";
	static const std::string G_VIEW_MATRIX = "gViewMatrix";
	static const std::string G_PROJECTION_MATRIX = "gProjectionMatrix";
	static const std::string G_INVERSE_VIEW_MATRIX = "gInverseViewMatrix";
	static const std::string G_LIGHT_SPACE_VP = "gLightSpaceVP";
	static const std::string G_VIEW_POS = "gViewPos";
	static const std::string G_CORRECTION_FACTOR = "gCorrectionFactor";
	static const std::string G_DIFFUSE_MAP = "gDiffuseMap";
	static const std::string G_SPECULAR_MAP = "gSpecularMap";
	static const std::string G_AMBIENT_MAP = "gAmbientMap";
	static const std::string G_BUMP_MAP = "gBumpMap";
	static const std::string G_OPACITY_MAP = "gOpacityMap";
	static const std::string G_NORMAL_MAP = "gNormalMap";
	static const std::string G_METALLIC_MAP = "gMetallicMap";
	static const std::string G_POSITION_MAP = "gPositionMap";
	static const std::string G_ALBEDO_SPECULAR_MAP = "gAlbedoSpecularMap";
	static const std::string G_FLUX_MAP = "gFluxMap";
	static const std::string G_AMBIENT_FACTOR = "gAmbientFactor";
	static const std::string G_SPECULAR_FACTOR = "gSpecularFactor";
	static const std::string G_METALNESS = "gMetalness";
	static const std::string G_TEXTURE_3D = "gTexture3D";
	static const std::string G_RENDER_MODE = "gRenderMode";
	static const std::string G_OUTPUT = "gOutput";
	static const std::string G_ANGLES_TEXTURE = "gAnglesTexture";
	static const std::string G_CALIBRATED_LIGHT_SIZE = "gCalibratedLightSize";
	static const std::string G_RENDER_TARGET = "gRenderTarget";
	static const std::string G_USE_NORMAL_MAP = "gUseNormalMap";

	// Error messages
	static const std::string GLFW_CREATE_WINDOW_ERROR = "Failed to create GLFW window!\n";
	static const std::string GLAD_LOAD_GL_LOADER_ERROR = "Failed to initialize GLAD!\n";
	static const std::string FILE_STREAM_OPEN_ERROR = "Exception opening/reading/closing file!\n";
	static const std::string FRAMEBUFFER_INIT_ERROR = "Failed to initialize the framebuffer!\n";
}