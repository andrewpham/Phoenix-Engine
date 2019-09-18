#pragma once
#include <glm/glm.hpp>

namespace phoenix
{
	// Less tweakable parameters
	static const float PERSPECTIVE_NEAR_PLANE = 0.1f, PERSPECTIVE_FAR_PLANE = 100.0f, MOVEMENT_SPEED = 2.5f,
		MOUSE_SENSITIVITY = 0.1f, FOV = 45.0f, MIN_FOV = 1.0f, MAX_FOV = 45.0f, NEAR_PLANE = 1.0f, FAR_PLANE = 7.5f,
		ORTHO_PROJ_HALF_WIDTH = 10.0f, MAX_CAMERA_PITCH = 89.0f, AMBIENT_FACTOR = 0.3f, SPECULAR_FACTOR = 64.0f;
	static const float CAMERA_YAW = -48.3996f; // Initial yaw of our camera
	static const float CAMERA_PITCH = -8.4003f; // Initial pitch of our camera
	static const float BORDER_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	static const unsigned int SCREEN_WIDTH = 2560, SCREEN_HEIGHT = 1440, NUM_CUBEMAP_FACES = 6, HIGH_RES_WIDTH = 4096, HIGH_RES_HEIGHT = 4096;

	static const glm::vec3 UP(0.0f, 1.0f, 0.0f);
	static const glm::vec3 CAMERA_POS(-9.2906f, 2.03786f, 10.2668f); // Starting position of our camera
}