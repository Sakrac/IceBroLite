#pragma once
#include <stdint.h>
#include "../struse/struse.h"
struct UserData;

struct RegisterView
{
	RegisterView();
	void WriteConfig( UserData & config );
	void ReadConfig( strref config );
	int cursor;
	float cursorTime;
	bool open;
	void Draw();
};

