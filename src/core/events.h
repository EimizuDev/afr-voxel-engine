#pragma once

struct GLFWwindow;

namespace afre {
	enum Keys
	{
		SPACE = 32,
		A = 65,
		D = 68,
		S = 83,
		W = 87,
		RIGHT = 262,
		LEFT = 263,
		DOWN = 264,
		UP = 265,
		LEFT_SHIFT = 340
	};

	enum KeyActions
	{
		RELEASE = 0,
		PRESS = 1,
		REPEAT = 2
	};

	// Define this in your application to get key press calls.
	void OnKey(Keys key, KeyActions action);
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	// Define this in your application to get a call with X and Y intent.
	void OnCursorMove(double xPos, double yPos);
	void CursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
}