#include <engine/camera.h>
#include <engine/common.h>
#include <glm/gtc/matrix_transform.hpp>

namespace phoenix
{
	Camera::Camera() : _position(CAMERA_POS), _yaw(CAMERA_YAW), _pitch(CAMERA_PITCH), _FOV(FOV)
	{
		update();
	}

	glm::mat4 Camera::getViewMatrix()
	{
		return glm::lookAt(_position, _position + _forward, _up);
	}

	void Camera::processKeyPress(Direction direction, float deltaTime)
	{
		if (direction == FORWARD)
		{
			_position += _forward * MOVEMENT_SPEED * deltaTime;
		}
		if (direction == BACKWARD)
		{
			_position -= _forward * MOVEMENT_SPEED * deltaTime;
		}
		if (direction == LEFT)
		{
			_position -= _right * MOVEMENT_SPEED * deltaTime;
		}
		if (direction == RIGHT)
		{
			_position += _right * MOVEMENT_SPEED * deltaTime;
		}
	}

	void Camera::processMouseMovement(float xOffset, float yOffset)
	{
		xOffset *= MOUSE_SENSITIVITY;
		yOffset *= MOUSE_SENSITIVITY;

		_yaw += xOffset;
		_pitch += yOffset;

		if (_pitch > MAX_CAMERA_PITCH)
		{
			_pitch = MAX_CAMERA_PITCH;
		}
		if (_pitch < -MAX_CAMERA_PITCH)
		{
			_pitch = -MAX_CAMERA_PITCH;
		}

		update();
	}

	void Camera::processMouseScroll(float yOffset)
	{
		if (_FOV >= MIN_FOV && _FOV <= MAX_FOV)
		{
			_FOV -= yOffset;
		}
		if (_FOV <= MIN_FOV)
		{
			_FOV = MIN_FOV;
		}
		if (_FOV >= MAX_FOV)
		{
			_FOV = MAX_FOV;
		}
	}

	void Camera::update()
	{
		glm::vec3 forward;
		forward.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		forward.y = sin(glm::radians(_pitch));
		forward.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
		_forward = glm::normalize(forward);

		_right = glm::normalize(glm::cross(_forward, UP));

		_up = glm::normalize(glm::cross(_right, _forward));
	}
}