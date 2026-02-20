#include "cleanup_stack.h"

namespace afre
{
	void CleanupStack::StartCleanup()
	{
		while (!m_cleanupStack.empty())
		{
			m_cleanupStack.back()();
			m_cleanupStack.pop_back();
		}
	}
}