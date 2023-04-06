// example for creating a texture:
// static void ImGui_ImplDX11_CreateFontsTexture()
#include <stdio.h>
#include <malloc.h>
#include <inttypes.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../Image.h"
#include "../Expressions.h"
#include "../Config.h"
#include "../ViceInterface.h"
#include "../6510.h"
#include "../Sym.h"
#include "Views.h"
#include "GfxView.h"
#include "GLFW/glfw3.h"
#include "../Image.h"
#include "../C64Colors.h"

#ifndef _MSC_VER
#define sprintf_s sprintf
#endif

// (Notes from Sleeping Elephant for Vic 20 graphics)
// Vic 20 VIC
// Vic-I reference: http://sleepingelephant.com/denial/wiki/index.php/MOS_Technology_VIC
//
// color ram
// The color RAM is a 1k block at memory locations $9400 - $97FF(37888 - 38911).
//. The specific memory used may start at exactly $9400, or 512 bytes higher at $9600,
// depending upon a setting in VIC register (bit 7 of $9002).The unused portion of the
// memory block(typically just over 512 bytes, but more or less if other VIC registers
// have been set to change the screen size)
//
// Palette
// #000000
// #ffffff
// #a8734a
// #e9b287
// #772d26
// #b66862
// #85d4dc
// #c5ffff
// #a85fb4
// #e99df5
// #559e4a
// #92df87
// #42348b
// #7e70ca
// #bdcc71
// #ffffb0
// 
// VIC registers
// $9000	36864	ABBBBBBB	$05	$0C	Interlace mode / Screen origin(horizontal)
// $9001	36865	CCCCCCCC	$19	$26	Screen origin(vertical)
// $9002	36866	HDDDDDDD	$16	Screen memory offset / Number of columns
// $9003	36867	GEEEEEEF	$2E	Raster value(lowest bit) / Number of rows / Double character size
// $9004	36868	GGGGGGGG	variable	Raster value
// $9005	36869	HHHHIIII	$F0	Screen memory location / Character memory location
// $900E	36878	WWWWVVVV	$00	Auxiliary color / Composite sound volume
// $900F	36879	XXXXYZZZ	$1B	Screen color / Reverse mode / Border color
//
// Values in $9005
// Byte $9005(36869) is comprised of two 4 - bit addresses for the character memoryand video memory.These addresses refer to sixteen 1k blocks.Values in the range of 0 - 7 are addresses in the $8000 - $9FFF range, and values in the range of 8 - 15 are in the $0000 - $1FFF range :
// 0 : $8000, 1k character ROM, capitals + glyphs
// 1 : $8400, 1k character ROM, capitals + lowercase
// 2 : $8800, 1k character ROM, inverse capitals + glyphs
// 3 : $8C00, 1k character ROM, inverse capitals + lowercase
// 4 : $9000, 1k VIC / VIA registers
// 5 : $9400, 1k RAM used as Color RAM
// 6 : $9800, 1k Expansion port
// 7 : $9C00, 1k Expansion port
// 8 : $0000, 1k RAM, zeropage, datasette buffer, and other various uses
// 9 : $0400, 1k Expansion port
// 10 : $0800, 1k Expansion port
// 11 : $0C00, 1k Expansion port
// 12 : $1000, 1k RAM
// 13 : $1400, 1k RAM
// 14 : $1800, 1k RAM
// 15 : $1C00, 1k RAM
// The character memory always starts at byte 0 of the selected block, while the video memory can be offset by 512 bytes by setting bit 7 of $9002. While these are the values that can be chosen via $9005, in practice, much isn't usable for a variety of reasons:

#define ColHex6( hex ) uint32_t(0xff000000|((hex<<16)&0xff0000)|(hex&0xff00)|((hex>>16)&0xff))

static uint32_t vic20pal[16] = {
	ColHex6(0x000000),
	ColHex6(0xffffff),
	ColHex6(0x772d26),
	ColHex6(0x85d4dc),
	ColHex6(0xa85fb4),
	ColHex6(0x559e4a),
	ColHex6(0x42348b),
	ColHex6(0xbdcc71),
	ColHex6(0xa8734a),
	ColHex6(0xe9b287),
	ColHex6(0xb66862),
	ColHex6(0xc5ffff),
	ColHex6(0xe99df5),
	ColHex6(0x92df87),
	ColHex6(0x7e70ca),
	ColHex6(0xffffb0),
};

