#pragma once

#include <entt.hpp>

namespace afre
{
	class Scene
	{
	public:
		void Update();

		entt::registry m_registry{};

		Scene();
	};

	_declspec(selectany) Scene g_scene{};
}