#pragma once
#include <glm/glm.hpp>

namespace phoenix
{
	enum Direction
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	class Camera
	{
	public:
		glm::vec3 _position, _forward, _up, _right;

		float _yaw, _pitch, _FOV;

		Camera();

		glm::mat4 getViewMatrix();

		void processKeyPress(Direction, float);
		void processMouseMovement(float, float);
		void processMouseScroll(float);

	private:
		void update();
	};
}