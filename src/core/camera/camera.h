#pragma once

#include <glm/glm.hpp>
#include "core/descriptor_manager.h"

namespace afre
{
	class Camera {
	public:
		float m_yaw = 0.f;
		float m_pitch = -1.f;

		glm::vec3 m_camOrigin{ 4, 4, 4 };

		glm::vec3 m_camTarget{ 4, 4, 4 };

		const glm::vec3 m_camUp{ 0, 1, 0 };

		glm::mat4 m_CTWMat{};

		static void Rotate(float pitchIntent, float pitchIntent);
	};
}