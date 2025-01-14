#pragma once
#include "CommonInclude.h"

#include <vector>

namespace wiInput
{
	// Do not alter order as it is bound to lua manually!
	enum BUTTON
	{
		BUTTON_NONE = 0,

		MOUSE_BUTTON_LEFT,
		MOUSE_BUTTON_RIGHT,
		MOUSE_BUTTON_MIDDLE,

		KEYBOARD_BUTTON_UP,
		KEYBOARD_BUTTON_DOWN,
		KEYBOARD_BUTTON_LEFT,
		KEYBOARD_BUTTON_RIGHT,
		KEYBOARD_BUTTON_SPACE,
		KEYBOARD_BUTTON_RSHIFT,
		KEYBOARD_BUTTON_LSHIFT,
		KEYBOARD_BUTTON_F1,
		KEYBOARD_BUTTON_F2,
		KEYBOARD_BUTTON_F3,
		KEYBOARD_BUTTON_F4,
		KEYBOARD_BUTTON_F5,
		KEYBOARD_BUTTON_F6,
		KEYBOARD_BUTTON_F7,
		KEYBOARD_BUTTON_F8,
		KEYBOARD_BUTTON_F9,
		KEYBOARD_BUTTON_F10,
		KEYBOARD_BUTTON_F11,
		KEYBOARD_BUTTON_F12,
		KEYBOARD_BUTTON_ENTER,
		KEYBOARD_BUTTON_ESCAPE,
		KEYBOARD_BUTTON_HOME,
		KEYBOARD_BUTTON_RCONTROL,
		KEYBOARD_BUTTON_LCONTROL,
		KEYBOARD_BUTTON_DELETE,

		CHARACTER_RANGE_START = 65, // letter A

		GAMEPAD_RANGE_START = 256, // do not use!

		GAMEPAD_BUTTON_UP,
		GAMEPAD_BUTTON_LEFT,
		GAMEPAD_BUTTON_DOWN,
		GAMEPAD_BUTTON_RIGHT,
		GAMEPAD_BUTTON_1,
		GAMEPAD_BUTTON_2,
		GAMEPAD_BUTTON_3,
		GAMEPAD_BUTTON_4,
		GAMEPAD_BUTTON_5,
		GAMEPAD_BUTTON_6,
		GAMEPAD_BUTTON_7,
		GAMEPAD_BUTTON_8,
		GAMEPAD_BUTTON_9,
		GAMEPAD_BUTTON_10,
		GAMEPAD_BUTTON_11,
		GAMEPAD_BUTTON_12,
		GAMEPAD_BUTTON_13,
		GAMEPAD_BUTTON_14,
	};
	enum GAMEPAD_ANALOG
	{
		GAMEPAD_ANALOG_THUMBSTICK_L,
		GAMEPAD_ANALOG_THUMBSTICK_R,
		GAMEPAD_ANALOG_TRIGGER_L,
		GAMEPAD_ANALOG_TRIGGER_R,
	};

	// call once at app start
	void Initialize();

	// call once per frame
	void Update();
	
	// check if a button is down
	bool down(BUTTON button, short playerindex = 0);
	// check if a button is pressed once
	bool press(BUTTON button, short playerindex = 0);
	// check if a button is held down
	bool hold(BUTTON button, uint32_t frames = 30, bool continuous = false, short playerIndex = 0);
	// get pointer position (eg. mouse pointer) (.xy) + scroll delta (.z) + 1 unused (.w)
	XMFLOAT4 getpointer();
	// set pointer position (eg. mouse pointer) + scroll delta (.z)
	void setpointer(const XMFLOAT4& props);
	// hide pointer
	void hidepointer(bool value);
	// read analog input from controllers, like thumbsticks or triggers
	XMFLOAT4 getanalog(GAMEPAD_ANALOG analog, short playerIndex = 0);

	struct Touch
	{
		enum TOUCHSTATE
		{
			TOUCHSTATE_PRESSED,
			TOUCHSTATE_RELEASED,
			TOUCHSTATE_MOVED,
			TOUCHSTATE_COUNT,
		} state;
		// current position of touch
		XMFLOAT2 pos;
	};
	const std::vector<Touch>& getTouches();

};

