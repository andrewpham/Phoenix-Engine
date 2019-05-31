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
		glm::vec3 _position;
		glm::vec3 _forward;
		glm::vec3 _up;
		glm::vec3 _right;

		float _yaw;
		float _pitch;
		float _FOV;

		Camera();

		glm::mat4 getViewMatrix();

		void processKeyPress(Direction, float);
		void processMouseMovement(float, float);
		void processMouseScroll(float);

	private:
		void update();
	};
}