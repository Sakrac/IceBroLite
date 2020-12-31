#pragma once

#include "struse/struse.h"

struct ConfigContext {
	int depth;

	ConfigContext() : depth( 0 ) {}
};

// write the config file
struct UserData {
	char* start;
	char* curr;
	size_t left;
	size_t capacity;
	int depth;

	UserData() : start( nullptr ), curr( nullptr ), left( 0 ), capacity( 0 ), depth( 0 ) {}
	void Append( strref str );
	~UserData();
	void AppendIndent();
	void AddValue( strref name, strref value );
	void AddValue( strref name, int value );
	void BeginStruct( strref name = strref() );
	void EndStruct();
	void BeginArray( strref name = strref() );
	void EndArray();
	strref OnOff( bool on );
};


enum ConfigParseType {
	CPT_Error,
	CPT_Value,
	CPT_Struct,
	CPT_Array
};

// read the config file
struct ConfigParse {
	strref parse;
	ConfigParse( void* file, size_t size ) : parse( (const char*)file, (strl_t)size ) {}
	ConfigParse( strref text ) : parse( text ) {}
	bool Empty() { parse.skip_whitespace(); return parse.is_empty(); }

	ConfigParseType Next( strref* name, strref* value );
	strref ArrayElement();
};

