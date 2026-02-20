#include "scene.h"
#include "core/camera/camera.h"

namespace afre
{
	Scene::Scene()
	{
		// Creating the camera
		entt::entity camera = m_registry.create();
		m_registry.emplace<Camera>(camera);

		// Creating the voxel world
		entt::entity voxelWorld = m_registry.create();
		m_registry.emplace<VoxelData>(voxelWorld);
	}
}