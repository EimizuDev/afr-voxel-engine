#include "events.h"

namespace afre {
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		OnKey((Keys)key, (KeyActions)action);
	}

	void CursorPositionCallback(GLFWwindow* window, double xPos, double yPos)
	{
		OnCursorMove(xPos, yPos);
	}
}