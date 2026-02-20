#include "camera.h"
#include <glm/ext/matrix_transform.inl>

#include "scene.h"

namespace afre
{
	BufferData<CameraData>::BufferData()
	{
		const auto& camView = g_scene.m_registry.view<Camera>();

		if (Camera* camera = &g_scene.m_registry.get<Camera>(camView.front()))
		{
			static glm::vec3 oldCamOrigin{};
			static glm::vec3 oldCamTarget{};

			if (camera->m_camOrigin != oldCamOrigin || camera->m_camTarget != oldCamTarget)
			{
				oldCamOrigin = camera->m_camOrigin;
				oldCamTarget = camera->m_camTarget;

				CameraData buf{};

				camera->m_CTWMat = glm::inverse(glm::lookAt(camera->m_camOrigin, camera->m_camTarget, camera->m_camUp));
				buf.m_CTWMat = camera->m_CTWMat;

				m_data = buf;
				m_shouldCopy = true;
			}
		}
	}

	void Camera::Rotate(float yawIntent, float pitchIntent)
	{
		const auto& camView = g_scene.m_registry.view<Camera>();

		if (Camera* camera = &g_scene.m_registry.get<Camera>(camView.front()))
		{
			camera->m_yaw = (yawIntent + camera->m_yaw >= 360 || yawIntent + camera->m_yaw <= -360 ? 0 : yawIntent + camera->m_yaw);
			camera->m_pitch = glm::clamp(camera->m_pitch + pitchIntent, -179.f, -1.f);
		}
	}
}