#pragma once

#include <vector>
#include <functional>

namespace afre
{
	class CleanupStack
	{
	public:
		inline void PushCleanup(std::function<void()> functionToPush)
		{
			m_cleanupStack.push_back(functionToPush);
		}

		void StartCleanup();

	private:
		std::vector<std::function<void()>> m_cleanupStack{};
	};
}