unsigned char _aStartupFont[] = {
	0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x62, 0x3c, 0x00, 0x18, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00, 0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00, 0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00, 0x7e, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7e, 0x00, 0x7e, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00, 0x3c, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3c, 0x00,
	0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0x6c, 0x38, 0x00, 0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00, 0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00, 0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00,
	0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x0e, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x00, 0x3c, 0x66, 0x60, 0x3c, 0x06, 0x66, 0x3c, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00, 0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00,
	0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00, 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00, 0x0c, 0x12, 0x30, 0x7c, 0x30, 0x62, 0xfc, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x00, 0x18, 0x3c, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x00, 0x10, 0x30, 0x7f, 0x7f, 0x30, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0xff, 0x66, 0xff, 0x66, 0x66, 0x00, 0x18, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x18, 0x00, 0x62, 0x66, 0x0c, 0x18, 0x30, 0x66, 0x46, 0x00, 0x3c, 0x66, 0x3c, 0x38, 0x67, 0x66, 0x3f, 0x00, 0x06, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00, 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00,
	0x3c, 0x66, 0x6e, 0x76, 0x66, 0x66, 0x3c, 0x00, 0x18, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7e, 0x00, 0x3c, 0x66, 0x06, 0x0c, 0x30, 0x60, 0x7e, 0x00, 0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00, 0x06, 0x0e, 0x1e, 0x66, 0x7f, 0x06, 0x06, 0x00, 0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x3c, 0x66, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00, 0x7e, 0x66, 0x0c, 0x18, 0x18, 0x18, 0x18, 0x00,
	0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00, 0x3c, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30, 0x0e, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x70, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x70, 0x00, 0x3c, 0x66, 0x06, 0x0c, 0x18, 0x00, 0x18, 0x00,
	0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x08, 0x1c, 0x3e, 0x7f, 0x7f, 0x1c, 0x3e, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0xe0, 0xf0, 0x38, 0x18, 0x18, 0x18, 0x18, 0x1c, 0x0f, 0x07, 0x00, 0x00, 0x00, 0x18, 0x18, 0x38, 0xf0, 0xe0, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x03, 0x03, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc0, 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xff, 0xff, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x3c, 0x7e, 0x7e, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x36, 0x7f, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x07, 0x0f, 0x1c, 0x18, 0x18, 0xc3, 0xe7, 0x7e, 0x3c, 0x3c, 0x7e, 0xe7, 0xc3, 0x00, 0x3c, 0x7e, 0x66, 0x66, 0x7e, 0x3c, 0x00,
	0x18, 0x18, 0x66, 0x66, 0x18, 0x18, 0x3c, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x08, 0x1c, 0x3e, 0x7f, 0x3e, 0x1c, 0x08, 0x00, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0xc0, 0xc0, 0x30, 0x30, 0xc0, 0xc0, 0x30, 0x30, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x03, 0x3e, 0x76, 0x36, 0x36, 0x00, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0xcc, 0xcc, 0x33, 0x33, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x18, 0x18, 0x18, 0x1f, 0x1f, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x18, 0x18, 0x18, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xf8, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x1f, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xf8, 0xf8, 0x18, 0x18, 0x18, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xf8, 0xf8, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f,
	0xc3, 0x99, 0x91, 0x91, 0x9f, 0x99, 0xc3, 0xff, 0xe7, 0xc3, 0x99, 0x81, 0x99, 0x99, 0x99, 0xff, 0x83, 0x99, 0x99, 0x83, 0x99, 0x99, 0x83, 0xff, 0xc3, 0x99, 0x9f, 0x9f, 0x9f, 0x99, 0xc3, 0xff, 0x87, 0x93, 0x99, 0x99, 0x99, 0x93, 0x87, 0xff, 0x81, 0x9f, 0x9f, 0x87, 0x9f, 0x9f, 0x81, 0xff, 0x81, 0x9f, 0x9f, 0x87, 0x9f, 0x9f, 0x9f, 0xff, 0xc3, 0x99, 0x9f, 0x91, 0x99, 0x99, 0xc3, 0xff,
	0x99, 0x99, 0x99, 0x81, 0x99, 0x99, 0x99, 0xff, 0xc3, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xc3, 0xff, 0xe1, 0xf3, 0xf3, 0xf3, 0xf3, 0x93, 0xc7, 0xff, 0x99, 0x93, 0x87, 0x8f, 0x87, 0x93, 0x99, 0xff, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x81, 0xff, 0x9c, 0x88, 0x80, 0x94, 0x9c, 0x9c, 0x9c, 0xff, 0x99, 0x89, 0x81, 0x81, 0x91, 0x99, 0x99, 0xff, 0xc3, 0x99, 0x99, 0x99, 0x99, 0x99, 0xc3, 0xff,
	0x83, 0x99, 0x99, 0x83, 0x9f, 0x9f, 0x9f, 0xff, 0xc3, 0x99, 0x99, 0x99, 0x99, 0xc3, 0xf1, 0xff, 0x83, 0x99, 0x99, 0x83, 0x87, 0x93, 0x99, 0xff, 0xc3, 0x99, 0x9f, 0xc3, 0xf9, 0x99, 0xc3, 0xff, 0x81, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0xc3, 0xff, 0x99, 0x99, 0x99, 0x99, 0x99, 0xc3, 0xe7, 0xff, 0x9c, 0x9c, 0x9c, 0x94, 0x80, 0x88, 0x9c, 0xff,
	0x99, 0x99, 0xc3, 0xe7, 0xc3, 0x99, 0x99, 0xff, 0x99, 0x99, 0x99, 0xc3, 0xe7, 0xe7, 0xe7, 0xff, 0x81, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f, 0x81, 0xff, 0xc3, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xc3, 0xff, 0xf3, 0xed, 0xcf, 0x83, 0xcf, 0x9d, 0x03, 0xff, 0xc3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xc3, 0xff, 0xff, 0xe7, 0xc3, 0x81, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0xef, 0xcf, 0x80, 0x80, 0xcf, 0xef, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xe7, 0xff, 0x99, 0x99, 0x99, 0xff, 0xff, 0xff, 0xff, 0xff, 0x99, 0x99, 0x00, 0x99, 0x00, 0x99, 0x99, 0xff, 0xe7, 0xc1, 0x9f, 0xc3, 0xf9, 0x83, 0xe7, 0xff, 0x9d, 0x99, 0xf3, 0xe7, 0xcf, 0x99, 0xb9, 0xff, 0xc3, 0x99, 0xc3, 0xc7, 0x98, 0x99, 0xc0, 0xff, 0xf9, 0xf3, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xf3, 0xe7, 0xcf, 0xcf, 0xcf, 0xe7, 0xf3, 0xff, 0xcf, 0xe7, 0xf3, 0xf3, 0xf3, 0xe7, 0xcf, 0xff, 0xff, 0x99, 0xc3, 0x00, 0xc3, 0x99, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0x81, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xcf, 0xff, 0xff, 0xff, 0x81, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xff, 0xff, 0xfc, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f, 0xff,
	0xc3, 0x99, 0x91, 0x89, 0x99, 0x99, 0xc3, 0xff, 0xe7, 0xe7, 0xc7, 0xe7, 0xe7, 0xe7, 0x81, 0xff, 0xc3, 0x99, 0xf9, 0xf3, 0xcf, 0x9f, 0x81, 0xff, 0xc3, 0x99, 0xf9, 0xe3, 0xf9, 0x99, 0xc3, 0xff, 0xf9, 0xf1, 0xe1, 0x99, 0x80, 0xf9, 0xf9, 0xff, 0x81, 0x9f, 0x83, 0xf9, 0xf9, 0x99, 0xc3, 0xff, 0xc3, 0x99, 0x9f, 0x83, 0x99, 0x99, 0xc3, 0xff, 0x81, 0x99, 0xf3, 0xe7, 0xe7, 0xe7, 0xe7, 0xff,
	0xc3, 0x99, 0x99, 0xc3, 0x99, 0x99, 0xc3, 0xff, 0xc3, 0x99, 0x99, 0xc1, 0xf9, 0x99, 0xc3, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xe7, 0xe7, 0xcf, 0xf1, 0xe7, 0xcf, 0x9f, 0xcf, 0xe7, 0xf1, 0xff, 0xff, 0xff, 0x81, 0xff, 0x81, 0xff, 0xff, 0xff, 0x8f, 0xe7, 0xf3, 0xf9, 0xf3, 0xe7, 0x8f, 0xff, 0xc3, 0x99, 0xf9, 0xf3, 0xe7, 0xff, 0xe7, 0xff,
	0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf7, 0xe3, 0xc1, 0x80, 0x80, 0xe3, 0xc1, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf,
	0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xff, 0xff, 0xff, 0x1f, 0x0f, 0xc7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe3, 0xf0, 0xf8, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xc7, 0x0f, 0x1f, 0xff, 0xff, 0xff, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x00, 0x3f, 0x1f, 0x8f, 0xc7, 0xe3, 0xf1, 0xf8, 0xfc, 0xfc, 0xf8, 0xf1, 0xe3, 0xc7, 0x8f, 0x1f, 0x3f, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x00, 0x00, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xff, 0xc3, 0x81, 0x81, 0x81, 0x81, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xc9, 0x80, 0x80, 0x80, 0xc1, 0xe3, 0xf7, 0xff, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0xff, 0xff, 0xff, 0xf8, 0xf0, 0xe3, 0xe7, 0xe7, 0x3c, 0x18, 0x81, 0xc3, 0xc3, 0x81, 0x18, 0x3c, 0xff, 0xc3, 0x81, 0x99, 0x99, 0x81, 0xc3, 0xff,
	0xe7, 0xe7, 0x99, 0x99, 0xe7, 0xe7, 0xc3, 0xff, 0xf9, 0xf9, 0xf9, 0xf9, 0xf9, 0xf9, 0xf9, 0xf9, 0xf7, 0xe3, 0xc1, 0x80, 0xc1, 0xe3, 0xf7, 0xff, 0xe7, 0xe7, 0xe7, 0x00, 0x00, 0xe7, 0xe7, 0xe7, 0x3f, 0x3f, 0xcf, 0xcf, 0x3f, 0x3f, 0xcf, 0xcf, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xfc, 0xc1, 0x89, 0xc9, 0xc9, 0xff, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xff, 0xff, 0xff, 0xff, 0x33, 0x33, 0xcc, 0xcc, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xe7, 0xe7, 0xe7, 0xe0, 0xe0, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xe7, 0xe7, 0xe7, 0xe0, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x07, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xe0, 0xe0, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x07, 0x07, 0xe7, 0xe7, 0xe7, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xe7, 0x07, 0x07, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xf0, 0xf0, 0xf0, 0xf0,
};

void GfxView::SwapSystem() {
	if (displaySystem == System::Vic20) {
		sprintf_s(address_screen, "$%04x", v20ScreenAddr);
		sprintf_s(address_gfx, "$%04x", v20GfxAddr);
		sprintf_s(address_col, "$%04x", v20ColorAddr);
		sprintf_s(columns_str, "%d", v20Columns);
		sprintf_s(rows_str, "%d", v20Rows);
	} else {
		sprintf_s(address_screen, "$%04x", addrScreenValue);
		sprintf_s(address_gfx, "$%04x", addrGfxValue);
		sprintf_s(address_col, "$%04x", addrColValue);
		sprintf_s(columns_str, "%d", columns);
		sprintf_s(rows_str, "%d", rows);
		sprintf_s(columns_spr_str, "%d", columns_sprite);
		sprintf_s(rows_spr_str, "%d", rows_sprite);
	}
}

