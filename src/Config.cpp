// Save a config file

#include <stdio.h>
#include <stdlib.h>
#include "Config.h"

void UserData::Append( strref str )
{
	if( !str.get_len() ) { return; }
	if( str.get_len() > left ) {
		size_t newCap = capacity ? (capacity * 2) : 256;
		while( str.get_len() > (newCap - capacity + left) ) { newCap <<= 1; }
		char* newBuf = (char*)malloc( newCap );
		curr = newBuf + (curr - start);
		if( start ) {
			memcpy( newBuf, start, capacity - left );
			free( start );
		}
		start = newBuf;
		left += (newCap - capacity);
		capacity = newCap;
	}
	memcpy( curr, str.get(), str.get_len() );
	curr += str.get_len();
	left -= str.get_len();
}

UserData::~UserData()
{
	if( start ) { free( start ); }
}

void UserData::AppendIndent()
{
	strown<128> indent;
	for( int d = 0; d < depth; ++d ) { indent.append( "  " ); }
	Append( indent.get_strref() );
}

// add a single value
void UserData::AddValue( strref name, strref value )
{
	AppendIndent();
	if( name ) {
		Append( name );
		Append( strref( " = " ) );
	}
	Append( value );
	Append( strref( "\n" ) );
}

// add a single value
void UserData::AddValue( strref name, int value )
{
	AppendIndent();
	if( name ) {
		Append( name );
		Append( " = " );
	}
	strown<64> num;
	num.sprintf( "%d\n", value );
	Append( num.c_str() );
}

// add a struct
void UserData::BeginStruct( strref name )
{
	AppendIndent();
	if( name ) {
		Append( name );
		Append( " " );
	}
	Append( strref( "{\n" ) );
	++depth;
}

void UserData::EndStruct()
{
	--depth;
	AppendIndent();
	Append( strref( "}\n" ) );
}

// add an array
void UserData::BeginArray( strref name )
{
	AppendIndent();
	if( name ) { Append( name ); Append( strref( " " ) ); }
	Append( strref( "[\n" ) );
	++depth;
}

void UserData::EndArray()
{
	--depth;
	AppendIndent();
	Append( strref( "]\n" ) );
}

strref UserData::OnOff( bool on )
{
	return on ? strref( "On" ) : strref( "Off" );
}

ConfigParseType ConfigParse::Next( strref * name, strref * value )
{
	parse.skip_whitespace();

	strref n;
	strref v;

	char f = parse[ 0 ];

	if( f != '{' && f != '[' ) {
		n = parse.split_label();
		if( parse[ 0 ] == '=' ) {
			++parse;
			v = parse.line();
			v.trim_whitespace();
			*name = n; *value = v;
			return CPT_Value;
		}
	}

	switch( parse[ 0 ] ) {
		case '{':
			*name = n;
			*value = parse.scoped_block_skip(true);
			value->trim_whitespace();
			return CPT_Struct;
		case '[':
			*name = n;
			*value = parse.scoped_block_skip(true);
			value->trim_whitespace();
			return CPT_Array;
	}
	return CPT_Error;
}

strref ConfigParse::ArrayElement()
{
	parse.skip_whitespace();
	strref ret;
	if( parse[ 0 ] == '{' ) { ret = parse.scoped_block_skip(true); }
	else { ret = parse.line(); }
	parse.skip_whitespace();
	ret.trim_whitespace();
	return ret;
}
