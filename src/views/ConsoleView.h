#pragma once
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include <stdlib.h>
struct UserData;

struct IceConsole
{
	char InputBuf[ 256 ];
	int HistoryPos; // -1: new line, 0..History.Size-1 browsing history.
	bool ScrollToBottom;
	bool open;
	ImVector<char*> Items;
	ImVector<char*> History;
	ImVector<char*> safeItems;
//	ImVector<const char*> Commands;

	IceConsole();
	~IceConsole();

	void Init();

	void WriteConfig( UserData & config );

	// Portable helpers
	static int Stricmp( const char* str1, const char* str2 ) { int d; while( (d = strref::toupper( *str2 ) - strref::toupper( *str1 )) == 0 && *str1 ) { str1++; str2++; } return d; }
	static int Strnicmp( const char* str1, const char* str2, int n ) { int d = 0; while( n > 0 && (d = strref::toupper( *str2 ) - strref::toupper( *str1 )) == 0 && *str1 ) { str1++; str2++; n--; } return d; }
	static char* Strdup( const char *str ) { size_t len = strlen( str ) + 1; void* buff = malloc( len ); return buff ? (char*)memcpy( buff, (const void*)str, len ) : nullptr; }
	static void Strtrim( char* str ) { char* str_end = str + strlen( str ); while( str_end > str && str_end[ -1 ] == ' ' ) str_end--; *str_end = 0; }

	void ReadConfig( strref config );

	void ClearLog();
	void AddLog( const char* fmt, ... ) IM_FMTARGS( 2 );
	void AddLog( strref line );
	void AddLogSafe( strref line );
	void FlushLogSafe();
	void Draw();
	void ExecCommand( const char* command_line );
	static int TextEditCallbackStub( ImGuiInputTextCallbackData* data ); // In C++11 you are better off using lambdas for this sort of forwarding callbacks
	int TextEditCallback( ImGuiInputTextCallbackData* data );

	static void	LogCB( void* user, const char* text, size_t len );

};