void GfxView::WriteConfig(UserData& config)
{
	if (address_screen[0] == 0) { strovl(address_screen, sizeof(address_screen)).append('$').append_num(addrScreenValue, 4, 16); }
	if (address_gfx[0] == 0) { strovl(address_gfx, sizeof(address_gfx)).append('$').append_num(addrGfxValue, 4, 16); }
	if (address_col[0] == 0) { strovl(address_col, sizeof(address_col)).append('$').append_num(addrColValue, 4, 16); }
	if (columns_str[0] == 0) { strovl(columns_str, sizeof(columns_str)).append('$').append_num(columns, 0, 16); }
	if (rows_str[0] == 0) { strovl(rows_str, sizeof(rows_str)).append('$').append_num(rows, 4, 16); }
	if (columns_spr_str[0] == 0) { strovl(columns_spr_str, sizeof(columns_spr_str)).append('$').append_num(columns_sprite, 0, 16); }
	if (rows_spr_str[0] == 0) { strovl(rows_spr_str, sizeof(rows_spr_str)).append('$').append_num(rows_sprite, 4, 16); }

	config.AddValue(strref("open"), config.OnOff(open));
	config.AddValue(strref("addressScreen"), strref(address_screen));
	config.AddValue(strref("addressChars"), strref(address_gfx));
	config.AddValue(strref("addressColor"), strref(address_col));
	config.AddValue(strref("mode"), displayMode);
	config.AddValue(strref("system"), displaySystem);
	config.AddValue(strref("genericMode"), genericMode);
	config.AddValue(strref("c64Mode"), c64Mode);
	config.AddValue(strref("zoom"), zoom);
	config.AddValue(strref("columns"), columns);
	config.AddValue(strref("rows"), rows);
	config.AddValue(strref("columns_sprite"), columns_sprite);
	config.AddValue(strref("rows_sprite"), rows_sprite);
	config.AddValue(strref("color"), config.OnOff(color));
	config.AddValue(strref("multicolor"), config.OnOff(multicolor));
	config.AddValue(strref("ecbm"), config.OnOff(ecbm));
	config.AddValue(strref("bg_color"), bg);
	config.AddValue(strref("fg_spr_color"), spr_col[0]);
	config.AddValue(strref("mc1_spr_color"), spr_col[1]);
	config.AddValue(strref("mc2_spr_color"), spr_col[2]);
	config.AddValue(strref("fg_color"), txt_col[0]);
	config.AddValue(strref("mc1_color"), txt_col[1]);
	config.AddValue(strref("mc2_color"), txt_col[2]);
	config.AddValue(strref("ext_color"), txt_col[3]);
}

void GfxView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		} else if (name.same_str("addressScreen") && type == ConfigParseType::CPT_Value) {
			strovl addr_scrn_str(address_screen, sizeof(address_screen));
			addr_scrn_str.copy(value);
			addrScreenValue = ValueFromExpression(addr_scrn_str.c_str());
			reeval = true;
		} else if (name.same_str("addressChars") && type == ConfigParseType::CPT_Value) {
			strovl addr_gfx_str(address_gfx, sizeof(address_gfx));
			addr_gfx_str.copy(value);
			addrGfxValue = ValueFromExpression(addr_gfx_str.c_str());
			reeval = true;
		} else if (name.same_str("addressColor") && type == ConfigParseType::CPT_Value) {
			strovl addr_gfx_str(address_col, sizeof(address_col));
			addr_gfx_str.copy(value);
			addrColValue = ValueFromExpression(addr_gfx_str.c_str());
			reeval = true;
		} else if (name.same_str("columns_spr") && type == ConfigParseType::CPT_Value) {
			strovl col_str(columns_spr_str, sizeof(columns_spr_str));
			col_str.copy(value);
			columns_sprite = ValueFromExpression(col_str.c_str());
			reeval = true;
		} else if (name.same_str("rows_spr") && type == ConfigParseType::CPT_Value) {
			strovl row_str(rows_spr_str, sizeof(rows_spr_str));
			row_str.copy(value);
			rows_sprite = ValueFromExpression(row_str.c_str());
			reeval = true;
		} else if (name.same_str("mode") && type == ConfigParseType::CPT_Value) {
			displayMode = (int)value.atoi();
			reeval = true;
		} else if (name.same_str("system") && type == ConfigParseType::CPT_Value) {
			displaySystem = (int)value.atoi();
			reeval = true;
		} else if (name.same_str("genericMode") && type == ConfigParseType::CPT_Value) {
			genericMode = (int)value.atoi();
			reeval = true;
		} else if (name.same_str("c64Mode") && type == ConfigParseType::CPT_Value) {
			c64Mode = (int)value.atoi();
			reeval = true;
		} else if (name.same_str("zoom") && type == ConfigParseType::CPT_Value) {
			zoom = (int)value.atoi();
			reeval = true;
		} else if (name.same_str("columns") && type == ConfigParseType::CPT_Value) {
			columns = (int)value.atoi();
			reeval = true;
			strovl(columns_str, sizeof(columns_str)).append_num(columns, 0, 10).c_str();
		} else if (name.same_str("rows") && type == ConfigParseType::CPT_Value) {
			rows = (int)value.atoi();
			reeval = true;
			strovl(rows_str, sizeof(rows_str)).append_num(rows, 0, 10).c_str();
		} else if (name.same_str("columns_sprite") && type == ConfigParseType::CPT_Value) {
			columns_sprite = (int)value.atoi();
			reeval = true;
			strovl(columns_spr_str, sizeof(columns_spr_str)).append_num(columns_sprite, 0, 10).c_str();
		} else if (name.same_str("rows_sprite") && type == ConfigParseType::CPT_Value) {
			rows_sprite = (int)value.atoi();
			reeval = true;
			strovl(rows_spr_str, sizeof(rows_spr_str)).append_num(rows_sprite, 0, 10).c_str();
		} else if (name.same_str("color") && type == ConfigParseType::CPT_Value) {
			color = !value.same_str("off");
		} else if (name.same_str("multicolor") && type == ConfigParseType::CPT_Value) {
			multicolor = !value.same_str("off");
		} else if (name.same_str("ecbm") && type == ConfigParseType::CPT_Value) {
			multicolor = !value.same_str("off");
		} else if (name.same_str("bg_color") && type == ConfigParseType::CPT_Value) {
			bg = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("fg_spr_color") && type == ConfigParseType::CPT_Value) {
			spr_col[0] = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("mc1_spr_color") && type == ConfigParseType::CPT_Value) {
			spr_col[1] = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("mc2_spr_color") && type == ConfigParseType::CPT_Value) {
			spr_col[2] = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("fg_color") && type == ConfigParseType::CPT_Value) {
			txt_col[0] = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("mc1_color") && type == ConfigParseType::CPT_Value) {
			txt_col[1] = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("mc2_color") && type == ConfigParseType::CPT_Value) {
			txt_col[2] = (uint8_t)value.atoi() & 0xf;
		} else if (name.same_str("ext_color") && type == ConfigParseType::CPT_Value) {
			txt_col[3] = (uint8_t)value.atoi() & 0xf;
		}
	}
}

static bool AcceptDragDropAddress(uint32_t* addrValue, char* addrName, size_t addrNameSize)
{
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AddressDragDrop")) {
			IM_ASSERT(payload->DataSize == sizeof(SymbolDragDrop));
			SymbolDragDrop* drop = (SymbolDragDrop*)payload->Data;
			if (drop->address < 0x10000) {
				*addrValue = (uint16_t)drop->address;
				strovl addrStr(addrName, (strl_t)addrNameSize);
				addrStr.copy(drop->symbol); addrStr.c_str();
				return true;
			}
		}
		ImGui::EndDragDropTarget();
	}
	return false;
}

