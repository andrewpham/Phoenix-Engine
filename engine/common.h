#pragma once
#include <glm/glm.hpp>

namespace phoenix
{
	// Less tweakable parameters
	static const float PERSPECTIVE_NEAR_PLANE = 0.1f, PERSPECTIVE_FAR_PLANE = 100.0f, MOVEMENT_SPEED = 2.5f, MOUSE_SENSITIVITY = 0.1f, FOV = 45.0f, MIN_FOV = 1.0f, MAX_FOV = 45.0f;
	static const float CAMERA_YAW = -48.3996f; // Initial yaw of our camera
	static const float CAMERA_PITCH = -8.4003f; // Initial pitch of our camera
	static const float MAX_CAMERA_PITCH = 89.0f;
	static const unsigned int SCREEN_WIDTH = 2560, SCREEN_HEIGHT = 1440;
	static const glm::vec3 UP(0.0f, 1.0f, 0.0f);

	static const glm::vec3 CAMERA_POS(-9.2906f, 2.03786f, 10.2668f); // Starting position of our camera
}