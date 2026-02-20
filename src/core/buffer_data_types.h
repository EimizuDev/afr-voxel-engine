#pragma once

#include <glm/glm.hpp>

namespace afre
{
	struct CameraData
	{
		glm::mat4 m_CTWMat{};
	};

	struct Brick
	{
		glm::uint16_t m_voxels[16][16][16]{};
	};

	struct VoxelData
	{
		Brick m_bricks[3][3][3]{};
		glm::uint16_t m_bricksPerDim{};

		bool SetVoxelData();
	};
}