bool GfxView::HandleContextMenu()
{
	bool redraw = false;
	if (ImGui::BeginPopupContextWindow()) {
		if (ImGui::BeginMenu("Zoom")) {
			if (ImGui::MenuItem("Pixel", nullptr, zoom == Zoom_1x1)) { zoom = Zoom_1x1; }
			if (ImGui::MenuItem("Double", nullptr, zoom == Zoom_2x2)) { zoom = Zoom_2x2; }
			if (ImGui::MenuItem("Quad", nullptr, zoom == Zoom_4x4)) { zoom = Zoom_4x4; }
			if (ImGui::MenuItem("Fit X", nullptr, zoom == Zoom_FitX)) { zoom = Zoom_FitX; }
			if (ImGui::MenuItem("Fit Y", nullptr, zoom == Zoom_FitY)) { zoom = Zoom_FitY; }
			if (ImGui::MenuItem("Fit Window", nullptr, zoom == Zoom_FitWindow)) { zoom = Zoom_FitWindow; }
			ImGui::EndMenu();
		}
		if (ImGui::Selectable("ROM Font", useRomFont)) {
			useRomFont = !useRomFont; redraw = true;
		}
		if (multicolor || (displayMode==C64_Text && ecbm)) {
			if (ImGui::Selectable("VIC colors", vicColors)) {
				vicColors = !vicColors; redraw = true;
			}
		}
		if (ImGui::BeginMenu("BG d021")) {
			uint8_t new_bg = DrawPaletteMenu(bg); if (new_bg != bg) { bg = new_bg; redraw = true; }
			ImGui::EndMenu();
		}
		if (displayMode == C64_Sprites) {
			if (ImGui::BeginMenu("FG")) {
				uint8_t new_col = DrawPaletteMenu(spr_col[0]);
				if (new_col != spr_col[0]) { spr_col[0] = new_col; redraw = true; }
				ImGui::EndMenu();
			}
			if (multicolor && !vicColors) {
				if (ImGui::BeginMenu("d025")) {
					uint8_t new_col = DrawPaletteMenu(spr_col[1]);
					if (new_col != spr_col[1]) { spr_col[1] = new_col; redraw = true; }
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("d026")) {
					uint8_t new_col = DrawPaletteMenu(spr_col[2]);
					if (new_col != spr_col[2]) { spr_col[2] = new_col; redraw = true; }
					ImGui::EndMenu();
				}
			}
			if (ImGui::BeginMenu(strown<16>().append("Spr $").append_num(hoverGfxAddr, 4, 16).c_str())) {
				if (ImGui::MenuItem("->Memory 1")) { SetMemoryViewAddr(hoverGfxAddr, 0); }
				if (ImGui::MenuItem("->Memory 2")) { SetMemoryViewAddr(hoverGfxAddr, 1); }
				if (ImGui::MenuItem("->Memory 3")) { SetMemoryViewAddr(hoverGfxAddr, 2); }
				if (ImGui::MenuItem("->Memory 4")) { SetMemoryViewAddr(hoverGfxAddr, 3); }
				ImGui::EndMenu();
			}
		} else if (displayMode == C64_Bitmap || displayMode == C64_Text) {
			if (!ecbm && !color && !multicolor && ImGui::BeginMenu("FG")) {
				uint8_t new_col = DrawPaletteMenu(txt_col[0]);
				if (new_col != txt_col[0]) { txt_col[0] = new_col; redraw = true; }
				ImGui::EndMenu();
			} else if ((multicolor || ecbm) && displayMode == C64_Text) {
				if (ImGui::BeginMenu("d022")) {
					uint8_t new_col = DrawPaletteMenu(txt_col[1]);
					if (new_col != txt_col[1]) { txt_col[1] = new_col; redraw = true; }
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("d023")) {
					uint8_t new_col = DrawPaletteMenu(txt_col[2]);
					if (new_col != txt_col[2]) { txt_col[2] = new_col; redraw = true; }
					ImGui::EndMenu();
				}
				if (ecbm && ImGui::BeginMenu("d024")) {
					uint8_t new_col = DrawPaletteMenu(txt_col[3]);
					if (new_col != txt_col[3]) { txt_col[3] = new_col; redraw = true; }
					ImGui::EndMenu();
				}
			}
			if (ImGui::BeginMenu(strown<16>().append("Scrn $").append_num(hoverScreenAddr, 4, 16).c_str())) {
				if (ImGui::MenuItem("->Memory 1")) { SetMemoryViewAddr(hoverScreenAddr, 0); }
				if (ImGui::MenuItem("->Memory 2")) { SetMemoryViewAddr(hoverScreenAddr, 1); }
				if (ImGui::MenuItem("->Memory 3")) { SetMemoryViewAddr(hoverScreenAddr, 2); }
				if (ImGui::MenuItem("->Memory 4")) { SetMemoryViewAddr(hoverScreenAddr, 3); }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(strown<16>().append("Chr $").append_num(hoverGfxAddr, 4, 16).c_str())) {
				if (ImGui::MenuItem("->Memory 1")) { SetMemoryViewAddr(hoverGfxAddr, 0); }
				if (ImGui::MenuItem("->Memory 2")) { SetMemoryViewAddr(hoverGfxAddr, 1); }
				if (ImGui::MenuItem("->Memory 3")) { SetMemoryViewAddr(hoverGfxAddr, 2); }
				if (ImGui::MenuItem("->Memory 4")) { SetMemoryViewAddr(hoverGfxAddr, 3); }
				ImGui::EndMenu();
			}
		}
		ImGui::EndPopup();
	}
	return redraw;
}


