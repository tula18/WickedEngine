#include "wiInput_BindLua.h"
#include "Vector_BindLua.h"

const char wiInput_BindLua::className[] = "Input";

Luna<wiInput_BindLua>::FunctionType wiInput_BindLua::methods[] = {
	lunamethod(wiInput_BindLua, Down),
	lunamethod(wiInput_BindLua, Press),
	lunamethod(wiInput_BindLua, Hold),
	lunamethod(wiInput_BindLua, GetPointer),
	lunamethod(wiInput_BindLua, SetPointer),
	lunamethod(wiInput_BindLua, HidePointer),
	lunamethod(wiInput_BindLua, GetAnalog),
	lunamethod(wiInput_BindLua, GetTouches),
	{ NULL, NULL }
};
Luna<wiInput_BindLua>::PropertyType wiInput_BindLua::properties[] = {
	{ NULL, NULL }
};

int wiInput_BindLua::Down(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::BUTTON button = (wiInput::BUTTON)wiLua::SGetInt(L, 1);
		short playerindex = 0;
		if (argc > 1)
		{
			playerindex = (short)wiLua::SGetInt(L, 2);
		}
		wiLua::SSetBool(L, wiInput::down(button, playerindex));
		return 1;
	}
	else
		wiLua::SError(L, "Down(int code, opt int playerindex = 0) not enough arguments!");
	return 0;
}
int wiInput_BindLua::Press(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::BUTTON code = (wiInput::BUTTON)wiLua::SGetInt(L, 1);
		short playerindex = 0;
		if (argc > 1)
		{
			playerindex = (short)wiLua::SGetInt(L, 2);
		}
		wiLua::SSetBool(L, wiInput::press(code, playerindex));
		return 1;
	}
	else
		wiLua::SError(L, "Press(int code, opt int playerindex = 0) not enough arguments!");
	return 0;
}
int wiInput_BindLua::Hold(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::BUTTON button = (wiInput::BUTTON)wiLua::SGetInt(L, 1);
		uint32_t duration = 30;
		if (argc > 1)
		{
			duration = wiLua::SGetInt(L, 2);
		}
		bool continuous = false;
		if (argc > 2)
		{
			continuous = wiLua::SGetBool(L, 3);
		}
		short playerindex = 0;
		if (argc > 3)
		{
			playerindex = (short)wiLua::SGetInt(L, 4);
		}
		wiLua::SSetBool(L, wiInput::hold(button, duration, continuous, playerindex));
		return 1;
	}
	else
		wiLua::SError(L, "Hold(int code, opt int duration = 30, opt boolean continuous = false, opt int playerindex = 0) not enough arguments!");
	return 0;
}
int wiInput_BindLua::GetPointer(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&wiInput::getpointer())));
	return 1;
}
int wiInput_BindLua::SetPointer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMFLOAT4 props;
			XMStoreFloat4(&props, vec->vector);
			wiInput::setpointer(props);
		}
		else
			wiLua::SError(L, "SetPointer(Vector props) argument is not a Vector!");
	}
	else
		wiLua::SError(L, "SetPointer(Vector props) not enough arguments!");
	return 0;
}
int wiInput_BindLua::HidePointer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::hidepointer(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "HidePointer(bool value) not enough arguments!");
	return 0;
}
int wiInput_BindLua::GetAnalog(lua_State* L)
{
	XMFLOAT4 result = XMFLOAT4(0, 0, 0, 0);

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::GAMEPAD_ANALOG type = (wiInput::GAMEPAD_ANALOG)wiLua::SGetInt(L, 1);
		short playerindex = 0;
		if (argc > 1)
		{
			playerindex = (short)wiLua::SGetInt(L, 2);
		}
		result = wiInput::getanalog(type, playerindex);
	}
	else
		wiLua::SError(L, "GetAnalog(int type, opt int playerindex = 0) not enough arguments!");

	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&result)));
	return 1;
}
int wiInput_BindLua::GetTouches(lua_State* L)
{
	auto& touches = wiInput::getTouches();
	for (auto& touch : touches)
	{
		Luna<Touch_BindLua>::push(L, new Touch_BindLua(touch));
	}
	return (int)touches.size();
}

void wiInput_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiInput_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
		wiLua::GetGlobal()->RunText("input = Input()");

		wiLua::GetGlobal()->RunText("MOUSE_BUTTON_LEFT			= 1");
		wiLua::GetGlobal()->RunText("MOUSE_BUTTON_RIGHT			= 2");
		wiLua::GetGlobal()->RunText("MOUSE_BUTTON_MIDDLE		= 3");

		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_UP			= 4");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_DOWN		= 5");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_LEFT		= 6");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_RIGHT		= 7");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_SPACE		= 8");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_RSHIFT		= 9");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_LSHIFT		= 10");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F1			= 11");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F2			= 12");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F3			= 13");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F4			= 14");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F5			= 15");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F6			= 16");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F7			= 17");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F8			= 18");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F9			= 19");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F10		= 20");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F11		= 21");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_F12		= 22");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_ENTER		= 23");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_ESCAPE		= 24");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_HOME		= 25");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_RCONTROL	= 26");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_LCONTROL	= 27");
		wiLua::GetGlobal()->RunText("KEYBOARD_BUTTON_DELETE		= 28");

		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_UP			= 257");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_LEFT		= 258");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_DOWN		= 259");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_RIGHT		= 260");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_1			= 261");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_2			= 262");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_3			= 263");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_4			= 264");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_5			= 265");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_6			= 266");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_7			= 267");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_8			= 268");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_9			= 269");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_10			= 270");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_11			= 271");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_12			= 272");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_13			= 273");
		wiLua::GetGlobal()->RunText("GAMEPAD_BUTTON_14			= 274");

		//Analog
		wiLua::GetGlobal()->RunText("GAMEPAD_ANALOG_THUMBSTICK_L	= 0");
		wiLua::GetGlobal()->RunText("GAMEPAD_ANALOG_THUMBSTICK_R	= 1");
		wiLua::GetGlobal()->RunText("GAMEPAD_ANALOG_TRIGGER_L		= 2");
		wiLua::GetGlobal()->RunText("GAMEPAD_ANALOG_TRIGGER_R		= 3");

		//Touch
		wiLua::GetGlobal()->RunText("TOUCHSTATE_PRESSED		= 0");
		wiLua::GetGlobal()->RunText("TOUCHSTATE_RELEASED	= 1");
		wiLua::GetGlobal()->RunText("TOUCHSTATE_MOVED		= 2");
	}

	Touch_BindLua::Bind();
}







const char Touch_BindLua::className[] = "Touch";

Luna<Touch_BindLua>::FunctionType Touch_BindLua::methods[] = {
	lunamethod(Touch_BindLua, GetState),
	lunamethod(Touch_BindLua, GetPos),
	{ NULL, NULL }
};
Luna<Touch_BindLua>::PropertyType Touch_BindLua::properties[] = {
	{ NULL, NULL }
};

int Touch_BindLua::GetState(lua_State* L)
{
	wiLua::SSetInt(L, (int)touch.state);
	return 1;
}
int Touch_BindLua::GetPos(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&touch.pos)));
	return 1;
}

void Touch_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Touch_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