void GfxView::Draw(int index)
{
	if (!open) { return; }
	{
		strown<64> title("Graphics");
		title.append_num(index + 1, 1, 10);

		ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(520, 200), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title.c_str(), &open)) {
			ImGui::End();
			return;
		}
	}

	ImGuiContext* g = ImGui::GetCurrentContext();
	if (g->CurrentWindow == g->NavWindow) {
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_C) &&
			(ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_LEFT_CONTROL) || ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_RIGHT_CONTROL))) {
			CopyBitmapToClipboard(bitmap, bitmapWidth, bitmapHeight);
		}
	}

	bool redraw = false;
	{
		int numColumns = 4;// (displayMode == C64_Sprites) ? 4 : 5;

		strown<32> name("gfxSetupColumns");
		name.append_num(index + 1, 1, 10);
		ImGui::Columns(numColumns, name.c_str(), true);  // 5-ways, no border
		name.copy("system##");
		name.append_num(index + 1, 1, 10);
		if (ImGui::Combo(name.c_str(), &displaySystem, "Generic\0C64\0Vic20\0\0")) {
			SwapSystem();
		}
		ImGui::NextColumn();

		int prevMode = displayMode;
		if (displayMode == C64_Sprites) {
			name.copy("addr##");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), address_gfx, sizeof(address_gfx))) {
				addrGfxValue = ValueFromExpression(address_gfx);
				redraw = true;
			}
			if (AcceptDragDropAddress(&addrGfxValue, address_gfx, sizeof(address_gfx))) {
				redraw = true;
			}
			ImGui::NextColumn();
			name.copy("Color##");
			name.append_num(index + 1, 1, 10);
			int colMode = multicolor ? 1 : 0;
			ImGui::Combo(name.c_str(), &colMode, "Mono\0Multi\0\0");
			if (multicolor != !!colMode) { redraw = true; }
			multicolor = !!colMode;
			ImGui::NextColumn();
			ImGui::NextColumn();
		} else if (displayMode != C64_Current) {
			name.copy("screen##");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), address_screen, sizeof(address_screen))) {
				addrScreenValue = ValueFromExpression(address_screen);
				redraw = true;
			}
			if (AcceptDragDropAddress(&addrScreenValue, address_screen, sizeof(address_screen))) {
				redraw = true;
			}

			ImGui::NextColumn();
			name.copy("chars##");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), address_gfx, sizeof(address_gfx))) {
				addrGfxValue = ValueFromExpression(address_gfx);
				redraw = true;
			}
			if (AcceptDragDropAddress(&addrGfxValue, address_gfx, sizeof(address_gfx))) {
				redraw = true;
			}

			ImGui::NextColumn();
			name.copy("color##");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), address_col, sizeof(address_col))) {
				addrColValue = ValueFromExpression(address_col);
				redraw = true;
			}
			if (AcceptDragDropAddress(&addrColValue, address_col, sizeof(address_col))) {
				redraw = true;
			}

			ImGui::NextColumn();
		} else {
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
		}

		name.copy("gfxSetupNext");
		name.append_num(index + 1, 1, 10);
		ImGui::Columns(numColumns, name.c_str(), true);  // 5-ways, no border
		name.copy("mode##");
		name.append_num(index + 1, 1, 10);
		switch (displaySystem) {
			case Generic:
				ImGui::Combo(name.c_str(), &genericMode, "Planar\0Columns\0Colmun Text MC\0\0");
				displayMode = genericMode + Generic_Modes;
				break;
			case C64:
				ImGui::Combo(name.c_str(), &c64Mode, "Current\0Text\0Bitmap\0Sprites\0\0");
				displayMode = c64Mode + C64_Modes;
				break;
			case Vic20:
				ImGui::Combo(name.c_str(), &vic20Mode, "Current\0Text\0\0");
				displayMode = vic20Mode + V20_Modes;
				break;
		}
		if (prevMode != displayMode) { redraw = true; }
		ImGui::NextColumn();
		// multicolor check?

		if (displayMode == C64_Sprites) {
			int colMode = multicolor ? 1 : 0;
			name.copy("Color##s");
			name.append_num(index + 1, 1, 10);
			ImGui::Combo(name.c_str(), &colMode, "Mono\0Multi\0\0");
			if (multicolor != !!colMode) { redraw = true; }
			multicolor = !!colMode;
			ImGui::NextColumn();

			name.copy("cols##spr");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), columns_spr_str, sizeof(columns_spr_str))) {
				columns_sprite = ValueFromExpression(columns_spr_str);
				redraw = true;
			}
			ImGui::NextColumn();
			name.copy("rows##spr");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), rows_spr_str, sizeof(rows_spr_str))) {
				rows_sprite = ValueFromExpression(rows_spr_str);
				redraw = true;
			}

		} else if (displayMode != C64_Current && displayMode != V20_Current) {
//			ImGui::NextColumn();

			if (displayMode == C64_Bitmap || displayMode == C64_Text) {
				int colMode = color ? 1 : (multicolor ? 2 : 0);
				if (displayMode == C64_Text && ecbm) { colMode = 3; }
				int prevMode = colMode;
				name.copy("Color##");
				name.append_num(index + 1, 1, 10);
				ImGui::Combo(name.c_str(), &colMode, 
					displayMode == C64_Text ? "Mono\0Color\0Multi\0ECBM\0\0" : "Mono\0Color\0Multi\0\0");
				if (prevMode != colMode) {
					if (displayMode == C64_Text && colMode == 3) {
						ecbm = true;
					} else {
						if (displayMode == C64_Text) { ecbm = false; }
						color = colMode == 1;
						multicolor = colMode == 2;
					}
					redraw = true;
				}
				ImGui::NextColumn();
			}
			name.copy("cols##");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), columns_str, sizeof(columns_str))) {
				if (displaySystem == System::Vic20) { v20Columns = ValueFromExpression(columns_str); }
				else { columns = ValueFromExpression(columns_str); }
				redraw = true;
			}
			ImGui::NextColumn();
			name.copy("rows##");
			name.append_num(index + 1, 1, 10);
			if (ImGui::InputText(name.c_str(), rows_str, sizeof(rows_str))) {
				if (displaySystem == System::Vic20) { v20Rows = ValueFromExpression(rows_str); }
				else { rows = ValueFromExpression(rows_str); }
				redraw = true;
			}
		}
		ImGui::Columns(1);
	}

	if (HandleContextMenu()) { redraw = true; }

	CPU6510* cpu = GetCurrCPU();
	if (!bitmap || redraw || reeval || cpu->MemoryChange()) {
		Create8bppBitmap(cpu);
		reeval = false;
	}

	ImVec2 size = ImVec2(float(bitmapWidth), float(bitmapHeight));
	switch (zoom) {
		case Zoom_2x2: size.x *= 2; size.y *= 2; break;
		case Zoom_4x4: size.x *= 4; size.y *= 4; break;
		case Zoom_FitX:
		{
			float x = ImGui::GetWindowWidth();
			size.y *= x / size.x;
			size.x = x;
			break;
		}
		case Zoom_FitY:
		{
			float y = ImGui::GetWindowHeight() - ImGui::GetCursorPosY();
			size.x *= y / size.y;
			size.y = y;
			break;
		}
		case Zoom_FitWindow:
		{
			float x = ImGui::GetWindowWidth();
			float y = ImGui::GetWindowHeight() - ImGui::GetCursorPosY();
			if ((x * size.y) < (y * size.x)) {
				size.y *= x / size.x; size.x = x;
			} else {
				size.x *= y / size.y; size.y = y;
			}
			break;
		}
	}
	ImVec2 textureScreen = ImGui::GetCursorScreenPos();
	ImVec2 mousePos = ImGui::GetMousePos();
	int hoverPos[2] = { 
		(int)size.x ? ((int)(mousePos.x - textureScreen.x) * bitmapWidth) / (int)size.x : -1,
		(int)size.y ? ((int)(mousePos.y - textureScreen.y) * bitmapHeight) / (int)size.y : -1 };
	ImGui::Image(texture, size);

	hovering = false;
	if(hoverPos[0]>=0 && hoverPos[0]<bitmapWidth && hoverPos[1]>=0 && hoverPos[1]<bitmapWidth) {
		hovering = true;
		if (ImGui::GetCurrentWindow() == ImGui::GetCurrentContext()->HoveredWindow) {
			if (displayMode == C64_Current) {
				PrintCurrentInfo(cpu, hoverPos);
			} else {
				PrintHoverInfo(cpu, hoverPos, displayMode, addrScreenValue, addrGfxValue,
					addrGfxValue, ecbm);
			}
		}
	}
	ImGui::End();
}

void GfxView::PrintCurrentInfo(CPU6510* cpu, int* hoverPos) {
	uint16_t vic = (3 ^ (cpu->GetByte(0xdd00) & 3)) * 0x4000;
	uint8_t d018 = cpu->GetByte(0xd018);
	uint8_t d011 = cpu->GetByte(0xd011);
	uint8_t d016 = cpu->GetByte(0xd016);
	uint16_t chars = (d018 & 0xe) * 0x400 + vic;
	uint16_t screen = (d018 >> 4) * 0x400 + vic;
	bool mc = (d016 & 0x10) ? true : false;

	strown<128> line;
	int mode = C64_Text;
	if (d011 & 0x40) { line.append("ExtCol "); }
	else if (d011 & 0x20) { line.append(mc ? "MultiColor Bitmap " : "Bitmap "); mode = C64_Bitmap; }
	else { line.append(mc ? "MultiColor Text " : "Text "); }
	line.append("Screen: $").append_num(screen, 4, 16).append((d011 & 0x20) ? " Bitmap: $" : " Font: $").append_num(chars, 4, 16);
	ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - ImGui::GetTextLineHeightWithSpacing()));
	ImGui::Text(line.c_str());

	PrintHoverInfo(cpu, hoverPos, mode, screen, chars, chars&0xe000, d011&0x40, 2);
}

void GfxView::PrintHoverInfo(CPU6510* cpu, int *hoverPos, int mode, uint16_t scrnAddr, uint16_t fontAddr, uint16_t bitmAddr, bool _ecbm, int row) {
	strown<128> line;
	line.sprintf("x: %d, y: %d", hoverPos[0], hoverPos[1]);
	if (mode == C64_Sprites) {
		int s = (hoverPos[0] / 24) + (hoverPos[1] / 21) * columns_sprite;
		int sa = scrnAddr + s * 64;
		line.append(" xs:").append_num(hoverPos[0] / 24, 0, 10).append(" ys:").append_num(hoverPos[1] / 24, 0, 10);
		line.append(" spr:$").append_num((sa / 64) & 0xff, 2, 16).append(" addr:$").append_num(sa, 4, 16);
		hoverGfxAddr = sa;
	} else if (mode == C64_Text) {
		int s = hoverPos[0] / 8 + (hoverPos[1] / 8) * columns;
		int sa = scrnAddr + s;
		line.append(" xc:").append_num(hoverPos[0] / 8, 0, 10).append(" yc:").append_num(hoverPos[1] / 8, 0, 10);
		line.append(" scr:$").append_num(sa, 4, 16).append(" chr:$").append_num(cpu->GetByte(sa), 2, 16);
		hoverScreenAddr = sa;
		hoverGfxAddr = fontAddr + (cpu->GetByte(sa) & (_ecbm ? 0x3f : 0xff)) * 8;
	} else if (mode == C64_Bitmap) {
		int s = hoverPos[0] / 8 + (hoverPos[1] / 8) * columns;
		int ss = scrnAddr + s;
		int sb = bitmAddr + s * 8;
		hoverScreenAddr = ss;
		hoverGfxAddr = sb;
		line.append(" xc:").append_num(hoverPos[0] / 8, 0, 10).append(" yc:").append_num(hoverPos[1] / 8, 0, 10);
		line.append(" scr:$").append_num(ss, 4, 16).append(" bm:$").append_num(sb, 4, 16);
	}
	ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - row * ImGui::GetTextLineHeightWithSpacing()));
	ImGui::Text(line.c_str());
}

void GfxView::Create8bppBitmap(CPU6510* cpu)
{
	// make sure generated bitmap fits in mem
	uint32_t cl = 40, rw = 25;
	if (displaySystem == System::Vic20) {
		cl = v20Columns; rw = v20Rows;
	} else if (displayMode == C64_Sprites) {
		cl = columns_sprite; rw = rows_sprite;
	} else if( displayMode != C64_Current) {
		cl = columns; rw = rows;
	}

	int cellWid = 8, cellHgt = 8;
	if (displayMode == C64_Sprites) {
		cellWid = 24;
		cellHgt = 21;
	}

	size_t bitmapMem = (size_t)cl * (size_t)rw * cellWid * cellHgt * 4;
	if (!bitmap || bitmapMem > bitmapSize) {
		if (bitmap) { free(bitmap); }
		bitmap = (uint8_t*)calloc(1, bitmapMem);
		bitmapSize = bitmapMem;
	}

	int linesHigh = rw * cellHgt;

	uint32_t *d = (uint32_t*)bitmap;
	uint32_t w = cl * cellWid;

	bitmapWidth = w;
	bitmapHeight = linesHigh;

//	uint32_t cw = 8;
//	const uint32_t* pal = c64pal;// (const uint32_t*)c64Cols;

	switch (displayMode) {
		case Planar: CreatePlanarBitmap(cpu, d, linesHigh, w, c64pal); break;
		case Columns: CreateColumnsBitmap(cpu, d, linesHigh, w, c64pal); break;

		case C64_Bitmap: 
			if (color) {
				CreateC64ColorBitmapBitmap(cpu, d, c64pal, addrGfxValue, addrScreenValue, cl, rw);
			} else if (multicolor) {
				CreateC64MulticolorBitmapBitmap(cpu, d, c64pal, addrGfxValue, addrScreenValue, addrColValue, cl, rw); break;
			} else {
				CreateC64BitmapBitmap(cpu, d, c64pal, addrGfxValue, cl, rw); break;
			}
			break;

		case C64_Text:
			if (ecbm) {
				CreateC64ExtBkgTextBitmap(cpu, d, c64pal, addrGfxValue, addrScreenValue, addrColValue, cl, rw, vicColors);
			} else if (color) {
				CreateC64ColorTextBitmap(cpu, d, c64pal, addrGfxValue, addrScreenValue, addrColValue, cl, rw, cpu->GetByte(0xd021) & 0xf);
			} else if (multicolor) {
				CreateC64MulticolorTextBitmap(cpu, d, c64pal, addrGfxValue, addrScreenValue, addrColValue, cl, rw, vicColors);
			} else {
				CreateC64TextBitmap(cpu, d, c64pal, cl, rw); break;
			}
			break;

		case C64_Sprites:
			if (multicolor) {
				CreateC64SpritesMCBitmap(cpu, d, linesHigh, w, c64pal); break;
			} else {
				CreateC64SpritesBitmap(cpu, d, linesHigh, w, c64pal); break;
			}

		case ColumnScreen_MC: CreateC64ColorTextColumns(cpu, d, c64pal, addrGfxValue, addrScreenValue, addrColValue, cl, rw); break;

		case C64_Current: CreateC64CurrentBitmap(cpu, d, c64pal); break;

		case V20_Text:
			CreateV20TextBitmap(cpu, d, vic20pal, v20GfxAddr,
				v20ScreenAddr, v20ColorAddr,
				v20Columns, v20Rows, true);
			break;
	}

	if (!texture) { texture = CreateTexture(); }
	if (texture) {
		SelectTexture(texture);
		UpdateTextureData(cl * cellWid, rw * cellHgt, bitmap);
	}
}

void GfxView::CreatePlanarBitmap(CPU6510* cpu, uint32_t* d, int linesHigh, uint32_t w, const uint32_t* pal)
{
	uint16_t a = addrGfxValue;
	for (int y = 0; y < linesHigh; y++) {
		uint16_t xp = 0;
		for (uint32_t x = 0; x < columns; x++) {
			uint8_t b = cpu->GetByte(a++);
			uint8_t m = 0x80;
			for (int bit = 0; bit < 8; bit++) {
				d[(y)*w + (xp++)] = pal[(b&m) ? 14 : 6];
				m >>= 1;
			}
		}
	}
}

void GfxView::CreateColumnsBitmap(CPU6510* cpu, uint32_t* d, int linesHigh, uint32_t w, const uint32_t* pal)
{
	const uint32_t cw = 8;
	uint16_t a = addrGfxValue;
	for (uint32_t x = 0; x < columns; x++) {
		for (int y = 0; y < linesHigh; y++) {
			int xp = x*cw;
			uint8_t b = cpu->GetByte(a++);
			uint8_t m = 0x80;
			for (uint32_t bit = 0; bit < cw; bit++) {
				d[(y)*w + (xp++)] = pal[(b&m) ? 14 : 6];
				m >>= 1;
			}
		}
	}
}

void GfxView::CreateC64CurrentBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal)
{
	uint16_t vic = (3 ^ (cpu->GetByte(0xdd00) & 3)) * 0x4000;
	uint8_t d018 = cpu->GetByte(0xd018);
	uint8_t d011 = cpu->GetByte(0xd011);
	uint8_t d016 = cpu->GetByte(0xd016);
	uint16_t chars = ( d018 & 0xe) * 0x400 + vic;
	uint16_t screen = (d018 >> 4) * 0x400 + vic;
	bool mc = (d016 & 0x10) ? true : false;

	if (d011 & 0x40) {
		CreateC64ExtBkgTextBitmap(cpu, d, pal, chars, screen, 0xd800, 40, 25, true);
	} else if (d011 & 0x20) {
		if (mc) {
			CreateC64MulticolorBitmapBitmap(cpu, d, pal, chars, screen, 0xd800, 40, 25);
		} else {
			CreateC64ColorBitmapBitmap(cpu, d, pal, chars & 0xe000, screen, 40, 25);
		}
	} else {
		if (mc) {
			CreateC64MulticolorTextBitmap(cpu, d, pal, chars, screen, 0xd800, 40, 25, true);
		} else {
			CreateC64ColorTextBitmap(cpu, d, pal, chars, screen, 0xd800, 40, 25, cpu->GetByte(0xd021)&0xf);
		}
	}

	uint8_t d015 = cpu->GetByte(0xd015); // enable
	uint8_t d010 = cpu->GetByte(0xd010); // hi x
	uint8_t d017 = cpu->GetByte(0xd017); // double width
	uint8_t d01d = cpu->GetByte(0xd01d); // double height
	uint8_t d01c = cpu->GetByte(0xd01c); // multicolor
	uint8_t mcol [3] = { (uint8_t)(cpu->GetByte(0xd025)&0xf), (uint8_t)0, (uint8_t)(cpu->GetByte(0xd026)&0xf) };
	int sw = columns * 8;
	int sh = rows * 8;
	int w = 40 * 8;
	for (int s = 7; s >= 0; --s) {
		uint8_t col = cpu->GetByte(0xd027 + s)&0xf;
		mcol[1] = col;
		if (d015 & (1 << s)) {
			int x = cpu->GetByte(0xd000 + 2 * s) + (d010 & (1 << s) ? 256 : 0) - 24;
			int y = cpu->GetByte(0xd001 + 2 * s) - 50;
			int /*l = 0, r = 0,*/ sy = 0, sx = 0;
			if (d017 & (1 << s)) { sy = 1; }
			if (d01d & (1 << s)) { sx = 1; }
			if (x < sw && x>(-(24<<sx)) && y < sh && y >(-(21<<sy))) {
				bool isMC = !!(d01c & (1 << s));
				uint8_t index = cpu->GetByte(screen + 0x3f8 + s);
				uint16_t sprite = vic + index * 64;
				for (int dy = y, by = y + (21<<sy); dy < by; ++dy) {
					if (dy >= 0 && dy < sh) {
//						uint32_t* ds = d + dy * w;
						uint16_t sr = sprite + 3 * ((dy - y) >> sy);
//						const uint8_t *row = Get6502Mem(sr);
						for (int dx = x, rx = x + (24<<sx); dx < rx; ++dx) {
							if (dx >= 0 && dy < sw) {
								int ox = (dx - x) >> sx;
								uint8_t b = cpu->GetByte(sr + (ox >> 3));
								if (isMC) {
									uint8_t ci = (b >> ((ox^6) & 6)) & 3;
									if (ci) {
										d[dy * w + dx] = pal[mcol[ci - 1]];
									}
								} else {
									if (b & (1 << ((ox^7) & 7))) {
										d[dy * w + dx] = pal[col];
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void GfxView::CreateC64BitmapBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t a, size_t cl, uint32_t rw)
{
	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint32_t* o = d + y * 64 * cl + x * 8;
			for (int h = 0; h < 8; h++) {
				uint8_t b = cpu->GetByte(a++);
				uint8_t m = 0x80;
				for (int bit = 0; bit < 8; bit++) {
					*o = pal[(b&m) ? 14 : 6];
					o++;
					m >>= 1;
				}
				o += (cl-1) * 8;
			}
		}
	}
}

void GfxView::CreateC64ColorBitmapBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t a, uint16_t c, size_t cl, uint32_t rw)
{
	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint32_t* o = d + y * 64 * cl + x * 8;
			uint8_t col = cpu->GetByte(c++);
			for (int h = 0; h < 8; h++) {
				uint8_t b = cpu->GetByte(a++);
				uint8_t m = 0x80;
				for (int bit = 0; bit < 8; bit++) {
					*o = pal[(b&m) ? (col >> 4) : (col & 0xf)];
					o++;
					m >>= 1;
				}
				o += cl*8 - 8;
			}
		}
	}
}

void GfxView::CreateC64ExtBkgTextBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t g, uint16_t a, uint16_t cm, size_t cl, uint32_t rw, bool useVicCol)
{
	uint32_t k[4] = { bg, txt_col[1], txt_col[2], txt_col[3] };
	if (useVicCol) {
		k[0] = cpu->GetByte(0xd021)&0xf;
		k[1] = cpu->GetByte(0xd022)&0xf;
		k[2] = cpu->GetByte(0xd023)&0xf;
		k[3] = cpu->GetByte(0xd024)&0xf;
	}

	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint8_t chr = cpu->GetByte(a++);
			uint32_t bg = pal[k[chr >> 6]];
			uint32_t fg = pal[cpu->GetByte(uint16_t(y * 40 + x + cm)) & 0xf];
			chr &= 0x3f;
			uint16_t cs = g + 8 * chr;
			for (int h = 0; h < 8; h++) {
				uint8_t b;
				if (useRomFont && ((cs >= 0x1000 && cs < 0x2000) || (cs >= 0x9000 && cs <= 0xa000))) {
					b = _aStartupFont[(cs++) & 0x7ff];
				} else {
					b = cpu->GetByte(cs++);
				}
				uint8_t m = 0x80;
				for (int bit = 0; bit < 8; bit++) {
					d[(y * 8 + h)*cl*8 + (x * 8 + bit)] = (b&m) ? fg : bg;
					m >>= 1;
				}
			}
		}
	}
}

void GfxView::CreateC64TextBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, size_t cl, uint32_t rw)
{
	uint16_t a = addrScreenValue;
	uint8_t col0 = bg, col1 = txt_col[0];
	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint8_t chr = cpu->GetByte(a++);
			uint16_t cs = addrGfxValue + 8 * chr;
			for (int h = 0; h < 8; h++) {
				uint8_t b;
				if (useRomFont && ((cs >= 0x1000 && cs < 0x2000) || (cs >= 0x9000 && cs <= 0xa000))) {
					b = _aStartupFont[(cs++) & 0x7ff];
				} else {
					b = cpu->GetByte(cs++);
				}
				uint8_t m = 0x80;
				for (int bit = 0; bit < 8; bit++) {
					d[(y * 8 + h)*cl*8 + (x * 8 + bit)] = pal[(b&m) ? col1 : col0];
					m >>= 1;
				}
			}
		}
	}
}

void GfxView::CreateC64ColorTextBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t g, uint16_t a, uint16_t f, size_t cl, uint32_t rw, uint8_t k)
{
	uint32_t *o = d;
	for (size_t y = 0, ye = rw; y < ye; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint8_t c = cpu->GetByte(f++) & 0xf;
			uint8_t chr = cpu->GetByte(a++);
			uint16_t cs = g + 8 * chr;
			for (int h = 0; h < 8; h++) {
				uint8_t b;
				if (useRomFont && ((cs >= 0x1000 && cs < 0x2000) || (cs >= 0x9000 && cs <= 0xa000))) {
					b = _aStartupFont[(cs++) & 0x7ff];
				} else {
					b = cpu->GetByte(cs++);
				}
				for (int m = 0x80; m; m>>=1) {
					*o++ = pal[(m&b) ? c : k];
				}
				o += cl*8 - 8;
			}
			o -= (cl*8 - 1) * 8;
		}
		o += 56*cl;
	}

}

void GfxView::CreateC64MulticolorTextBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t g, uint16_t a, uint16_t cm, size_t cl, uint32_t rw, bool useVicCol)
{
	uint8_t k[4] = { bg, txt_col[0], txt_col[1], 0 };
	if (useVicCol) {
		k[0] = cpu->GetByte(0xd021) & 0xf;
		k[1] = cpu->GetByte(0xd022) & 0xf;
		k[2] = cpu->GetByte(0xd023) & 0xf;
		k[3] = 0;
	}

	uint32_t *o = d;
	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			k[3] = cpu->GetByte(cm++) & 0xf;
			int mc = k[3] & 0x8;
			k[3] &= 7;
			uint8_t chr = cpu->GetByte(a++);
			uint16_t cs = g + 8 * chr;
			for (int h = 0; h < 8; h++) {
				uint8_t b = cpu->GetByte(cs++);
				if (mc) {
					for (int bit = 6; bit >= 0; bit -= 2) {
						uint8_t c = k[(b >> bit) & 0x3];
						*o++ = pal[c];
						*o++ = pal[c];
					}
				} else {
					for (int bit = 7; bit >= 0; bit--) {
						*o++ = pal[k[((b >> bit) & 1) ? 3 : 0]];
					}
				}
				o += (cl-1) * 8;
			}
			o -= cl * 64 - 8;
		}
		o += 56 * cl;
	}
}

void GfxView::CreateV20TextBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t g, uint16_t a, uint16_t cm, size_t cl, uint32_t rw, bool useVicCol)
{
	uint8_t k[4] = { bg, txt_col[0], 0, txt_col[1] };
	if (useVicCol) {
		k[0] = cpu->GetByte(0x900f) >> 4;
		k[1] = cpu->GetByte(0x900f) & 0x7;
		k[2] = 0;
		k[3] = cpu->GetByte(0x900e) >> 4;
	}

	uint32_t* o = d;
	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint8_t colRam = cpu->GetByte(cm++) & 0xf;
			k[2] = colRam & 7;
			int mc = colRam & 0x8;
			uint8_t chr = cpu->GetByte(a++);
			uint16_t cs = g + 8 * chr;
			for (int h = 0; h < 8; h++) {
				uint8_t b = cpu->GetByte(cs++);
				if (mc) {
					for (int bit = 6; bit >= 0; bit -= 2) {
						uint8_t c = k[(b >> bit) & 0x3];
						*o++ = pal[c];
						*o++ = pal[c];
					}
				} else {
					for (int bit = 7; bit >= 0; bit--) {
						*o++ = pal[k[(b&0x80) ? 2 : 0]];
						b <<= 1;
					}
				}
				o += (cl - 1) * 8;
			}
			o -= cl * 64 - 8;
		}
		o += 56 * cl;
	}
}

void GfxView::CreateC64MulticolorBitmapBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t a, uint16_t s, uint16_t cm, size_t cl, uint32_t rw)
{
	uint8_t k = cpu->GetByte(0xd021) & 15;
	for (size_t y = 0; y < rw; y++) {
		for (size_t x = 0; x < cl; x++) {
			uint8_t sc = cpu->GetByte(s++);
			uint8_t fc = cpu->GetByte(cm++);
			for (int h = 0; h < 8; h++) {
				uint8_t b = cpu->GetByte(a++);
				for (int p = 3; p >= 0; p--) {
					uint8_t c = 0;
					switch ((b >> (p << 1)) & 3) {
						case 0: c = k; break;
						case 1: c = (sc >> 4); break;
						case 2: c = (sc & 15); break;
						case 3: c = (fc & 15); break;
					}
					*d++ = pal[c];
					*d++ = pal[c];
				}
				d += cl*8 - 8;
			}
			d -= cl*64 - 8;
		}
		d += cl * 56;
	}
}

void GfxView::CreateC64SpritesBitmap(CPU6510* cpu, uint32_t* d, int linesHigh, uint32_t w, const uint32_t* pal)
{
	uint16_t a = addrGfxValue;
	int sx = w / 24;
	int sy = linesHigh / 21;
	uint32_t col_bg = bg;
	uint32_t col_fg = spr_col[0];
	for (size_t y = 0; y < (size_t)sy; y++) {
		for (size_t x = 0; x < (size_t)sx; x++) {
			for (int l = 0; l < 21; l++) {
				uint32_t *ds = d + (y * 21 + l)*w + x * 24;
				for (int s = 0; s < 3; s++) {
					uint8_t b = cpu->GetByte(a++);
					uint8_t m = 0x80;
					for (int bit = 0; bit < 8; bit++) {
						*ds++ = pal[b&m ? col_fg : col_bg];
						m >>= 1;
					}
				}
			}
			++a;
		}
	}
}

void GfxView::CreateC64SpritesMCBitmap(CPU6510* cpu, uint32_t* d, int linesHigh, uint32_t w, const uint32_t* pal)
{
	uint16_t a = addrGfxValue;
	int sx = w / 24;
	int sy = linesHigh / 21;
	uint32_t cols[4] = { bg, spr_col[1], spr_col[0], spr_col[2] };
	if (vicColors) {
		cols[0] = cpu->GetByte(0xd020)&0xf;
		cols[1] = cpu->GetByte(0xd025)&0xf;
		cols[3] = cpu->GetByte(0xd026)&0xf;
	}
	for (size_t y = 0; y < (size_t)sy; y++) {
		for (size_t x = 0; x < (size_t)sx; x++) {
			for (int l = 0; l < 21; l++) {
				uint32_t* ds = d + (y * 21 + l) * w + x * 24;
				for (int s = 0; s < 3; s++) {
					uint8_t b = cpu->GetByte(a++);
					for (int bit = 0; bit < 4; bit++) {
						uint32_t c = pal[cols[b >> 6]];
						*ds++ = c; *ds++ = c;
						b <<= 2;
					}
				}
			}
			++a;
		}
	}
}

void GfxView::CreateC64ColorTextColumns(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t g, uint16_t a, uint16_t f, size_t cl, uint32_t rw)
{
	uint8_t k[4] = { uint8_t(cpu->GetByte(0xd021) & 0xf), uint8_t(cpu->GetByte(0xd022) & 0xf), uint8_t(cpu->GetByte(0xd023) & 0xf), 0 };
	for (size_t x = 0; x < cl; x++) {
		uint32_t* o = d + x * 8;
		for (size_t y = 0; y < rw; y++) {
			uint8_t chr = cpu->GetByte(a++);
			uint8_t charCol = cpu->GetByte(f++);
			k[3] = charCol & 7;
			uint8_t mc = charCol & 0x8;
			uint16_t cs = g + 8 * chr;
			for (int h = 0; h < 8; h++) {
				uint8_t b = cpu->GetByte(cs++);
				if (mc) {
					for (int bit = 6; bit >= 0; bit -= 2) {
						uint8_t c = k[(b >> bit) & 0x3];
						*o++ = pal[c];
						*o++ = pal[c];
					}
				}
				else {
					for (int bit = 7; bit >= 0; bit--) {
						*o++ = pal[k[((b >> bit) & 1) ? 3 : 0]];
					}
				}
				o += (cl - 1) * 8;
			}
		}
	}
}



GfxView::GfxView() : open(false), reeval(false), color(false), multicolor(false), useRomFont(true), bitmapWidth(0)
{
	addrScreenValue = 0x0400;
	addrGfxValue = 0x1000;
	addrColValue = 0xd800;
	columns = 40;
	rows = 25;

	v20ScreenAddr = 0x1e00;
	v20GfxAddr = 0x8000;
	v20ColorAddr = 0x9600;

	v20Columns = 22;
	v20Rows = 23;
	columns_sprite = 8;
	rows_sprite = 6;
	zoom = Zoom_FitWindow;
	displaySystem = 1;
	displayMode = C64_Current;
	genericMode = Planar - Generic_Modes;
	c64Mode = C64_Current - C64_Modes;
	bitmap = nullptr;
	bitmapSize = 0;
	texture = 0;
	vicColors = false;

	bg = 6;
	spr_col[0] = 14; spr_col[1] = 0; spr_col[2] = 1;
	txt_col[0] = 14; txt_col[1] = 0; txt_col[2] = 1; txt_col[3] = 5;

	SwapSystem();
}
