/*
String User Classes

The MIT License (MIT)

Copyright (c) 2015 Carl-Henrik Skårstedt

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://github.com/sakrac/struse

Add this #define to *one* C++ file before #include "struse.h" to create the implementation:

#define STRUSE_IMPLEMENTATION

// in other words, this sequence should be at the top of one file:
#include ...
#define STRUSE_IMPLEMENTATION
#include "struse.h"
*/

#ifndef __STRUSE_H__
#define __STRUSE_H__

#include <inttypes.h> // uint32_t etc.
#include <string.h> // memcpy, memmove
#include <stdio.h> // printf, vsnprintf
#include <stdarg.h> // va_list

//
// Naming Rules:
//
//  find* returns a position of a string where a match if found or -1 if not
//	skip*/clip* trims beginning or end of string based on condition
//	get_* returns a property, single character or substring
//	before*/after* returns a substring matching the condition
//	is_* returns a bool for a character or whole string test
//	len_* return the number of characters matching the condition from the start of the string
//  *_last indicates that search is done from the end of the string
//	*_rh indicates a rolling hash search is used
//	*_esc indicates that the search string allows for escape codes (\x => x)
//	same_str* is a full string compare and returns true or false based on condtions
//  *_range indicates that the result is filtered by a given set of valid characters
//

typedef unsigned int strl_t;

// helper defines for sprintf/printf with strop
//	example: printf("string is " STROP_FMT "\n", STROP_ARG(strref))
#define STRREF_FMT "%.*s"
#define STRREF_ARG(s) (int)(s).get_len(), (s).get()
#ifdef _WIN32
#define DIR_SEP '\\'
#define NOT_DIR_SEP '/'
#else
#define NOT_DIR_SEP '\\'
#define DIR_SEP '/'
#endif

// internal helper functions for strref
int _find_rh(const char *text, strl_t len, const char *comp, strl_t comp_len);
int _find_rh_case(const char *text, strl_t len, const char *comp, strl_t comp_len);

// strref holds a reference to a constant substring (const char*)
class strref {
protected:
	const char	*string;
	strl_t		length;

public:
	strref() { clear(); }
	strref(const char *str) { set(str); }
	strref(char *str) { set((const char*)str); }
	strref(const char *str, strl_t len) : string(str), length(len) {}
	strref(const char *str, int len) : string(str), length(strl_t(len)) {}
	strref(char *str, int len) : string((const char*)str), length(strl_t(len)) {}
	strref(char *str, strl_t len) : string((const char*)str), length(len) {}

	bool valid() const { return string && length; }
	void clear() { string = nullptr; length = 0; }
	void set(const char *str);
	const char* get() const { return string; }
	const uint8_t* get_u() const { return (const uint8_t*)string; }
	strl_t get_len() const { return length; }
	char get_first() const { return (string && length) ? *string : 0; }
	char get_last() const { return (string && length) ? string[length-1] : 0; }
	char pop_first() { if (length && string) { char c = *string++; --length; return c; } return 0; }

	strl_t limit_pos(strl_t pos) { return pos<length ? pos : length;  }

	bool is_substr(const char *sub) const { return sub>=string && sub<=(string+length); }
	strl_t substr_offs(strref substr) const {
		if (is_substr(substr.get())) { return strl_t(substr.get()-get()); } return 0; }
	strl_t substr_end_offs(strref substr) const {
		if (is_substr(substr.get())) { return strl_t(substr.get()-get()) + substr.get_len(); } return 0; }
	bool is_empty() const { return length==0; }

	// get fnv1a hash for string
	unsigned int fnv1a(unsigned int seed = 2166136261) const;
	unsigned int fnv1a_lower(unsigned int seed = 2166136261) const;
	unsigned int fnv1a_append(unsigned int base_fnv1a_hash) const { return fnv1a(base_fnv1a_hash); }
	unsigned short fnv1a_16( unsigned int seed = 2166136261 ) const;
	uint64_t fnv1a_64(uint64_t seed = 14695981039346656037ULL) const;

    // whitespace ignore fnv1a (any sequence whitespace is replaced by one space)
    unsigned int fnv1a_ws(unsigned int seed = 2166136261) const;

	// convert string to basic integer
	int64_t atoi() const;
	uint64_t atoui() const;

	// convert string to floating point
	float atof() const;
	double atod() const;

	// number of characters of basic integer in string
	int atoi_skip();

	// convert hexadecimal string to signed integer
	int ahextoi() const;

	// convert hexadecimal string to unsigned integer
	size_t ahextoui() const;
	uint64_t ahextou64() const;
	size_t ahextoui_skip();
	size_t abinarytoui_skip();

	// output string with newline (printf)
	void writeln();

	// single digit number
	static char num_to_char(uint8_t num) { return num<10 ? (num+'0'):(num+'a'-10); }

	// is character empty such as space, tab, linefeed etc.?
	static bool is_ws(uint8_t c) { return c <= ' '; }
	static bool is_ws(char c) { return (uint8_t)c <= ' '; }

	// is character a number?
	static bool is_number(uint8_t c) { return c>='0' && c<='9'; }
	static bool is_number(char c) { return (uint8_t)c>='0' && (uint8_t)c<='9'; }

	// is character a hexadecimal number?
	static bool is_hex(uint8_t c) { return is_number(c) || (c>='A' && c<='F') || (c>='a' && c<='f'); }
	static bool is_hex(char c) { return is_hex(uint8_t(c)); }

	// is character alphabetic (A-Z or a-z)?
	static bool is_alphabetic(uint8_t c) { return (c>='a' && c<='z') || (c>='A' && c<='Z'); }

	// is character alphabetic or numeric?
	static bool is_alphanumeric(uint8_t c) { return is_number(c) || is_alphabetic(c); }

	// is character valid as part of a label? (num, _, A-Z, a-z)
	static bool is_valid_label(uint8_t c) { return c=='_' || is_alphanumeric(c); }

	// word separators are non-alphanumeric characters except apostrophe.
	static bool is_sep_ws(uint8_t c) { return c!='\'' && !is_alphanumeric(c); }
	static bool is_sep_ws(char c) { return is_sep_ws(uint8_t(c)); }

	// is control character? (!-/, ?-@, [-^, {-~)
	static bool is_control(uint8_t c) { return !is_ws(c) && !is_alphanumeric(c) && c!='_'; }

	// choice of upper/lowercase conversions
	static char tolower(char c);
	static char toupper(char c);
	static char tolower_win(char c);
	static char toupper_win(char c);
	static char tolower_amiga(char c);
	static char toupper_amiga(char c);
	static char tolower_macos(char c);
	static char toupper_macos(char c);
	static size_t tolower_unicode(int c);
	static size_t toupper_unicode(int c);

	// operators
	// strref += int: move string forward (skip)
	void operator+=(const strl_t skip) { if (skip<length) { string += skip; length -= skip; } else clear(); }
	void operator+=(const int skip) { if (strl_t(skip)<length) { string += skip; length -= strl_t(skip); } else clear(); }

	// strref + int: return a string that is moved forward (skip)
	strref operator+(const strl_t skip) { if (skip<length) return strref(string+skip, length-skip); return strref(); }
	strref operator+(const int skip) { if (skip>=0 && strl_t(skip)<length) return strref(string+skip, length-skip); return strref(); }

	// (bool): check if string is valid (0 length strings are invalid)
	operator bool() const { return string && length; }

	// strref++: skip string forward by one character
	strref& operator++() { if (valid()) { string++; length--; } return *this; }

	// strref > strref / strref < strref: greater than and lesser than operators
	bool operator>(const strref o) const;
	bool operator<(const strref o) const;

	// strref[int]: get character at position
	char operator[](strl_t pos) const { return pos<length ? string[pos] : 0; }
	char operator[](int pos) const { return (strl_t)pos<length ? string[pos] : 0; }

	// remove from start or end
	void skip(strl_t count) { if (count<length) { string += count; length -= count; } else clear(); }
	void clip(strl_t count) { if (count<length) { length -= count; } else clear(); }

	// check if there are any potential non-text characters in the string
	bool valid_ascii7() const;

	// fetch a utf-8 character from the beginning of this string
	size_t get_utf8() const;

	// fetch a utf-8 character from the beginnning of this string and move cursor forward to the next utf-8 character
	size_t pop_utf8();

	// return character at position
	char get_at(strl_t pos) const { return string && pos<length ? string[pos] : 0; }
	char get_at(int pos) const { return string && (strl_t)pos<length ? string[pos] : 0; }

	// return uint8_tacter at position
	uint8_t get_u_at(strl_t pos) const { return (uint8_t)get_at(pos); }

	// is there a whitespace at position offs in the string?
	bool whitespace_at(strl_t pos) const { return is_ws(get_u_at(pos)); }

	// count occurrences of char c
	int count_char(char c) const;

	// count characters to end of line
	strl_t len_eol() const;

	// characters start of next line (may be end of string)
	strl_t len_next_line() const;

	// count characters that can be converted to a floating point number
	strl_t len_float_number() const;

	// count characters that can be converted to a hexadecimal number
	strl_t len_hex() const;

	// check if string is a valid floating point number
	bool is_float_number() const { return valid() && len_float_number() == length; }

	// check if matching first char and skip if match
	bool grab_char( char c ) { if(length && string[0]==c) {	length--; string++; return true; } return false; }

	// wildcard search
	strref find_wildcard(const strref wild, strl_t pos = 0, bool case_sensitive = true) const;
	strref next_wildcard(const strref wild, strref prev, bool case_sensitive = true) const {
		return find_wildcard(wild, is_substr(prev.get()) ? (strl_t(prev.get()-get())+1) : 0, case_sensitive); }
	strref wildcard_after(const strref wild, strref prev, bool case_sensitive = true) const {
		return find_wildcard(wild, is_substr(prev.get()) ? strl_t(prev.get()+prev.get_len()-get()) : 0, case_sensitive); }

	// character filter by string, as in a wildcard [] operator
	bool char_matches_ranges(uint8_t c) const;
	bool char_matches_ranges(char c) const { return char_matches_ranges((uint8_t)c); }

	// whole string compare (_case indicates case sensitive)
	bool same_str(const strref str) const;
	bool same_str_case(const strref str) const;
	bool same_str(const char *str) const;
	bool same_str_case(const char *str);

	// whole string compare, treat character same1 and same2 as equal
	bool same_str(const strref str, char same1, char same2) const;
	bool same_str_case(const strref str, char same1, char same2) const;

	// mid string compare
	bool same_substr(const strref str, strl_t pos) const;
	bool same_substr_esc(const strref str, strl_t pos) const;
	bool same_substr_case(const strref str, strl_t pos) const;
	bool same_substr_case_esc(const strref str, strl_t pos) const;
	bool same_substr_case_esc(const strref str, int pos) const { return same_substr_case_esc(str, (strl_t)pos); }
	bool same_substr_case_esc_range(const strref str, const strref range, strl_t pos) const;

	// prefix compare
	strl_t prefix_len(const strref str) const;
	strl_t prefix_len_case(const strref str) const;
	strl_t prefix_len(const char *str) const;
	strl_t prefix_len_case(const char *str) const;
	strl_t prefix_len(const strref str, char same1, char same2) const;
	bool has_prefix(const strref str) const { return prefix_len(str) == str.get_len(); }
	bool has_prefix(const char* str) const { return str[prefix_len(str)]==0; }
	bool is_prefix_of(const strref str) const { return prefix_len(str)==get_len(); }
	bool is_prefix_of(const strref str, char same1, char same2) const {
		return prefix_len(str, same1, same2)==get_len(); }
	bool is_prefix_word(const strref str) const { return prefix_len(str)==get_len() &&
			(str.get_len()==length || !is_alphanumeric((uint8_t)str[length])); }
	bool is_prefix_case_of(const strref str) const { return prefix_len_case(str)==get_len(); }
	bool is_prefix_float_number() const { return len_float_number() > 0; }
	bool grab_prefix( const char* str ) { strl_t p = prefix_len( str ); if( !str[ p ] ) { skip( p ); return true; } return false; }

	// suffix compare
	strl_t suffix_len(const strref str) const;
	strl_t suffix_len_case(const strref str) const;
	bool is_suffix_of(const strref str) const { return suffix_len(str)==get_len(); }
	bool is_suffix_case_of(const strref str) { return suffix_len_case(str)==get_len(); }
	bool has_suffix(const char *str) const { return strref(str).is_suffix_of(*this); }

	// whole word compare (prefix match + next char is whitespace or end of string)
	bool is_word(const strref str) const { return prefix_len(str)==str.get_len() && whitespace_at(str.get_len()); }
	bool is_word_case(const strref str) const { return prefix_len_case(str)==str.get_len() && whitespace_at(str.get_len()); }

	// string search

	// find first index of char c
	int find(char c) const;

	// find first index of char c after pos
	int find_at(char c, strl_t pos) const;

	// find first index of char c after index pos (find_at(c, pos+1))
	int find_after(char c, strl_t pos) const;

	// find first index of char c after index pos or return length for full string
	strl_t find_or_full(char c, strl_t pos) const;
	strl_t find_or_full_esc(char c, strl_t pos) const;

	// find last position of char c
	int find_last(char c) const;

	// find first position of either char c or char d
	int find(char c, char d) const;

	// find last position of either char c or char d
	int find_last(char c, char d) const;

	// find first <b> after last <a> in string
	int find_after_last(char a, char b) const { return find_after(b, (strl_t)(find_last(a)+1)); }
	int find_after_last(char a1, char a2, char b) const {
		int w = find_last(a1, a2)+1; int l = strref(string+w, length-w).find(b); return l>=0?l+w:-1; }

	// return position in this string of the first occurrence of the argument or negative if not found, not case sensitive
	int find(const strref str) const;
	int find(const strref str, strl_t pos) const;	// find first instance after pos
	int find_bookend(const strref str, const strref bookend) const;

	// return position in this string of the first occurrence of the argument or negative if not found, not case sensitive
	int find(const char *str, strl_t pos = 0) const;
	strl_t find_or_full(const char *str) const { int f = find(str); return f < 0 ? length : strl_t(f); }

	// return position in this string of the first occurrence of the argument or negative if not found, case sensitive
	int find_case(const strref str, strl_t pos = 0) const;

	// return position in this string of the first occurrence of the argument or negative if not found, case sensitive
	int find_case(const char *str) const;
	int find_case_esc(const strref str, strl_t pos) const;
	int find_case_esc_range(const strref str, const strref range, strl_t pos) const;
	int find_esc_range(const strref str, const strref range, strl_t pos) const;

	// return position in this string of the last occurrence of the argument or negative if not found, not case sensitive
	int find_last(const strref str) const;
	int find_last_bookend(const strref str, const strref bookend) const;

	// return position in this string of the last occurrence of the argument or negative if not found, not case sensitive
	int find_last(const char *str) const;

	// return position in this string of the last occurrence of the argument or negative if not found, case sensitive
	int find_last_case(const strref str) const;

	// find first instance after pos allowing escape codes in search string
	int find_esc(const strref str, strl_t pos) const;

	// find any char from str in this string at position
	int find_any_char_of(const strref range, strl_t pos = 0) const;

	// find any char from str or char range or char - with backslash prefix
	int find_any_char_or_range(const strref range, strl_t pos = 0) const;
	int find_any_not_in_range(const strref range, strl_t pos = 0) const;

	// find any char from str or char range or char - with backslash prefix
	int find_range_char_within_range(const strref range_find, const strref range_within, strl_t pos = 0) const;

	// counts
	int substr_count(const strref str) const; // count the occurrences of the argument in this string
	int substr_count_bookend(const strref str, const strref bookend) const;
	int substr_case_count(const strref str) const; // count the occurrences of the argument in this string
	int substr_label_case_count(const strref str) const;
	int count_repeat(char c, strl_t pos) const;
	int count_repeat_reverse(char c, strl_t pos) const;
    int count_lines() const;
    int count_lines(strl_t pos) const { return strref(string, pos<length ? pos : length).count_lines(); }
	int count_lines(strref substring) const { return is_substr(substring.get()) ?
		count_lines(strl_t(substring.get()-get())) : -1; }

	strl_t prev_line_pos( strl_t pos );
	strl_t start_line_pos( strl_t pos );
	strl_t end_line_pos( strl_t pos );

	// rolling hash find
	int find_rh(strref str) const { return _find_rh(get(), get_len(), str.get(), str.get_len()); }
	int find_rh_case(strref str) const {
		return _find_rh_case(get(), get_len(), str.get(), str.get_len()); }

	int find_rh(strref str, strl_t pos) const {
		strref sub = *this; sub += pos; return sub.find_rh(str); }

	int find_rh_case(strref str, strl_t pos) const {
		strref sub = *this; sub += pos; return sub.find_rh_case(str); }

	int find_rh_after(strref str, strref prev) const {
		strref sub = *this; if (is_substr(prev.get())) sub.skip(strl_t(prev.get()-get()));
		return sub.find_rh(str); }

	int find_rh_case_after(strref str, strref prev) const {
		strref sub = *this; if (is_substr(prev.get())) sub.skip(strl_t(prev.get()-get()));
		return sub.find_rh_case(str); }

	// whitespace management

	// number of white space characters from start of string
	strl_t len_whitespace() const { if (!valid()) return 0;
		strl_t o = 0; while (o<length && is_ws((uint8_t)string[o])) o++; return o; }

	// number of white space characters from pos
	strl_t len_whitespace(strl_t pos) const {
		strl_t o = pos; while (o<length && is_ws((uint8_t)string[o])) o++; return o-pos; }

	// number of separator or white space characters from pos
	strl_t len_sep_ws(strl_t pos) const {
		strl_t o = pos; while (o<length && is_sep_ws((uint8_t)string[o])) o++; return o-pos; }

	strl_t len_sep_ws(int pos) const { return len_sep_ws((strl_t)pos); }

	// number of non white space characters from start of string
	strl_t len_grayspace() const { if (!valid()) return 0;
		strl_t o = 0; while (o<length && !is_ws((uint8_t)string[o])) o++; return o; }

	// number of non white space characters from pos
	strl_t len_grayspace(strl_t pos) const {
		strl_t o = pos; while (o<length && !is_ws((uint8_t)string[o])) o++; return o-pos; }

	// number of non separating non white space characters from pos
	strl_t len_non_sep_ws(strl_t pos) const {
		strl_t o = pos; while (o<length && !is_sep_ws((uint8_t)string[o])) o++; return o-pos; }

	strl_t len_non_sep_ws(int pos) const { return len_non_sep_ws((strl_t)pos); }

	// move string forward to the first non-whitespace character
	void skip_whitespace() { skip(len_whitespace()); }

	// move string forward to the first whitespace character
	void skip_to_whitespace() { skip(len_grayspace()); }

	// cut white space characters at end of string
	void clip_trailing_whitespace() { if (valid()) {
		const char *e = string+length; while (*--e<=0x20 && length) { length--; } } }

	// remove white space from start and end
	void trim_whitespace() { clip_trailing_whitespace(); skip_whitespace(); }

	strref get_trimmed_ws() const { strref ret = *this;
		ret.clip_trailing_whitespace(); ret.skip_whitespace(); return ret; }

	// find next whitespace or entire string
	strl_t find_whitespace_or_full() const {
		strl_t ws = (strl_t)len_grayspace(); return (ws==length) ? 0 : ws; }

	// find next whitespace or -1
	int find_whitespace() const {
		strl_t ws = len_grayspace(); return (ws==length) ? -1 : (int)ws; }

	// string content
	// count number of alphanumeric characters from p
	strl_t len_alphanumeric(strl_t p = 0) const { if (valid()) {
		const char *s = string+p; strl_t r = length-p; while (is_alphanumeric((uint8_t)*s) && r) { s++; r--; }
		return length-r; } return 0; }

	// length of word consisting of alphanumeric characters
	strl_t len_word() const { return len_alphanumeric(0); }

	// length of string with escape characters
	strl_t len_esc() const;

	// return number of characters that can be a label
	strl_t len_label() const { if (valid()) { const uint8_t *s = get_u(); strl_t r = length;
		while (is_valid_label(*s) && r) { s++; r--; } return length-r; } return 0; }

	// return whether or not this string is a number
	bool is_number() const { if (strl_t r = length) { if (const uint8_t *s = get_u()) {
		while (r) { if (!is_number(*s)) return false; s++; r--; } return true; } } return false; }

	// go to the next word
	void next_word_ws() { skip_whitespace(); skip_to_whitespace(); skip_whitespace(); }

	// go to the end of this word
	void end_word() { strl_t a = len_word(); string += a; length -= a; }

	// get a portion of a string
	strref get_substr(strl_t pos, strl_t len) const { return pos<length ?
		(strref(string+pos, (pos+len)<length?len : (length-pos))) : strref(); }

	strref get_substr(int pos, int len) const { return get_substr((strl_t)pos, (strl_t)len); }
	strref get_substr(int pos, strl_t len) const { return get_substr((strl_t)pos, len); }

	strref get_skipped(strl_t len) const { if (len<length)
		{ return strref(string+len, length-len); } return strref(); }

	// get this strref without leading whitespace
	strref get_skip_ws() const { return get_skipped(len_whitespace()); }

	strref get_clipped(strl_t len) const { return strref(len>0?string:nullptr,
		len>0?(strl_t(len)<length?len:length):0); }

	strref get_word() const { return get_clipped(len_word()); }

	// get a range of characters matching the range
	strref get_range_word(const strref range, strl_t pos = 0) const;
	
	// get the next block of characters separated by whitespace
	strref get_word_ws() const {
		strl_t w = len_whitespace(), g = len_grayspace(w); return get_substr(w, g - w); }

	strref get_valid_json_string() const {
		const uint8_t *s = get_u(); strl_t l = length; while (l) {
		uint8_t c = *s++; if (!(c=='+' || c=='.' || c=='-' || is_number(c) || c>='A'))
		{ break; } l--; } return strref(string, length-l); }

	strref before(char c) const {
		int o = find(c); if (o>=0) return strref(string, o); return strref(); }

	strref before(char c, char d) const {
		int o = find(c, d); if (o>=0) return strref(string, o); return strref(); }

	strref before_or_full(char c) const {
		int o = find(c); if (o>=0) return strref(string, o); return *this; }

	strref before_last(char c) const {
		int o = find_last(c); if (o>=0) return strref(string, o); return strref(); }

	strref before_last(char c, char d) const {
		int o = find_last(c, d); if (o>=0) return strref(string, o); return strref(); }

	strref before_or_full(const strref str) const {
		int o = find(str); if (o<0) return *this; return strref(string, o); }

	strref after_or_full(const strref str) const {
		int o = find(str); if (o<0) return *this; return strref(string+o, length-o); }

	strref after_or_full(char c) const { int o = find(c);
		if (o>=0) { return strref(string+o+1, length-o-1); } return *this; }

	strref after_or_full(char c, char d) const { int o = find(c, d);
		if (o>=0) { return strref(string+o+1, length-o-1); } return *this; }

	strref after(char c) const { int o = find(c);
		if (o>=0) { return strref(string+o+1, length-o-1); } return strref(); }

	strref after_last_or_full(char c) const { int o = find_last(c);
		if (o>=0) { return strref(string+o+1, length-o-1); } return *this; }

	strref after_last_or_full(char c, char d) const {
		int o = find_last(c, d); if (o>=0) { return strref(string+o+1, length-o-1); } return *this; }

	strref after_last(char c) const { int o = find_last(c); if (o>=0)
			{ return strref(string+o+1, length-o-1); } return strref(); }

	strref after_last(char c, char d) const { int o = find_last(c, d); if (o>=0)
		{ return strref(string+o+1, length-o-1); } return strref(); }

	strref get_alphanumeric() const { strref r(*this); r.skip_whitespace();
		if (strl_t l = r.len_alphanumeric()) { return strref(string, l); } return strref(); }
	
	strref get_label() const { return strref(string, len_label()); }
	
	strref before_or_full_case(const strref str) const { int o = find_case(str);
		if (o<0) { return *this; } return strref(string, o); }

	strref after_or_full_case(const strref str) const { int o = find_case(str);
		if (o<0) { return *this; } return strref(string+o, length-o); }

    strref between(char c, char d) { int s = find(c); if (s>=0) { int e = find_after(d, (strl_t)s);
        if (e>=0) { return get_substr(strl_t(s+1), strl_t(e-s-1)); } } return strref(); }

	// tokenization
	strref split(strl_t pos);
	strref split(int pos);
	strref split_token(char c);
	strref split_token_any(const strref chars);
	strref split_token_trim(char c);
	strref split_token_any_trim(const strref chars);
	strref split_range(const strref range, strl_t pos=0);
	strref split_range_trim(const strref range, strl_t pos=0);
	strref split_label();
	strref split_lang();
	strref split_num();

	// get a snippet, previous and full current line around a position
	strref get_snippet( strl_t pos );

	// grab a block of text starting with (, [ or { and end with the corresponding number of ), ] or }
	strref scoped_block_skip( bool quotes = false );

	// scoped_block_skip with C style comments
	strl_t scoped_block_comment_len();
	strref scoped_block_comment_skip(bool include = false) { strref ret = split(scoped_block_comment_len()); if (!include) { ++ret; ret.clip(1); } return ret; }

	// check matching characters that are terminated by any character in term or ends
	strl_t match_chars_str(const strref match, const strref term = strref());

	strref get_line() const; // return the current line even if empty,t don't change this line
	strref get_line(strl_t line) const; // return line by index
	strref next_line(); // return the current line even if empty and skip this to line after
	strref line() { strref ret; while (valid() && !ret.valid()) ret = next_line(); return ret; }	// return the current or next valid line skip this to line after
	strref next_token(char c) { int o = find(c); if (o<0) o = (int)get_len(); return split(o); }
	strref token_chunk(char c) const { int o = find(c); if (o<0) return *this; return strref(string, o); }
	void token_skip(const strref chunk) { skip(chunk.length+1); }
	strref find_token(const char *substr, char token) const;
	strref find_token_case(const char *substr, char token) const;
	strref find_token(strref substr, char token) const;
	strref find_token_case(strref substr, char token) const;
	strref within_last(char a, char b) const { int f = find_last(a)+1;
		int l = strref(string+f, length-f).find(b); if (l<0) l = 0; return strref(string+f, l); }
	strref within_last(char a1, char a2, char b) const { int f = find_last(a1, a2)+1;
		int l = strref(string+f, length-f).find(b); if (l<0) l = 0; return strref(string+f, l); }

	strref get_quote_xml() const;
	int find_quoted_xml(char d) const; // returns length up to the delimiter d with xml quotation rules, or -1 if delimiter not found
	int find_quoted(char d) const; // returns length up to the delimiter d with c/c++ quotation rules, or -1 if delimiter not found

	strref next_chunk_xml(char open, char close) const { int s = find_quoted_xml(open);
		if (s<0) { return strref(); } strref left = get_skipped(strl_t(s+1)); return left.get_clipped(strl_t(left.find_quoted_xml(close))); }
	strref next_chunk_quoted(char open, char close) const { int s = find_quoted(open);
		if (s<0) { return strref(); } strref left = get_skipped(strl_t(s+1)); return left.get_clipped(strl_t(left.find_quoted(close))); }
	void skip_chunk(const strref chunk) { strl_t add = strl_t(chunk.string-string)+chunk.length+1UL;
		if (add<length) { string += add; length -= add; } else { clear(); } }
};

// internal helper functions for strmod
strl_t _strmod_copy(char *string, strl_t cap, const char *str);
strl_t _strmod_copy(char *string, strl_t cap, strref str);
strl_t _strmod_append(char *string, strl_t length, strl_t cap, const char *str);
strl_t _strmod_append(char *string, strl_t length, strl_t cap, strref str);
strl_t _strmod_insert(char *string, strl_t length, strl_t cap, const strref sub, strl_t pos);
strl_t _strmod_utf8_tolower(char *string, strl_t length, strl_t cap);
strl_t _strmod_write_utf8( char *string, strl_t cap, size_t code, strl_t pos );
void _strmod_substrcopy(char *string, strl_t length, strl_t cap, strl_t src, strl_t dst, strl_t chars);
void _strmod_tolower(char *string, strl_t length);
void _strmod_toupper(char *string, strl_t length);
strl_t _strmod_format_insert(char *string, strl_t length, strl_t cap, strl_t pos, strref format, const strref *args);
strl_t _strmod_remove(char *string, strl_t length, char a);
strl_t _strmod_remove(char *string, strl_t length, strl_t start, strl_t len);
strl_t _strmod_exchange(char *string, strl_t length, strl_t cap, strl_t start, strl_t size, const strref insert);
strl_t _strmod_cleanup_path(char *file, strl_t len);
strl_t _strmod_relative_path(char *out, strl_t cap, strref orig, strref target);

// intermediate template class to support writeable string classes. use strown or strovl which inherits from this.
template <class B> class strmod : public B {
	// mirror base class unsafe size operations (doesn't check capacity)
	void add_len_int(strl_t l) { B::add_len_int(l); }
	void sub_len_int(strl_t l) { B::sub_len_int(l); }
	void set_len_int(strl_t l) { B::set_len_int(l); }
	void dec_len_int() { B::dec_len_int(); }
	void inc_len_int() { B::inc_len_int(); }
public:
	strmod() { clear(); }
	operator strref() { return strref(charstr(), len()); }

	// mirror base template class
	strl_t cap() const { return B::cap(); }
	char* charstr() { return B::charstr(); }
	const char* charstr() const { return B::charstr(); }
	strl_t len() const { return B::len(); }

	// get a strref version of this string
	strref get_strref() { return strref(charstr(), len()); }
	strref get_strref() const { return strref(charstr(), len()); }
	strl_t get_len() const { return B::len(); }

	// basic tests and operations
	void clear() { set_len_int(0); }
	bool valid() const { return charstr() && len(); }
	operator bool() const { return valid(); }
	bool empty() const { return !len(); }
	bool full() const { return len() == cap(); }
	const char* get() const { return charstr(); }
	char get_first() const { return (charstr() && len()) ? *charstr() : 0; }
	char get_last() const { return (charstr() && len()) ? charstr()[len()-1] : 0; }
	void copy(strref o) { set_len_int(_strmod_copy(charstr(), cap(), o)); }
    bool is_substr(const char *sub) const { return sub>=charstr() && sub<=(charstr()+len()); }

	// public size operators (checks for capacity)
	strl_t fit_add(strl_t desired) { return (desired+len()) < cap() ? desired : (cap()-len()); }
	bool set_len(strl_t l) { if (l<=cap()) { set_len_int(l); return true; } set_len_int(cap()); return false; }
	void add_len(strl_t l) { add_len_int(fit_add(l)); }

	// offset operators will always return a strref
	strref operator+(const strl_t skip) { if (skip<len()) {
		return strref(charstr()+skip, len()-skip); } return strref(); }
	strref operator+(const int skip) { if (skip>=0 && strl_t(skip)<len()) {
		return strref(charstr()+skip, len()-skip); } return strref(); }

	// get character at position
	char operator[](size_t pos) { return pos<len() ? charstr()[pos] : 0; }

	// mirror strref functionality
	int count_char(char c) const { return get_strref().count_char(c); }
	int len_eol() const { return get_strref().len_eol(); }
	int len_next_line() const { return get_strref().len_next_line(); }
	strl_t len_float_number() const { return get_strref().len_float_number(); }
	bool is_float_number() const { return get_strref().is_float_number(); }

	// get fnv1a hash for string
	unsigned int fnv1a(unsigned int seed = 2166136261) const { return get_strref().fnv1a(seed);  }
	unsigned int fnv1a_append(unsigned int base_fnv1a_hash) const { return get_strref().fnv1a(base_fnv1a_hash); }

	// whole string compare
	bool same_str(const strref str) const { return get_strref().same_str_case(str); }
	bool same_str_case(const strref str) const { return get_strref().same_str_case(str); }
	bool same_str(const strref str, char same1, char same2) const {
		return get_strref().same_str_case(str, same1, same2); }
	bool same_str_case(const strref str, char same1, char same2) const {
		return get_strref().same_str_case(str, same1, same2); }
	bool same_str(const char *str) const { return get_strref().same_str(str); }
	bool same_str_case(const char *str) const { return get_strref().same_str_case(str); }

	// prefix compare
	strl_t prefix_len(const strref str) const { return get_strref().prefix_len(str); }
	strl_t prefix_len(const strref str, char same1, char same2) const {
		return get_strref().prefix_len_case(str, same1, same2); }
	strl_t prefix_len(const char *str) const { return get_strref().prefix_len(str); }
	strl_t prefix_len_case(const strref str) const { return get_strref().prefix_len_case(str); }
	strl_t prefix_len_case(const char *str) const { return get_strref().prefix_len_case(str); }
	bool is_prefix_of(const strref str) const { return get_strref().is_prefix_of(str); }
	bool is_prefix_of(const strref str, char same1, char same2) const {
		return get_strref().is_prefix_of(str, same1, same2); }
	bool is_prefix_word(const strref str) const { return get_strref().is_prefix_word(str); }
	bool is_prefix_case_of(const strref str) const { return get_strref().is_prefix_case_of(str); }
	bool is_prefix_float_number() const { return get_strref().is_prefix_float_number(); }
	bool grab_prefix( const char* str ) { return get_strref().grab_prefix( str ); }

	// whole word compare (prefix match + next char is whitespace or end of string)
	bool is_word(const strref str) const { return get_strref().is_word(str); }
	bool is_word_case(const strref str) const { return get_strref().is_word_case(str); }

	// suffix compare
	strl_t suffix_len(const strref str) const { return get_strref().suffix_len(str); }
	strl_t suffix_len_case(const strref str) const { return get_strref().suffix_len_case(str); }
	bool is_suffix_of(const strref str) const { return get_strref().is_suffix_of(str); }
	bool is_suffix_case_of(const strref str) const { return get_strref().is_suffix_case_of(str); }
	bool has_suffix(const char *str) const { return get_strref().has_suffix(str); }

	// string search
	int find(char c) const { return get_strref().find(c); }
	int find_after(char c, strl_t pos) const { return get_strref().find_after(c, pos); }
	int find_or_full(char c, strl_t pos) const { return get_strref().find_or_full(c, pos); }
	int find_last(char c) const { return get_strref().find_last(c); }
	int find(char c, char d) const { return get_strref().find(c, d); }
	int find_last(char c, char d) const { return get_strref().find_last(c, d); }
	int find_after_last(char a, char b) const { return get_strref().find_after_last(a, b); }
	int find_after_last(char a1, char a2, char b) const { return get_strref().find_after_last(a1, a2, b); }
	int find(const strref str) const { return get_strref().find(str); }
	int find(const strref str, strl_t pos) const { return get_strref().find(str, pos); }
	int find(const char *str, strl_t pos = 0) const { return get_strref().find(str, pos); }
	int find_case(const strref str) const { return get_strref().find_case(str); }
	int find_case(const char *str) const { return get_strref().find_case(str); }
	int find_last(const strref str) const { return get_strref().find_last(str); }
	int find_last(const char *str) const { return get_strref().find_last(str); }
	int find_last_case(const strref str) const { return get_strref().find_last_case(str); }
	int substr_count(const strref p) const { return get_strref().substr_count(p); }

	// rolling hash string search
	int find_rh(strref str) const { return get_strref().find_rh(str); }
	int find_rh_case(strref str) const { return get_strref().find_rh_case(str); }
	int find_rh(strref str, strl_t pos) const { return get_strref().find_rh(str, pos); }
	int find_rh_case(strref str, strl_t pos) const { return get_strref().find_rh_case(str, pos); }
	int find_rh_after(strref str, strref prev) const { return get_strref().find_rh_after(str, prev); }
	int find_rh_case_after(strref str, strref prev) const { return get_strref().find_rh_case_after(str, prev); }

	// whitespace management
	int len_whitespace() const { return get_strref().len_whitespace(); }
	int len_grayspace() const { return get_strref().len_grayspace(); }
	strl_t find_whitespace_or_full() const { return get_strref().find_whitespace_or_full(); }
	int find_whitespace() const { return get_strref().find_whitespace(); }

	// string content
	strl_t len_alphanumeric(strl_t p = 0) const { return get_strref().len_alphanumeric(p); }
	strl_t len_word() const { return get_strref().len_word(); }
	int len_label() const { return get_strref().len_label(); }
	bool is_number() const { return get_strref().is_number(); }

	// get a portion of a string
	strref get_substr(strl_t pos, strl_t length) const { return get_strref().get_substr(pos, length); }
	strref get_skipped(int skip_len) const { return get_strref().get_skipped(skip_len); }
	strref get_clipped(int clip_len) const { return get_strref().get_clipped(clip_len); }
	strref get_word() const { return get_strref().get_word(); }
	strref get_json() const { return get_strref().get_json(); }

	strref before(char c) const { return get_strref().before(c); }
	strref before(char c, char d) const { return get_strref().before(c, d); }
	strref before_last(char c) const { return get_strref().before_last(c); }
	strref before_last(char c, char d) const { return get_strref().before_last(c, d); }
	strref before_or_full(char c) const { return get_strref().before_or_full(c); }
	strref before_or_full(const strref str) const { return get_strref().before_or_full(str); }
	strref after_or_full(const strref str) const { return get_strref().after_or_full(str); }
	strref after_or_full(char c) const { return get_strref().after_or_full(c); }
	strref after(char c) const { return get_strref().after(c); }
	strref after_last_or_full(char c) const { return get_strref().after_last_or_full(c); }
	strref after_last(char c) const { return get_strref().after_last(c); }
	strref get_alphanumeric() const { return get_strref().get_alphanumeric(); }
	strref before_or_full_case(const strref str) const { return get_strref().before_or_full_case(str); }
	strref after_or_full_case(const strref str) const { return get_strref().after_or_full_case(str); }
    strref between(char c, char d) const { return get_strref().between(c, d); }

	// tokenization
	strref line();	// return the current line and skip this to next line
	strref token_chunk(char c) const { return get_strref().token_chunk(c); }
	strref find_token(const char *substr, char token) const { return get_strref().find_token(substr, token); }
	strref find_token_case(const char *substr, char token) const { return get_strref().find_token_case(substr, token); }
	strref find_token(strref substr, char token) const { return get_strref().find_token(substr, token); }
	strref find_token_case(strref substr, char token) const { return get_strref().find_token_case(substr, token); }
	strref within_last(char a, char b) const { return get_strref().within_last(a, b); }
	strref within_last(char a1, char a2, char b) const { return get_strref().within_last(a1, a2, b); }

	strref get_quote_xml() const { return get_strref().get_quote_xml(); }
	int find_quoted_xml(char d) const { return get_strref().find_quoted_xml(d); }
	int find_quoted(char d) const { return get_strref().find_quoted(d); }

	// wildcard search
	strref find_wildcard(const strref wild, strl_t pos = 0, bool case_sensitive = true) const {
		return get_strref().find_wildcard(wild, pos, case_sensitive); }

	// find a wildcard right after the start of the previous find
	strref next_wildcard(const strref wild, strref prev, bool case_sensitive = true) const {
		return get_strref().next_wildcard(wild, prev, case_sensitive); }

	// find a wildcard right after the end of the previous find
	strref wildcard_after(const strref wild, strref prev, bool case_sensitive = true) const {
		return get_strref().next_wildcard(wild, prev, case_sensitive); }

	// write a single utf-8 character to pos and return how many bytes was required
	strl_t write_utf8(int code, strl_t pos) { return _strmod_write_utf8(charstr(), cap(), code, pos); }

	// push a single utf-8 character to the end of this string
	void push_utf8(int code) { add_len_int(write_utf8(code, len())); }

	// get a single utf-8 character
	int get_utf8(strl_t pos) { strl_t skip; return _strmod_read_utf8(charstr(), len(), pos, skip); }

	// get a single utf-8 characer and the number of bytes that was used to store it
	int get_utf8(strl_t pos, strl_t &skip) { return _strmod_read_utf8(charstr(), len(), pos, skip); }

	// choice of uppercasing / lowercasing entire string
	void tolower() { _strmod_tolower(charstr(), len()); }
	void toupper() { _strmod_toupper(charstr(), len()); }
	void tolower_win() { _strmod_tolower_win_ascii(charstr(), len()); }
	void toupper_win() { _strmod_toupper_win_ascii(charstr(), len()); }
	void tolower_amiga() { _strmod_tolower_amiga_ascii(charstr(), len()); }
	void toupper_amiga() { _strmod_toupper_amiga_ascii(charstr(), len()); }
	void tolower_macos() { _strmod_tolower_macos_ascii(charstr(), len()); }
	void toupper_macos() { _strmod_toupper_macos_ascii(charstr(), len()); }
	void tolower_utf8() { set_len_int(_strmod_utf8_tolower(charstr(), len(), cap())); }
	void toupper_utf8() { set_len_int(_strmod_utf8_toupper(charstr(), len(), cap())); }

	// get the end of the current string
	char *end() { return charstr()+len(); }

	// number of characters left until at capacity
	strl_t left() const { return cap()-len(); }

	// remove trailing whitespace
	void clip_trailing_whitespace() { if (valid()) { 
		const char *e = end(); while (*--e<=0x20 && len()) { dec_len_int(); } } }

	// copy a substring internally while checking for overlap
	void substrcopy(strl_t pos, strl_t target, strl_t _length) { 
		_strmod_substrcopy(charstr(), len(), cap(), pos, target, _length); }

	// insert a substring and expand the string to fit it
	bool insert(const strref sub, strl_t pos) { 
		return set_len(_strmod_insert(charstr(), len(), cap(), sub, pos)); }

	// append a substring at the end of this string
	strmod& append(const strref o) { if (o) { strl_t a = fit_add(o.get_len());
		if (a) { memcpy(end(), o.get(), a); add_len_int(a); } } return *this; }

	// append a character at the end of this string
	strmod& append(char c) { if (!full()) { charstr()[len()] = c; inc_len_int(); } return *this; }

	// C#-ish string concatenation
	strmod& operator+(char c) { return append(c); }
	strmod& operator+(const char* str) { return append(str); }
	strmod& operator+(strref str) { return append(str); }

	// C++COut-ish string concatenation
	strmod& operator<<(char c) { return append(c); }
	strmod& operator<<(const char* str) { return append(str); }
	strmod& operator<<(strref str) { return append(str); }

	// append a character repeatedly
	strmod& pad_to(char c, strl_t pos) { if (len() >= pos) set_len_int(pos); else {
		strl_t ol = len(); set_len(pos); for (strl_t p = ol; p < len(); ++p) charstr()[p] = c; } return *this; }

	// prepend this string with a substring
	void prepend(const strref o) { insert(o, 0); }

	// prepend this string with a c string
	void prepend(const char *s) { insert(strref(s), 0); }

	// format this string using {n} notation to index into the args list
	void format(const strref format, const strref *args) {
		set_len_int(_strmod_format_insert(charstr(), 0, cap(), 0, format, args)); }

	// append a formatted string, return the appended part as a strref
	strref format_append(const strref format, const strref *args) { strl_t l = len();
		set_len_int(_strmod_format_insert(charstr(), len(), cap(), len(), format, args));
		return strref(charstr()+l, len()-l); }

	// prepend a formatted string, return the prepend part as a strref
	strref format_prepend(const strref format, const strref *args) { strl_t l = len();
		set_len_int(_strmod_format_insert(charstr(), len(), cap(), 0, format, args)); 
		return strref(charstr(), len()-l); }

	// insert a formatted string
	void format_insert(const strref format, const strref *args, strl_t pos) {
		set_len_int(_strmod_format_insert(charstr(), len(), cap(), pos, format, args)); }

	strmod& append_num(uint32_t num, strl_t size, strl_t radix) {
		strl_t div = 1;
		if(!size) {
			strl_t mul = 9;
			size = 1;
			while( num > mul ) { size++; mul = (mul+1)*10-1; }
		}
		for(strl_t n=1; n<size; ++n) { div *= radix; }
		for(strl_t a=0; a<size; ++a) {
			char v = (num / div) % radix + '0';
			append( v<='9' ? v : (v+'a'-'0'-10) );
			div /= radix;
		}
		return *this;
	}

	// c style sprintf (work around windows _s preference)
#ifdef WIN32
	int sprintf(const char *format, ...) { va_list args; va_start(args, format);
		set_len_int((strl_t)vsnprintf_s(charstr(), cap(), _TRUNCATE, format, args)); va_end(args); return (int)len(); }
	int sprintf_at(strl_t pos, const char *format, ...) { va_list args; va_start(args, format);
		int l = vsnprintf_s(charstr()+pos, cap()-pos, _TRUNCATE, format, args);
        if (l+pos>len()) set_len(l+pos); va_end(args); return l; }
	int sprintf_append(const char *format, ...) { va_list args; va_start(args, format); strl_t l = 0;
		if (len()<cap()) { l = (strl_t)vsnprintf_s(end(), cap()-len(), _TRUNCATE, format, args); va_end(args);
			add_len_int(l);	} return (int)l; }
#else
	int sprintf(const char *format, ...) { va_list args; va_start(args, format);
		set_len_int(vsnprintf(charstr(), cap(), format, args)); va_end(args); return len(); }
	int sprintf_at(strl_t pos, const char *format, ...) { va_list args; va_start(args, format);
		int l = vsnprintf(charstr()+pos, cap()-pos, format, args);
        if (l+pos>len()) set_len(l+pos); va_end(args); return l; }
	int sprintf_append(const char *format, ...) { va_list args; va_start(args, format);
		int l = vsnprintf(end(), cap()-len(), format, args); va_end(args); add_len_int(l); return l; }
#endif
	// replace instances of character c with character d
	strref replace(char c, char d) { if (char *b = charstr()) {
		for (strl_t i = len(); i; i--) { if (*b==c) *b = d; b++; } } return get_strref(); }

	// replace instances of substring a with substring b
	strref replace(const strref a, const strref b) {
		set_len(_strmod_inplace_replace_int(charstr(), len(), cap(), a, b)); return get_strref(); }

	// replace strings bookended by a specific string
	strref replace_bookend(const strref a, const strref b, const strref bookend) { if (len() && get() && a && bookend) {
		set_len(_strmod_inplace_replace_bookend_int(charstr(), len(), cap(), a, b, bookend)); } return get_strref(); }

	// replace a string found within this string with another string
    void exchange(strl_t pos, strl_t size, const strref insert) {
        set_len_int(_strmod_exchange(charstr(), len(), cap(), pos, size, insert)); }
    
    void exchange(const strref original, const strref insert) {
        if (is_substr(original.get())) { exchange(strl_t(original.get()-get()), original.get_len(), insert); } }
            
    // remove a part of this string
	strref remove(strl_t start, strl_t _length) {
		set_len_int(_strmod_remove(charstr(), len(), start, _length)); return get_strref(); }

	// remove all instances of a character from this string
	strref remove(char a) { set_len_int(_strmod_remove(charstr(), len(), a)); return get_strref(); }

	// zero terminate this string and return it
	const char *c_str() { charstr()[len()<cap()?len():(cap()-1)] = 0; return charstr(); }

	// get the end of this string
	char* charend() { return charstr()+len(); }

	// remove a portion of this string
	void erase(strl_t pos, strl_t length) { if (pos<len()) { if ((pos+length)>len()) {
		length = len()-pos; } if (length) { for (strl_t i = 0; i<length; i++)
		charstr()[pos+i] = charstr()[pos+i+length];	} sub_len_int(length); } }

	strmod& cleanup_path() { 
		set_len(_strmod_cleanup_path(charstr(), get_len()));
		return *this;
	}

	// find a relative path between an original folder and a target file path (returns this as a strref)
	strmod& relative_path(strref orig_folder, strref target_path) { 
		set_len(_strmod_relative_path(charstr(), cap(), orig_folder, target_path));
		return *this;
	}
};

template <strl_t S> class strown_base {
	char string[S];
	strl_t  length;
protected:
	void add_len_int(strl_t l) { length += l; } // unsafe add len (size already checked)
	void sub_len_int(strl_t l) { length -= l; } // unsafe sub len (size already checked)
	void set_len_int(strl_t l) { length = l; }
	void dec_len_int() { length--; }
	void inc_len_int() { length++; }
public:
	strl_t cap() const { return S; }
	char* charstr() { return string; }
	const char* charstr() const { return string; }
	strl_t len() const { return length; }
};

class strovl_base {
protected:
	char *string_ptr;
	strl_t string_length;
	strl_t string_space;
	void add_len_int(strl_t l) { string_length += l; } // unsafe add len (size already checked)
	void sub_len_int(strl_t l) { string_length -= l; } // unsafe sub len (size already checked)
	void set_len_int(strl_t l) { string_length = l; }
	void dec_len_int() { string_length--; }
	void inc_len_int() { string_length++; }
public:
	strl_t cap() const { return string_space; }
	strl_t len() const { return string_length; }
	char *charstr() { return string_ptr; }
	const char* charstr() const { return string_ptr; }
	void invalidate() { string_ptr = nullptr; string_space = 0; }
	void set_overlay(char *ptr, strl_t space) { string_ptr = ptr; string_space = space; }
	void set_overlay(char *ptr, strl_t space, strl_t len) {
		string_ptr = ptr; string_space = space; string_length = len; }
};

// owned string class, instance with 'strown<capacity> name'
template <strl_t S> class strown : public strmod<strown_base<S> > {
public:
	strown(const char *s) { strmod<strown_base<S> >::copy(s); }
	explicit strown(strref s) { strmod<strown_base<S> >::copy(s); }
	strown() {}
};

// overlay string class, instance with 'strovl name(char *, size)'
class strovl : public strmod<strovl_base> {
public:
	strovl() { invalidate(); string_length = 0; }
	strovl(char *ptr, strl_t space) { set_overlay(ptr, space); string_length = 0; }
    strovl(char *ptr, strl_t space, strl_t length) { set_overlay(ptr, space); string_length = length; }
};


// helper for relative strings. purpose is for string collections that may need to grow
// by allocating a new buffer and copying. requires calling get(base strref) tp use string.
class strref_rel {
protected:
	strl_t		offset;
	strl_t		length;
public:
	strref_rel() { clear(); }
	strref_rel(const strref_rel &rel) : offset(rel.offset), length(rel.length) {}
	strref_rel(strref orig, strref base) {
		if (base.is_substr(orig.get())) {
			offset = strl_t(orig.get()-base.get()); length = orig.get_len();
		} else
			length = 0;
	}
	strref_rel(const char *str, strl_t len, strref base) {
		if (base.is_substr(str)) {
			offset = strl_t(str-base.get()); length = len;
		} else
			length = 0;
	}

	strref get(strref base) { return strref(base.get() + offset, length); }
	strref get(strovl base) { return strref(base.get() + offset, length); }
	strl_t get_len() { return length; }

	bool valid() const { return length>0; }
	operator bool() const { return valid(); }

	void clear() { length = 0; }
};



// dynamic collection of strings in a single fixed char array
template <strl_t S> class strcol {
	char _buffer[S];
	strl_t end_buf;
	char* push_back_len(char *w, char *e, strl_t len) { while (w<e && len) { *w++ = (len&0x7f) | (len&(~strl_t(0x7f)) ? 0x80 : 0); len >>= 7; } return w; }
	char* push_back_int(const char *s, strl_t l, strl_t o) { char *e = _buffer+S, *w = push_back_len(_buffer+o, e, l); if (strl_t(e-w)<l) return nullptr; memcpy(w, s, l); return w+l; }
	strl_t lim(strl_t pos) const { return pos<end_buf ? pos : end_buf; }
	strl_t lim_len(strl_t pos, strl_t len) const { return (pos+len)<end_buf ? len : (end_buf-pos); }
public:
	strcol() : end_buf(0) { }
	bool empty() const { return end_buf == 0; }
	void clear() const { end_buf = 0; }
	bool end(strl_t curr) const { return curr>=end_buf; }
	bool last(strl_t curr) const { return end(next(curr)); }
	strl_t get_len(strl_t curr) { strl_t o = 0, s = 0; char c; do { c = _buffer[curr++]; o += strl_t(c&0x7f)<<s; s += 7; } while (c<0); return lim_len(c, o); }
	strl_t next(strl_t curr) const { if (curr >= end_buf) return end_buf; strl_t o = 0, s = 0; char c; do { c = _buffer[curr++]; o += strl_t(c&0x7f)<<s; s += 7; } while (c<0); return lim(o+curr); }
	strref get(strl_t curr) const { if (end(curr)) return strref(); strl_t o = 0, s = 0, c; do { c = _buffer[curr++]; o += (c&0x7f)<<s; s += 7; } while (c&0x80); return strref(_buffer+curr, lim_len(curr, o)); }
	strl_t get_index(int i) const { strl_t curr = 0; while (i-- && !end(curr)) { curr = next(curr); } return curr; }
	strref operator[](int i) const { return get(get_index(i)); }
	bool push_back(const strref s) { if (char *w = push_back_int(s.get(), s.get_len(), end_buf)) { end_buf = (strl_t)(w-_buffer); return true; } return false; }
	void erase(strl_t curr) { strl_t n = next(curr); memmove(_buffer+curr, _buffer+n, end_buf-n); end_buf -= n-curr; }

	class iterator {
		strcol *coll;
		strl_t curr;
	public:
		iterator() : coll(nullptr), curr(0) {}
		iterator(strcol &collection, strl_t pos = 0) : coll(&collection), curr(pos) {}
		void operator++() { if (coll) curr = coll->next(curr); else curr = 0; }
		bool operator==(const iterator &i) const { return curr==i.curr && coll==i.coll; }
		bool operator!=(const iterator &i) const { return curr!=i.curr || coll!=i.coll; }
		void erase() { coll->erase(curr); }
		strref operator*() { return coll->get(curr); }
	};
	iterator end() { return iterator(*this, end_buf); }
	iterator begin() { return iterator(*this); }
};

#ifdef STRUSE_IMPLEMENTATION
//#include <math.h>
#include <stdlib.h> // atof

// Windows extended ascii: https://msdn.microsoft.com/en-us/library/9hxt0028(v=vs.80).aspx
// Unicode: http://unicode-table.com/en/#basic-latin
// Mac OS Roman ascii: https://en.wikipedia.org/wiki/Mac_OS_Roman
// Amiga ascii: http://www.amigacoding.com/index.php/AMOSi:ASCII_Table

static const uint8_t _aMacOSRomanHigh_ToLower[0x80] = {
	0x8a, 0x8c, 0x8d, 0x8e, 0x96, 0x9a, 0x9f, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xbe, 0xbf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0x88, 0x8a, 0x9b, 0xcf, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd8, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0x89, 0x90, 0x87,
	0x91, 0x8f, 0x92, 0x94, 0x95, 0x93, 0x97, 0x99,
	0xf0, 0x98, 0x9c, 0x9e, 0x9d, 0xfd, 0xfe, 0xff };

static const uint8_t _aMacOSRomanHigh_ToUpper[0x80] = {
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xe7,
	0xcb, 0xe5, 0x80, 0xcc, 0x81, 0x82, 0x83, 0xe9,
	0xe6, 0xe8, 0xea, 0xed, 0xeb, 0xec, 0x84, 0xee,
	0xf1, 0xef, 0x85, 0xcd, 0xf2, 0xf4, 0xf3, 0x86,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xae, 0xaf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xce,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd9, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };


uint8_t int_tolower_macos_roman_ascii(uint8_t c) {
	if (c>='A' && c<='Z')
		return (uint8_t)(c+'a'-'A');
	if (c>=0x80)
		return _aMacOSRomanHigh_ToLower[c&0x7f];
	return c;
}

uint8_t int_toupper_macos_roman_ascii(uint8_t c) {
	if (c>='a' && c<='z')
		return (uint8_t)(c+'A'-'a');
	if (c>=0x80)
		return _aMacOSRomanHigh_ToUpper[c&0x7f];
	return c;
}

uint8_t int_tolower_amiga_ascii(uint8_t c) {
	if (c>='A' && c<='Z')
		return (uint8_t)(c+'a'-'A');
	if (c>=0xc0 && c<0xe0)
		return (uint8_t)(c+0x20);
	return c;
}

uint8_t int_toupper_amiga_ascii(uint8_t c) {
	if (c>='a' && c<='z')
		return (uint8_t)(c+'A'-'a');
	if (c>=0xe0)
		return (uint8_t)(c-0x20);
	return c;
}

uint8_t int_toupper_win_ascii(uint8_t c) {
	if (c<'a')
		return c;
	if (c<='z')
		return (uint8_t)(c+'A'-'a');
	switch (c) {
		case 0x84:
			return 0x83;
		case 0x86:
			return 0x8f;
		case 0x82:
			return 0x90;
		case 0x91:
			return 0x92;
		case 0x94:
			return 0x99;
		case 0x81:
			return 0x9a;
	}
	return c;
}

uint8_t int_tolower_win_ascii(uint8_t c) {
	if (c<'A')
		return c;
	if (c<='Z')
		return (uint8_t)(c+'a'-'A');
	switch (c) {
		case 0x8e:
			return 0x84;
		case 0x8f:
			return 0x86;
		case 0x90:
			return 0x82;
		case 0x92:
			return 0x91;
		case 0x99:
			return 0x94;
		case 0x9a:
			return 0x81;
	}
	return c;
}

// General lowercase of unicode range
size_t int_tolower_unicode(size_t c)
{
	if (c<'A' || c==0xd7 || c==0x138 || c==0x149)
		return c;
	if (c<='Z')
		return c+'a'-'A';
	if (c<0xc0)
		return c;
	if (c<0xe0 || (c>=0x391 && c<0x3ab) || (c>=0x3d8 && c<0x3f0) || (c>=0x410 && c<0x430))
		return c+0x20;
	if (c<0x100)
		return c;
	if (c<0x178) {
		if (c>0x138 && c<0x149)
			return ((c-1)|1)+1;
		return c | 1;
	}
	if ((c>=0x460 && c<0x482) || (c>=0x48a && c<0x4c0) || (c>=0x4d0 && c<0x530))
		return c | 1;
	if (c>=0x4c1 && c<0x4cf)
		return ((c-1)|1)+1;
	if (c>=0x400 && c<0x410)
		return c+0x50;

	if (c>=0x531 && c<0x556)
		return c+0x30;

	if (c==0x178)
		return 0xff;
	if (c==0x4c0)
		return 0x4cf;

	return c;
}

// General uppercase of unicode range
size_t int_toupper_unicode(size_t c)
{
	if (c<'a' || c==0xd7 || c==0x138 || c==0x149)
		return c;
	if (c<='z')
		return c+'A'-'c';
	if (c<0xe0)
		return c;
	if (c==0xff)
		return 0x178;
	if (c<0x100 || (c>=0x3b1 && c<0x3cb) || (c>=0x3f8 && c<0x410) || (c>=0x430 && c<0x450))
		return c-0x20;
	if (c<0x178) {
		if (c>0x138 && c<0x149)
			return ((c-1)&1UL)+1;
		return c & ~1UL;
	}
	if ((c>=0x460 && c<0x482) || (c>=0x48a && c<0x4c0) || (c>=0x4d0 && c<0x530))
		return c & ~1UL;
	if (c>=0x4c1 && c<0x4cf)
		return ((c-1)&1UL)+1;
	if (c>=0x450 && c<0x460)
		return c-0x50;

	if (c>=0x561 && c<0x586)
		return c-0x30;

	if (c==0x4cf)
		return 0x4c0;

	return c;
}

// english latin lowercase
uint8_t int_tolower_ascii7(uint8_t c)
{
	if (c<='Z' && c>='A')
		return (uint8_t)(c+'a'-'A');
	return c;
}

// english latin uppercase
uint8_t int_toupper_ascii7(uint8_t c)
{
	if (c>='a' && c<='z')
		return (uint8_t)(c+'A'-'a');
	return c;
}

// convert escape codes to characters
// supports: \a, \b, \f, \n, \r, \t, \v, \000, \x00
// any other character is returned as same
static strl_t int_get_esc_code(const uint8_t *buf, strl_t left, uint8_t &code)
{
	strl_t step = 0;
	if (!left)
		return step;
    uint8_t c = *buf++;
	left--;
	step++;
	if (c=='x' && left && strref::is_hex(*buf)) {
		// parse hex char code
		c = 0;
		for (int r = 0; r<2 && left; r++) {
			uint8_t n = *buf++;
			if (!strref::is_hex(n))
				break;
			c = (uint8_t)((c<<4) + n - (n<='9' ? '0' : (n<='F'?('A'-0xA) : ('a'-0xa))));
			step++;
			left--;
		}
	} else if (c>='0' && c<='7') {
		// parse octal char code
		c -= '0';
		for (int r = 0; r<2 && left; r++) {
			uint8_t n = *buf++;
			if (n<'0' || n>'7')
				break;
			c = (uint8_t)(c*8 + n-'0');
			step++;
			left--;
		}
	} else {
		// check for custom escape code symbol
		switch (c) {
			case 'a':
				c = 7;
				break;
			case 'b':
				c = 8;
				break;
			case 'f':
				c = 12;
				break;
			case 'n':
				c = 10;
				break;
			case 'r':
				c = 13;
				break;
			case 't':
				c = 9;
				break;
			case 'v':
				c = 11;
				break;
		}
	}
	code = c;
	return step;
}

// tolower/toupper implementation
char strref::tolower(char c) { return (char)int_tolower_ascii7((uint8_t)c); }
char strref::toupper(char c) { return (char)int_toupper_ascii7((uint8_t)c); }
char strref::tolower_win(char c) { return (char)int_tolower_win_ascii((uint8_t)c); }
char strref::toupper_win(char c) { return (char)int_toupper_win_ascii((uint8_t)c); }
char strref::tolower_amiga(char c) { return (char)int_tolower_amiga_ascii((uint8_t)c); }
char strref::toupper_amiga(char c) { return (char)int_toupper_amiga_ascii((uint8_t)c); }
char strref::tolower_macos(char c) { return (char)int_tolower_macos_roman_ascii((uint8_t)c); }
char strref::toupper_macos(char c) { return (char)int_toupper_macos_roman_ascii((uint8_t)c); }
size_t strref::tolower_unicode(int c) { return int_tolower_unicode((size_t)c); }
size_t strref::toupper_unicode(int c) { return int_toupper_unicode((size_t)c); }

// use printf to print current string on a single line
void strref::writeln()
{
	if (valid()) {
		printf(STRREF_FMT "\n", STRREF_ARG(*this));
	} else
		printf("\n");
}

// construct a strref from const char*
void strref::set(const char *str)
{
	if (!str || !*str) {
		string = nullptr;
		length = 0;
	} else {
		string = str;
		strl_t l = 0;
		while (*str++)
			l++;
		length = l;
	}
}

// get fnv1a hash of a string
unsigned int strref::fnv1a(unsigned int seed) const
{
	unsigned int hash = seed;
	if (string) {
		unsigned const char *scan = (unsigned const char*)string;
		strl_t left = length;
		while (left--)
			hash = (*scan++ ^ hash) * 16777619;
	}
	return hash;
}

// get 16 bit fnv1a hash of a string, this is officially done by xor folding
unsigned short strref::fnv1a_16(unsigned int seed) const
{
	unsigned int hash = fnv1a(seed);
	return (unsigned short)(hash ^ (hash>>16));
}


uint64_t strref::fnv1a_64(uint64_t seed) const
{
	uint64_t hash = seed;
	if (string) {
		unsigned const char *scan = (unsigned const char*)string;
		strl_t left = length;
		while (left--)
			hash = (*scan++ ^ hash) * 1099511628211;
	}
	return hash;
}

// get lowercase fnv1a hash of a string
unsigned int strref::fnv1a_lower(unsigned int seed) const
{
	unsigned const char *scan = (unsigned const char*)string;
	unsigned int hash = seed;
	strl_t left = length;
	while (left--)
		hash = (int_toupper_ascii7(*scan++) ^ hash) * 16777619;
	return hash;
}

// get fnv1a hash of a string and treat any number whitespace as a single space
unsigned int strref::fnv1a_ws(unsigned int seed) const
{
    unsigned const char *scan = (unsigned const char*)string;
    unsigned int hash = seed;
    strl_t left = length;
    while (left--) {
        uint8_t c = *scan++;
        if (c<' ')
            c = ' ';
        hash = (c ^ hash) * 16777619;
        if (c==' ') {
            while (left && *scan<=0x20) {
                left--;
                scan++;
            }
        }
    }
    return hash;
}

// convert numeric string to integer
int64_t strref::atoi() const
{
	if (string) {
		const unsigned char *s = get_u();
		const unsigned char *e = s + length;
		while (s!=e && *s<=0x20) s++;
		if (s<e) {
			int64_t v = 0;
			bool neg = *s=='-';
			if (neg) s++;
			while (s!=e) {
				unsigned char c = *s++;
				if (c<'0' || c>'9') break;
				v = c-'0' + v*10;
			}
			return neg ? -v : v;
		}
	}
	return 0;
}

uint64_t strref::atoui() const
{
	if (string) {
		const unsigned char *s = get_u();
		strl_t left = length;

		while (left && *s<=0x20) { s++; left--; }
		if (left>=2 && *s=='0' && s[1]=='x' ) { return ahextou64(); }

		uint64_t v = 0;
		while (left) {
			unsigned char c = *s++;
			if (c<'0' || c>'9') break;
			v = c-'0' + v*10;
			--left;
		}
		return v;
	}
	return 0;
}

// convert numeric string into floating point value
float strref::atof() const {
	if (string[length]==0)
		return (float)::atof(string);
	strown<64> num(*this);
	return (float)::atof(num.c_str());
}

// convert numeric string into double precision floating point value
double strref::atod() const {
	if (string[length]==0)
		return ::atof(string);
	strown<64> num(*this);
	return ::atof(num.c_str());
}

// convert numeric string to integer and move string forward
int strref::atoi_skip()
{
	const char *scan = string;
	strl_t left = length;
	while (*scan<=0x20 && left) {
		scan++;
		left--;
	}
	if (!left)
		return 0;
	bool neg = false;
	if (*scan=='-') {
		neg = true;
		left--;
	}
	int value = 0;
	while (left) {
		char c = *scan;
		if (c<'0' || c>'9')
			break;
		left--;
		scan++;
		value = c-'0' + value*10;
	}
	string += length-left;
	length = left;
	return neg ? -value : value;
}

// convert a hexadecimal string to an unsigned integer
size_t strref::ahextoui() const
{
	const char *scan = string;
	strl_t left = length;
	while (left && *scan<=0x20) {
		scan++;
		left--;
	}
	if (!left)
		return 0;
	if (left>2 && *scan=='0' && (scan[1]=='x' || scan[1]=='X')) {
		scan += 2;
		left -= 2;
	}
	strl_t hex = 0;
	while (left) {
		char c = *scan++;
		left--;
		if (c>='0' && c<='9')
			hex = (hex<<4) | (c-'0');
		else if (c>='a' && c<='f')
			hex = (hex<<4) | (c-'a'+10);
		else if (c>='A' && c<='F')
			hex = (hex<<4) | (c-'A'+10);
		else
			break;
	}
	return hex;
}

// convert a hexadecimal string to an unsigned integer
uint64_t strref::ahextou64() const
{
	const char *scan = string;
	strl_t left = length;
	while (*scan<=0x20 && left) {
		scan++;
		left--;
	}
	if (!left)
		return 0;
	if (left>2 && *scan=='0' && (scan[1]=='x' || scan[1]=='X')) {
		scan += 2;
		left -= 2;
	}
	uint64_t hex = 0;
	while (left) {
		char c = *scan++;
		left--;
		if (c>='0' && c<='9')
			hex = (hex<<4) | (c-'0');
		else if (c>='a' && c<='f')
			hex = (hex<<4) | (c-'a'+10);
		else if (c>='A' && c<='F')
			hex = (hex<<4) | (c-'A'+10);
		else
			break;
	}
	return hex;
}
// convert a hexadecimal string to an unsigned integer
size_t strref::ahextoui_skip()
{
	const char *scan = string;
	strl_t left = length;
	while (left && *scan<=0x20) {
		scan++;
		left--;
	}
	if (!left)
		return 0;
	if (left>2 && *scan=='0' && (scan[1]=='x' || scan[1]=='X')) {
		scan += 2;
		left -= 2;
	}
	strl_t hex = 0;
	while (left) {
		char c = *scan;
		if (c>='0' && c<='9')
			hex = (hex<<4) | (c-'0');
		else if (c>='a' && c<='f')
			hex = (hex<<4) | (c-'a'+10);
		else if (c>='A' && c<='F')
			hex = (hex<<4) | (c-'A'+10);
		else
			break;
		scan++;
		left--;
	}
	length -= strl_t(scan-string);
	string = scan;
	return hex;
}

// convert a binary string to an unsigned integer
size_t strref::abinarytoui_skip()
{
	skip_whitespace();
	const char *scan = string;
	strl_t left = length;
	if (!left)
		return 0;
	strl_t bin = 0;
	while (left) {
		uint8_t c = (uint8_t)*scan;
		if (c<'0' || c>'1')
			break;
		scan++;
		left--;
		bin = (bin<<1) | (c-'0');
	}
	length -= strl_t(scan-string);
	string = scan;
	return bin;
}
// convert a hexadecimal string to a signed integer
int strref::ahextoi() const
{
	const char *scan = string;
	strl_t left = length;
	while (*scan<=0x20 && left) {
		scan++;
		left--;
	}
	if (!left)
		return 0;
	bool neg = *scan=='-';
	if (neg) {
		scan++;
		left--;
	}
	if (left>2 && *scan=='0' && (scan[1]=='x' || scan[1]=='X')) {
		scan += 2;
		left -= 2;
	}
	strl_t hex = 0;
	while (left) {
		char c = *scan++;
		left--;
		if (c>='0' && c<='9')
			hex = (hex<<4) | (c-'0');
		else if (c>='a' && c<='f')
			hex = (hex<<4) | (c-'a'+10);
		else if (c>='A' && c<='F')
			hex = (hex<<4) | (c-'A'+10);
		else
			break;
	}
	return neg ? -(int)hex : (int)hex;
}

// count instances of a character in a string
int strref::count_char(char c) const
{
	strl_t left = length;
	const char *scan = string;
	int count = 0;
	while (left) {
		if (*scan++ == c)
			count++;
		left--;
	}
	return count;
}

// find a character in a string
static int int_find_char(char c, const char *scan, strl_t length)
{
	strl_t left = length;
	while (left) {
		if (*scan++ == c)
			return int(length - left);
		left--;
	}
	return -1;
}

// find a character in a string after pos
int strref::find(char c) const
{
	if (!valid())
		return -1;
	return int_find_char(c, string, length);
}

// find an instance of a char after pos
int strref::find_after(char c, strl_t pos) const
{
	if (length>pos) {
		int o = int_find_char(c, string + pos + 1, length - pos - 1);
		if (o >= 0)
			return int(o + pos + 1);
	}
	return -1;
}

// find an instance of a char at pos or after
int strref::find_at(char c, strl_t pos) const
{
	if (length>pos) {
		int o = int_find_char(c, string + pos, length - pos);
		if (o >= 0)
			return int(o + pos);
	}
	return -1;
}

// find an instance of a char at pos or after or return full string
strl_t strref::find_or_full(char c, strl_t pos) const
{
	if (!string)
		return 0;
	if (pos>=length)
		return length;
	int o = int_find_char(c, string + pos, length - pos);
	if (o >= 0)
		return o + pos;
	return length;
}

// ignore matches that are in escape codes
strl_t strref::find_or_full_esc(char c, strl_t pos) const
{
	if (!string)
		return 0;
	if (pos>=length)
		return length;

	strl_t left = length-pos;
	const char *scan = string+pos;
	while (left) {
		char m = *scan++;
		if (m=='\\' && left) {
			scan++;
			left--;
		} else if (m==c)
			return length-left;
		left--;
	}
	return length;
}

// find last position of character c
int strref::find_last(char c) const
{
	if (length && string) {
		strl_t left = length;
		const char *scan = string + left;
		while (left) {
			if (*--scan == c)
				return int(left - 1);
			left--;
		}
	}
	return -1;
}

// find first position of either c or d
int strref::find(char c, char d) const
{
	strl_t left = length;
	if (const char *scan = string) {
		while (left) {
			char n = *scan++;
			if (n == c || n == d)
				return int(length - left);
			left--;
		}
	}
	return -1;
}

// find last instance of either character c or d
int strref::find_last(char c, char d) const
{
	if (length && string) {
		strl_t left = length - 1;
		const char *scan = string + left;
		while (left) {
			char n = *--scan;
			if (n == c || n == d)
				return int(left - 1);
			left--;
		}
	}
	return -1;
}

// compare a string with a substring case sensitive
static bool int_compare_substr_case(const char *scan, strl_t length, const char *check, strl_t chk_len)
{
	if (length < chk_len)
		return false;
	if (scan==nullptr || check==nullptr)
		return scan==check;
	for (strl_t cl = 0; cl<chk_len; cl++) {
		if (*scan++ != *check++)
			return false;
	}
	return true;
}

// compare a string with a substring case sensitive
static bool int_compare_substr(const uint8_t *scan, strl_t length, const uint8_t *check, strl_t chk_len)
{
    if (length < chk_len)
        return false;
    for (strl_t cl = 0; cl<chk_len; cl++) {
        if (int_tolower_ascii7(*scan++) != int_tolower_ascii7(*check++))
            return false;
    }
    return true;
}

// case sensitive rolling hash find substring
int _find_rh_case(const char *text, strl_t length, const char *comp, strl_t comp_length)
{
	if (length < comp_length)
		return -1;
	size_t roll_hash = 0;
	size_t hash = 0;
	size_t hash_remove = 1;
	const char *rem = text;

	// generate compare hash
	for (strl_t cl = 0; cl<comp_length; cl++) {
		hash = hash * 101 + (size_t)comp[cl];
		hash_remove *= 101;
	}

	// initial buffer
	for (strl_t cl = 0; cl<comp_length; cl++)
		roll_hash = roll_hash*101 + (size_t)*text++;

	// roll
	strl_t left = length - comp_length;
	for (;;) {
		if (roll_hash == hash) {
			// compare!
			if (int_compare_substr_case(text - comp_length, left + comp_length, comp, comp_length))
				return int(length - comp_length - left);
		}
		if (!left)
			break;
		left--;
		roll_hash = roll_hash * 101 + (size_t)*text++ -
			hash_remove * (size_t)*rem++;
	}
	return -1;
}

// case ignore rolling hash find substring
int _find_rh(const uint8_t *text, strl_t length, const uint8_t *comp, strl_t comp_length)
{
	if (length < comp_length)
		return -1;

	size_t roll_hash = 0;
	size_t hash = 0;
	size_t hash_remove = 1;
	const uint8_t *rem = text;

	// generate compare hash
	for (strl_t cl = 0; cl<comp_length; cl++) {
		hash = hash*101 + int_tolower_ascii7(comp[cl]);
		hash_remove *= 101;
	}

	// initial buffer
	for (strl_t cl = 0; cl<comp_length; cl++)
		roll_hash = roll_hash*101 + int_tolower_ascii7(*text++);

	// roll
	strl_t left = length - comp_length;
	for (;;) {
		if (roll_hash == hash) {
			// compare!
			if (int_compare_substr(text - comp_length, left + comp_length, comp, comp_length))
				return int(left - length + comp_length);
		}
		if (!left)
			break;
		left--;
		roll_hash = roll_hash*101 + int_tolower_ascii7(*text++) -
			hash_remove * int_tolower_ascii7(*rem++);
	}
	return -1;
}

// calculate number of characters in this line
strl_t strref::len_eol() const
{
	strl_t left = length;
	const char *str = string;
	if (!left || !str)
		return 0;
	while (left && *str!=0x0d && *str!=0x0a) {
		str++;
		left--;
	}

	return length-left;
}

// length of string with escape characters counting as one
strl_t strref::len_esc() const
{
	strl_t len = 0;
	if (const char *str = string) {
		strl_t left = length;
		while (left) {
			left--;
			if (*str=='\\' && left) {
				left--;
				str++;
			}
			str++;
			len++;
		}
	}
	return len;
}


// calculate number of characters to next line
strl_t strref::len_next_line() const
{
	strl_t line = len_eol();
	if (line < length) {
		if (string[line]==0x0d) {
			line++;
			if (line < length && string[line]==0x0a)
				line++;
		} else if (string[line]==0x0a) {
			line++;
			if (string[line]==0x0d)
				line++;
		}
	}
	return line;
}

// compare two strings case ignored
bool strref::same_str(const strref str) const
{
	if (length!=str.length)
		return false;

	const uint8_t *scan = get_u();
	const uint8_t *compare = str.get_u();
	if (scan==nullptr || compare==nullptr)
		return scan==compare;
	strl_t compare_left = length;
	while (compare_left) {
		if (int_tolower_ascii7(*scan++)!=int_tolower_ascii7(*compare++))
			return false;
		compare_left--;
	}
	return true;
}

// mid string compare
bool strref::same_substr(const strref str, strl_t pos) const {
	if ((str.length+pos) > length)
		return false;

	return int_compare_substr(get_u() + pos, length - pos, str.get_u(), str.length);
}

// allow escape codes in search string
bool strref::same_substr_esc(const strref str, strl_t pos) const {
	if (pos >= length)
		return false;

	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = str.get_u();
	strl_t compare_left = str.length;
	while (compare_left) {
		uint8_t c = (uint8_t)*compare++;
		compare_left--;
		if (c=='\\' && compare_left) {
			strl_t skip = int_get_esc_code(compare, compare_left, c);
			compare += skip;
			compare_left -= skip;
		}
		if (int_tolower_ascii7(*scan++) != int_tolower_ascii7(c))
			return false;
	}
	return true;
}

// case sensitive substring compare
bool strref::same_substr_case(const strref str, strl_t pos) const {
	if ((str.length+pos) > length)
		return false;

	return int_compare_substr_case(string + pos, length - pos, str.string, str.length);
}

// allow escape codes in search string
bool strref::same_substr_case_esc(const strref str, strl_t pos) const {
	if (pos >= length)
		return false;

	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = str.get_u();
	strl_t compare_left = str.length;
	while (compare_left) {
		uint8_t c = (uint8_t)*compare++;
		compare_left--;
		if (c=='\\' && compare_left) {
			strl_t skip = int_get_esc_code(compare, compare_left, c);
			compare += skip;
			compare_left -= skip;
		}
		if (*scan++ != c)
			return false;
	}
	return true;
}

// iterate over tokens to find a string that matches substr case ignored
strref strref::find_token(const char *substr, char token) const
{
	strref parse = *this;
	while (strref chunk = parse.token_chunk(token)) {
		if (chunk.same_str(substr))
			return chunk;
		parse.token_skip(chunk);
	}
	return strref();
}

// iterate over tokens to find a string matching substr case ignored
strref strref::find_token(strref substr, char token) const
{
	strref parse = *this;
	while (strref chunk = parse.token_chunk(token)) {
		if (chunk.same_str(substr))
			return chunk;
		parse.token_skip(chunk);
	}
	return strref();
}

// iterate over tokens to find a string matching substr case sensitive
strref strref::find_token_case(const char *substr, char token) const
{
	strref parse = *this;
	while (strref chunk = parse.token_chunk(token)) {
		if (chunk.same_str_case(substr))
			return chunk;
		parse.token_skip(chunk);
	}
	return strref();
}

// iterate over tokens to find a string matching substr case sensitive
strref strref::find_token_case(strref substr, char token) const
{
	strref parse = *this;
	while (strref chunk = parse.token_chunk(token)) {
		if (chunk.same_str_case(substr))
			return chunk;
		parse.token_skip(chunk);
	}
	return strref();
}

// determine if string is greater than other string
bool strref::operator>(const strref o) const
{
	const char *scan = string;
	const char *compare = o.string;
	strl_t left = length>o.length ? o.length : length;
	while (left) {
		char c1 = *scan++;
		char c2 = *compare++;
		if (c1>c2)
			return true;
		else if (c1<c2)
			return false;
		left--;
	}
	return length>o.length;
}

// determine if string is lesser than other string
bool strref::operator<(const strref o) const
{
	const char *scan = string;
	const char *compare = o.string;
	strl_t left = length>o.length ? o.length : length;
	while (left) {
		char c1 = *scan++;
		char c2 = *compare++;
		if (c1<c2)
			return true;
		else if (c1>c2)
			return false;
		left--;
	}
	return length<o.length;
}

// compare two strings case ignored where character same1 is considered equal to same2
bool strref::same_str(const strref str, char same1, char same2) const
{
	if (length!=str.length)
		return false;

	const uint8_t *scan = get_u();
	const uint8_t *compare = str.get_u();
	strl_t left = length;
	while (left) {
		uint8_t c1 = int_tolower_ascii7(*scan++);
		uint8_t c2 = int_tolower_ascii7(*compare++);
		if (c1!=c2 && !(c1==same1 && c2==same2) && !(c1==same2 && c2==same1))
			return false;
		left--;
	}
	return true;
}

// compare two strings case ignored
bool strref::same_str(const char *compare) const
{
	if (!*compare || !valid())
		return false;

	const uint8_t *scan = get_u();
	const uint8_t *cmp = (const uint8_t*)compare;
	strl_t left = length;
	uint8_t c;
	for (;;) {
		c = *cmp++;
		if (c == 0 || !left)
			break;
		if (int_tolower_ascii7(*scan++)!=int_tolower_ascii7(c))
			return false;
		left--;
	}
	return !c && !left;
}

// compare two strings case sensitive
bool strref::same_str_case(const strref str) const
{
	if (length!=str.length)
		return false;

	return int_compare_substr_case(string, length, str.string, str.length);
}

// compare two strings case sensitive where character same1 is considered equal to same2
bool strref::same_str_case(const strref str, char same1, char same2) const
{
	if (length!=str.length)
		return false;

	const char *scan = string;
	const char *compare = str.string;
	strl_t left = length;
	while (left) {
		char c1 = *scan++;
		char c2 = *compare++;
		if (c1!=c2 && !(c1==same1 && c2==same2) && !(c1==same2 && c2==same1))
			return false;
		left--;
	}
	return true;
}

// compare two strings case sensitive
bool strref::same_str_case(const char *str)
{
	if (!*str || !valid())
		return false;

	const char *scan = string;
	strl_t left = length;
	char c;
	for (;;) {
		c = *str++;
		if (c == 0 || !left)
			break;
		if (*scan++!=c)
			return false;
		left--;
	}
	return !c && !left;
}

// count how many characters match from start between two strings
strl_t strref::prefix_len(const strref str) const
{
	const uint8_t *scan = get_u();
	const uint8_t *compare = str.get_u();

	if (!scan || !compare)
		return 0;

	strl_t left = length<str.length ? length : str.length;
	strl_t count = 0;
	while (left) {
		if (int_tolower_ascii7(*scan++)!=int_tolower_ascii7(*compare++))
			break;
		count++;
		left--;
	}
	return count;
}

// count how many characters match from start between two strings where
// character same1 is considered equal to same2
strl_t strref::prefix_len(const strref str, char same1, char same2) const
{
	const uint8_t *scan = get_u();
	const uint8_t *compare = str.get_u();

	strl_t left = length<str.length ? length : str.length;
	strl_t count = 0;
	while (left) {
		uint8_t c1 = int_tolower_ascii7(*scan++);
		uint8_t c2 = int_tolower_ascii7(*compare++);
		if (c1!=c2 && !(c1==same1 && c2==same2) && !(c1==same2 && c2==same1))
			break;
		count++;
		left--;
	}
	return count;
}

// count how many characters match from start between two strings case ignored
strl_t strref::prefix_len(const char *str) const
{
	if (!str)
		return 0;

	const uint8_t *scan = get_u();
	const uint8_t *stru = (const uint8_t*)str;
	strl_t left = length;
	uint8_t c;
	while ((c = *stru++) && left) {
		if (int_tolower_ascii7(*scan++)!=int_tolower_ascii7(c))
			break;
		left--;
	}
	return length-left;
}

// count how many characters match from start between two strings case sensitive
strl_t strref::prefix_len_case(const strref str) const
{
	const uint8_t *scan = get_u();
	const uint8_t *compare = str.get_u();

	strl_t left = length<str.length ? length : str.length;
	strl_t count = 0;
	while (left) {
		if (*scan++!=*compare++)
			break;
		count++;
		left--;
	}
	return count;
}

// count how many characters match from start between two strings case sensitive
strl_t strref::prefix_len_case(const char *str) const
{
	if (!str)
		return 0;
	const char *scan = string;
	strl_t left = length;
	char c;
	while ((c = *str++) && left) {
		if (*scan++!=c)
			break;
		left--;
	}
	return length-left;
}

// count how many characters match from the end between two strings case ignored
strl_t strref::suffix_len(const strref str) const
{
	const uint8_t *scan = get_u()+length;
	const uint8_t *compare = str.get_u()+str.length;

	strl_t left = length<str.length ? length : str.length;
	strl_t count = 0;
	while (left) {
		if (int_tolower_ascii7(*(--scan))!=int_tolower_ascii7(*(--compare)))
			break;
		count++;
		left--;
	}
	return count;
}

// count how many characters match from the end between two strings case sensitive
strl_t strref::suffix_len_case(const strref str) const
{
	const uint8_t *scan = get_u()+length;
	const uint8_t *compare = str.get_u()+str.length;

	strl_t left = length<str.length ? length : str.length;
	strl_t count = 0;
	while (left) {
		if (*(--scan)!=*(--compare))
			break;
		count++;
		left--;
	}
	return count;
}

// find a substring within a string case ignored
int strref::find(const strref str) const
{
	if (!str.valid() || !valid() || length<str.length)
		return -1;

	const uint8_t *scan = get_u();
	strl_t left = length;

	const uint8_t *compare = str.get_u();
	strl_t find_len = str.length;

	uint8_t c = int_tolower_ascii7(*compare++);

	while (left>=find_len) {
		if (int_tolower_ascii7(*scan++)==c) {
			if (int_compare_substr(scan, left - 1, compare, find_len - 1))
				return int(length-left);
		}
		left--;
	}
	return -1;
}

// find a substring within a string case ignored
int strref::find_bookend(const strref str, const strref bookend) const
{
	if (!str.valid() || !valid() || length<str.length)
		return -1;

	const uint8_t *scan = get_u();
	strl_t left = length;

	const uint8_t *compare = str.get_u();
	strl_t find_len = str.length;

	uint8_t p = 0;
	uint8_t c = int_tolower_ascii7(*compare++);

	while (left >= find_len) {
		uint8_t d = int_tolower_ascii7(*scan++);
		if (d == c && (left == length || bookend.char_matches_ranges(p)) &&
			(left == find_len || bookend.char_matches_ranges(int_tolower_ascii7(scan[find_len-1])))) {
			if (int_compare_substr(scan, left - 1, compare, find_len - 1))
				return int(length - left);
		}
		p = d;
		left--;
	}
	return -1;
}
// find a substring within a string case ignored starting at pos
int strref::find(const strref str, strl_t pos) const
{
	if (!str.valid() || !valid() || length<str.length)
		return -1;

	const uint8_t *scan = get_u() + pos;
	strl_t left = length - pos;

	const uint8_t *compare = str.get_u();
	strl_t find_len = str.length;

	uint8_t c = int_tolower_ascii7(*compare++);
	while (left >= find_len) {
		if (int_tolower_ascii7(*scan++) == c) {
			if (int_compare_substr(scan, left - 1, compare, find_len - 1))
				return int(length - left);
		}
		left--;
	}

	return -1;
}

// find case sensitive allow escape codes (\x => x) in search string
int strref::find_esc(const strref str, strl_t pos) const
{
	if (!str.valid() || !valid() || pos>=length)
		return -1;

	// start scan buffer pointers
	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = str.get_u();

	// number of characters left in each buffer
	strl_t scan_left = length - pos;
	strl_t compare_left = str.length;

	// get first character
	uint8_t c = (uint8_t)*compare++;
	compare_left--;
	if (c=='\\' && compare_left) {
		strl_t skip = int_get_esc_code(compare, compare_left, c);
		compare += skip;
		compare_left -= skip;
	}

	// sweep the scan buffer for the matching string
	while (scan_left) {
		if (*scan++ == c) {
			const uint8_t *chk_scan = scan;
			const uint8_t *chk_compare = compare;
			strl_t chk_scan_left = scan_left;
			strl_t chk_compare_left = compare_left;
			while (chk_compare_left) {
				uint8_t d = *chk_compare++;
				chk_compare_left--;
				if (d=='\\' && compare_left) {
					strl_t skip = int_get_esc_code(compare, compare_left, d);
					compare += skip;
					compare_left -= skip;
				}
				if (!chk_scan_left || int_tolower_ascii7(*chk_scan++)!=int_tolower_ascii7(d)) {
					chk_compare_left = 1;
					break;
				}
				chk_scan_left--;
			}
			if (!chk_compare_left)
				return int(length - scan_left);
		}
		scan_left--;
	}
	return -1;
}

// find a substring within a string case ignored
int strref::find(const char *str, strl_t pos) const
{
	if (!str || !valid() || pos>=length)
		return -1;

	const uint8_t *stru = (const uint8_t*)str;
	uint8_t c = int_tolower_ascii7(*stru++);
	if (!c)
		return 0;

	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = stru;

	strl_t l = length - pos;
	while (l) {
		if (int_tolower_ascii7(*scan++)==c) {
			bool equal = true;
			const uint8_t *scan_chk = scan;
			while (uint8_t c2 = *compare++) {
				if (int_tolower_ascii7(*scan_chk++)!=int_tolower_ascii7(c2)) {
					compare = stru;
					equal = false;
					break;
				}
			}
			if (equal)
				return int(length-l);
		}
		l--;
	}
	return -1;
}

// find a substring within a string case sensitive
int strref::find_case(const strref str, strl_t pos) const
{
	if (!str.valid() || !valid() || length<str.length || pos>=length)
		return -1;

	const char *scan = string + pos;
	const char *compare = str.string, *compare_chk = compare;
	strl_t left2 = str.length - pos;

	strl_t left = length;
	while (left>=left2) {
		if (*scan++==*compare_chk) {
			const char *scan_chk = scan;
			compare_chk++;
			while (--left2) {
				if (*scan_chk++!=*compare_chk++) {
					compare_chk = compare;
					left2 = str.length;
					break;
				}
			}
			if (!left2)
				return int(length-left);
		}
		left--;
	}
	return -1;
}

// find case sensitive allow escape codes (\x => x) in search string
int strref::find_case_esc(const strref str, strl_t pos) const
{
	if (!str.valid() || !valid() || pos>=length)
		return -1;

	// start scan buffer pointers
	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = str.get_u();

	// number of characters left in each buffer
	strl_t scan_left = length - pos;
	strl_t compare_left = str.length;

	// get first character
	uint8_t c = *compare++;
	compare_left--;
	if (c=='\\' && compare_left) {
		strl_t skip = int_get_esc_code(compare, compare_left, c);
		compare += skip;
		compare_left -= skip;
	}

	// sweep the scan buffer for the matching string
	while (scan_left) {
		if (*scan++ == c) {
			const uint8_t *chk_scan = scan;
			const uint8_t *chk_compare = compare;
			strl_t chk_scan_left = scan_left;
			strl_t chk_compare_left = compare_left;
			while (chk_compare_left) {
				uint8_t d = *chk_compare++;
				chk_compare_left--;
				if (d=='\\' && chk_compare_left) {
					strl_t skip = int_get_esc_code(chk_compare, chk_compare_left, d);
					chk_compare += skip;
					chk_compare_left -= skip;
				}
				if (!chk_scan_left || *chk_scan++!=d) {
					chk_compare_left = 1;
					break;
				}
				chk_scan_left--;
			}
			if (!chk_compare_left)
				return int(length - scan_left);
		}
		scan_left--;
	}
	return -1;
}

// checks if a range is an exclusion
static strref int_check_exclude(strref range, bool &include)
{
	const char *rng = range.get();
	strl_t rng_left = range.get_len();
	if (rng_left && *rng == '!') {
		include = false;
		return range + 1;
	}
	include = true;
	return range;
}

// checks if character c matches range
static bool int_char_match_range_case(uint8_t c, const uint8_t *rng_chk, strl_t rng_lft)
{
	// no match yet, check skipped character against allowed range
	bool match = false;
	while (rng_lft) {
		uint8_t m = *rng_chk++;
		rng_lft--;
		// escape code?
		if (m == '\\' && rng_lft) {
			strl_t skip = int_get_esc_code(rng_chk, rng_lft, m);
			rng_chk += skip;
			rng_lft -= skip;
		}
		// range?
		if (rng_lft>1 && *rng_chk == '-') {
			rng_chk++;
			rng_lft--;
			uint8_t n = (uint8_t)*rng_chk++;
			rng_lft--;
			// escape code for range end?
			if (n == '\\' && rng_lft) {
				strl_t skip = int_get_esc_code(rng_chk, rng_lft, n);
				rng_chk += skip;
				rng_lft -= skip;
			}
			if (c >= m && c <= n) {
				match = true;
				break;
			}
		}
		else if (c == m) {
			match = true;
			break;
		}
	}
	return match;
}

// checks if character c matches range
static bool int_char_match_range(uint8_t c, const uint8_t *rng_chk, strl_t rng_lft)
{
	// no match yet, check skipped character against allowed range
	bool match = false;
	while (rng_lft) {
		uint8_t m = (uint8_t)*rng_chk++;
		rng_lft--;
		// escape code?
		if (m == '\\' && rng_lft) {
			strl_t skip = int_get_esc_code(rng_chk, rng_lft, m);
			rng_chk += skip;
			rng_lft -= skip;
		}
		// range?
		if (rng_lft>1 && *rng_chk == '-') {
			rng_chk++;
			rng_lft--;
			uint8_t n = (uint8_t)*rng_chk++;
			rng_lft--;
			// escape code for range end?
			if (n == '\\' && rng_lft) {
				strl_t skip = int_get_esc_code(rng_chk, rng_lft, n);
				rng_chk += skip;
				rng_lft -= skip;
			}
			if (c >= int_tolower_ascii7(m) && c <= int_tolower_ascii7(n)) {
				match = true;
				break;
			}
		}
		else if (c == int_tolower_ascii7(m)) {
			match = true;
			break;
		}
	}
	return match;
}

// find case sensitive allow escape codes (\x => x) in search string
int strref::find_case_esc_range(const strref str, const strref range, strl_t pos) const
{
	if (!str.valid() || !valid() || pos>=get_len() || !range.get_len())
		return -1;

	// start scan buffer pointers
	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = str.get_u();

	// number of characters left in each buffer
	strl_t scan_left = length - pos;
	strl_t compare_left = str.length;

	// get first character
	uint8_t c = *compare++;
	compare_left--;
	if (c=='\\' && compare_left) {
		strl_t skip = int_get_esc_code(compare, compare_left, c);
		compare += skip;
		compare_left -= skip;
	}

	// check if range is inclusive or exclusive
	bool include;
	strref rng = int_check_exclude(range, include);

	// sweep the scan buffer for the matching string
	while (scan_left) {
		uint8_t b = (uint8_t)*scan++;
		// check for string match
		if (b == c) {
			const uint8_t *chk_scan = scan;
			const uint8_t *chk_compare = compare;
			strl_t chk_scan_left = scan_left;
			strl_t chk_compare_left = compare_left;
			while (chk_compare_left) {
				uint8_t d = *chk_compare++;
				chk_compare_left--;
				uint8_t e = *chk_scan++;
				chk_scan_left--;
				if (d=='\\' && compare_left) {
					strl_t skip = int_get_esc_code(compare, compare_left, d);
					compare += skip;
					compare_left -= skip;
				}
				if (e!=d) {
					chk_compare_left = 1;
					break;
				}
			}
			if (!chk_compare_left)
				return int(length - scan_left);
		}

		// no match yet, check character against range
		// check if character is allowed
		bool match = int_char_match_range_case(b, rng.get_u(), rng.get_len());
		if ((match && !include) || (!match && include))
			return -1;

		scan_left--;
	}
	return -1;
}

// find substring, allow escape codes (\x => x) in search string
int strref::find_esc_range(const strref str, const strref range, strl_t pos) const
{
	if (!str.valid() || !valid() || pos>=get_len() || !range.get_len())
		return -1;

	// start scan buffer pointers
	const uint8_t *scan = get_u() + pos;
	const uint8_t *compare = str.get_u();

	// number of characters left in each buffer
	strl_t scan_left = length - pos;
	strl_t compare_left = str.length;

	// get first character
	uint8_t c = *compare++;
	compare_left--;
	if (c=='\\' && compare_left) {
		strl_t skip = int_get_esc_code(compare, compare_left, c);
		compare += skip;
		compare_left -= skip;
	}

	// check if range is inclusive or exclusive
	bool include;
	strref rng = int_check_exclude(range, include);

	// sweep the scan buffer for the matching string
	while (scan_left) {
		uint8_t b = int_tolower_ascii7(*scan++);
		// check for string match
		if (b == c) {
			const uint8_t *chk_scan = scan;
			const uint8_t *chk_compare = compare;
			strl_t chk_scan_left = scan_left;
			strl_t chk_compare_left = compare_left;
			while (chk_compare_left) {
				uint8_t d = int_tolower_ascii7(*chk_compare++);
				chk_compare_left--;
				uint8_t e = int_tolower_ascii7(*chk_scan++);
				chk_scan_left--;
				if (d=='\\' && compare_left) {
					strl_t skip = int_get_esc_code(compare, compare_left, d);
					compare += skip;
					compare_left -= skip;
				}
				if (e!=d) {
					chk_compare_left = 1;
					break;
				}
			}
			if (!chk_compare_left)
				return int(length - scan_left);
		}

		// no match yet, check character against range
		bool match = int_char_match_range(b, rng.get_u(), rng.get_len());

		// check if character is allowed
		if ((match && !include) || (!match && include))
			return -1;

		scan_left--;
	}
	return -1;
}

// find a substring within a string case sensitive
int strref::find_case(const char *str) const
{
	if (!str || !valid())
		return -1;

	char c = *str++;
	if (!c)
		return 0;

	const char *scan = string;
	const char *compare = str;

	strl_t left = length;
	while (left) {
		if (*scan++==c) {
			bool equal = true;
			const char *pb = scan;
			while (char c2 = *compare++) {
				if (*pb++!=c2) {
					compare = str;
					equal = false;
					break;
				}
			}
			if (equal)
				return int(length-left);
		}
		left--;
	}
	return -1;
}

// find last matching substring within a string case ignored
int strref::find_last(const strref str) const
{
	if (!str.valid() || !valid() || length<str.length)
		return -1;

	const uint8_t *scan = get_u() + length;
	const uint8_t *compare = str.get_u() + str.length;

	uint8_t c = int_tolower_ascii7(*--compare);

	int left = (int)length;
	while (left>0) {
		left--;
		if (int_tolower_ascii7(*--scan)==c) {
			const uint8_t *scan_chk = scan;
			const uint8_t *cmp_chk = compare;
			strl_t left_check = str.length;
			while (--left_check) {
				if (int_tolower_ascii7(*--scan_chk)!=int_tolower_ascii7(*--cmp_chk)) {
					left_check = 1;
					break;
				}
			}
			if (!left_check)
				return left-(int)str.length+1;
		}
	}
	return -1;
}

// find last matching substring within a string case ignored
int strref::find_last_bookend(const strref str, const strref bookend) const
{
	if (!str.valid() || !valid() || length<str.length)
		return -1;

	const uint8_t *scan = get_u() + length;
	const uint8_t *compare = str.get_u() + str.length;

	uint8_t c = int_tolower_ascii7(*--compare);
	uint8_t p = 0;
	strl_t left = length;
	while (left>0) {
		left--;
		uint8_t d = int_tolower_ascii7(*--scan);
		if (d == c && (left==length || bookend.char_matches_ranges(p))) {
			const uint8_t *scan_chk = scan;
			const uint8_t *cmp_chk = compare;
			strl_t left_check = str.length;
			while (--left_check) {
				if (int_tolower_ascii7(*--scan_chk) != int_tolower_ascii7(*--cmp_chk)) {
					left_check = 1;
					break;
				}
			}
			if (!left_check) {
				if (get_u() == scan_chk || bookend.char_matches_ranges(int_tolower_ascii7(*--scan_chk)))
					return int(left - str.length + 1);
			}
		}
		p = d;
	}
	return -1;
}

// find last matching substring within a string case ignored
int strref::find_last(const char *str) const
{
	if (!str || !*str || !valid())
		return -1;

	const uint8_t *scan = get_u()+length;
	const uint8_t *compare = (const uint8_t*)str + strlen(str);
	uint8_t c = int_tolower_ascii7(*--compare);
	int l = (int)length;
	while (l>=0) {
		l--;
		if (int_tolower_ascii7(*--scan)==c) {
			const uint8_t *scan_chk = scan;
			const uint8_t *cmp_chk = compare;
			while (cmp_chk>(const uint8_t*)str) {
				if (int_tolower_ascii7(*--scan_chk)!=int_tolower_ascii7(*--cmp_chk)) {
					cmp_chk = compare;
					break;
				}
			}
			if (cmp_chk==(const uint8_t*)str)
				return int(scan_chk-(const uint8_t*)string);
		}
	}
	return -1;
}

// find last matching substring within a string case sensitive
int strref::find_last_case(const strref str) const
{
	if (!str.valid() || !valid() || length<str.length)
		return -1;

	const char *scan = string+length-str.length;
	const char *compare = str.string+str.length-1,
		*compare_chk = compare;
	strl_t left_chk = str.length;

	int left = int(length-left_chk);
	while (left>=0) {
		if (*--scan==*compare_chk) {
			const char *scan_chk = scan;
			while (--left_chk) {
				if (*--scan_chk!=*--compare_chk) {
					compare_chk = compare;
					left_chk = str.length;
					break;
				}
			}
			if (!left_chk)
				return int(length-left);
		}
		left--;
	}
	return -1;
}

// count number of matching substrings in string
int strref::substr_count(const strref str) const
{
	if (!str.valid() || !valid() || length<str.length)
		return 0;

	int count = 0;
	const uint8_t *scan = get_u();
	strl_t left = length;
	strl_t substrlen = str.length;
	uint8_t c = int_tolower_ascii7((uint8_t)str.get_first());

	while (left>=substrlen) {
		while (left && int_tolower_ascii7(*scan++)!=c)
			left--;
		if (left && left>=substrlen) {
			// first character matches and enough characters remain for a potential match
			const uint8_t *compare = str.get_u()+1;
			strl_t sr = substrlen-1;
			const uint8_t *scan_chk = scan;
			while (sr && int_tolower_ascii7(*compare++)==int_tolower_ascii7(*scan_chk++))
				sr--;
			if (sr==0) {
				scan = scan_chk;
				left -= substrlen-1;
				count++;
			}
		}
	}
	return count;
}

// count number of matching substrings in string
int strref::substr_count_bookend(const strref str, const strref bookend) const
{
	if (!str.valid() || !valid() || length<str.length)
		return 0;

	int count = 0;
	const uint8_t *scan = get_u();
	strl_t left = length;
	strl_t substrlen = str.length;
	uint8_t c = int_tolower_ascii7((uint8_t)str.get_first());
	uint8_t p = 0;

	while (left >= substrlen) {

		while (left) {
			uint8_t d = int_tolower_ascii7(*scan++);
			if (d == c && bookend.char_matches_ranges(p))
				break;
			p = d;
			left--;
		}
		if (left && left >= substrlen) {
			// first character matches and enough characters remain for a potential match
			const uint8_t *compare = str.get_u() + 1;
			strl_t sr = substrlen - 1;
			const uint8_t *scan_chk = scan;
			while (sr && int_tolower_ascii7(*compare++) == int_tolower_ascii7(*scan_chk++))
				sr--;
			if (sr == 0 && (scan_chk == (get_u() + length) || bookend.char_matches_ranges(int_tolower_ascii7(*scan_chk++)))) {
				scan = scan_chk;
				left -= substrlen - 1;
				count++;
			}
		}
	}
	return count;
}

// count number of matching substrings in string
int strref::substr_case_count(const strref str) const
{
	if (!str.valid() || !valid() || length<str.length)
		return 0;
	
	int count = 0;
	const char *scan = string;
	strl_t left = length;
	strl_t substrlen = str.length;
	char c = str.get_first();
	
	while (left>=substrlen) {
		while (left && *scan++!=c)
			left--;
		if (left && left>=substrlen) {
			// first character matches and enough characters remain for a potential match
			const char *compare = str.string+1;
			strl_t sr = substrlen-1;
			const char *scan_chk = scan;
			while (sr && *compare++==*scan_chk++)
				sr--;
			if (sr==0) {
				scan = scan_chk;
				left -= substrlen-1;
				count++;
			}
		}
	}
	return count;
}

// count number of matching substrings that are bounded by separators and case sensitive in string
int strref::substr_label_case_count(const strref str) const
{
	if (!str.valid() || !valid() || length<str.length)
		return 0;
	
	int count = 0;
	const uint8_t *scan = get_u();
	strl_t left = length;
	strl_t substrlen = str.length;
	uint8_t c = (uint8_t)str.get_first(), p=0;
	
	while (left>=substrlen) {
		while (left) {
			uint8_t d = *scan++;
			if (d==c)
				break;
			left--;
			p = d;
		}
		if (!is_valid_label(p) && left && left>=substrlen) {
			// first character matches and enough characters remain for a potential match
			const char *compare = str.string+1;
			strl_t sr = substrlen-1;
			const uint8_t *scan_chk = scan;
			while (sr && *compare++==*scan_chk++)
				sr--;
			if (sr==0) {
				if (!left || !is_valid_label(*scan_chk)) {
					scan = scan_chk;
					left -= substrlen-1;
					count++;
				}
			}
		}
	}
	return count;
}

// count how many times character c repeats at pos
int strref::count_repeat(char c, strl_t pos) const {
	if (pos>=length)
		return 0;
	const char *scan = string + pos;
	strl_t left = length - pos;
	int count = 0;
	while (left) {
		if (*scan++ != c)
			return count;
		left--;
		count++;
	}
	return count;
}

// count how many time a character repeats backwards at pos
int strref::count_repeat_reverse(char c, strl_t pos) const {
	if (pos>=length)
		return 0;
	const char *scan = string + pos;
	strl_t left = pos;
	int count = 0;
	while (left) {
		if (*scan-- != c)
			return count;
		left--;
		count++;
	}
	return count;
}

// count number of lines with any line ending standard
int strref::count_lines() const {
    const char *scan = string;
    strl_t left = length;
    int count = 0;
    while (left) {
        char c = *scan++;
        left--;
        if (c==0x0a || c==0x0d) {
            count++;
            if (left && ((c==0x0a && *scan==0x0d) || (c==0x0d && *scan==0x0a))) {
                scan++;
                left--;
            }
        }
    }
    return count;
}

strl_t strref::prev_line_pos( strl_t pos )
{
	if( !pos || !length ) { return pos; }

	const char *start = string;
	pos = pos < length ? pos : length;
	--pos;
	char c = start[pos];
	if( c==0x0a || c==0x0d ) --pos;
	if(pos && ((c==0x0a && start[pos]==0x0d) || (c==0x0d && start[pos]==0x0a))) --pos;
	while( pos && start[pos-1]!=0x0a && start[pos-1]!=0x0d ) --pos;
	return pos;
}

strl_t strref::start_line_pos( strl_t pos )
{
	if( !pos || !length ) { return pos; }

	const char *start = string;
	pos = pos < length ? pos : length;
	--pos;
	while( pos && start[pos-1]!=0x0a && start[pos-1]!=0x0d ) --pos;
	return pos;
}

strl_t strref::end_line_pos( strl_t pos )
{
	const char *start = string;
	if( pos > length ) { return length; }
	while( pos < length ) {
		if (start[pos]==0x0a || start[pos] == 0x0d) { return pos; }
		++pos;
	}
	return pos;
}


// find any char from str in this string at position
//	(does not check for escape codes or ranges)
int strref::find_any_char_of(const strref range, strl_t pos) const {
	if (pos>=length)
		return -1;

	const char *scan = get() + pos;
	strl_t left = length-pos;

	const char *rng = range.get();
	strl_t count = range.get_len();

	while (left) {
		char c = *scan++;
		const char *rng_chk = rng;
		for (strl_t n = count; n; --n) {
			if (c == *rng_chk++)
				return int(length-left);
		}
		left--;
	}
	return -1;
}

static int int_find_range(const char *scan, strl_t left, strl_t length, strref rng, bool include)
{
	while (left) {
		uint8_t c = (uint8_t)*scan++;
		bool match = int_char_match_range_case(c, rng.get_u(), rng.get_len());
		if ((match && include) || (!match && !include))
			return int(length - left);
		left--;
	}
	return -1;
}

// find a character matching a range, allow a range of characters using '-'
// such as a-fq0-5 == abcdefq012345 and prefix ! to exclude
int strref::find_any_char_or_range(const strref range, strl_t pos) const {
	if (pos>=length)
		return -1;

	bool include;
	strref rng = int_check_exclude(range, include);

	return int_find_range(string+pos, length-pos, length, rng, include);
}

// find a word made out of characters in the given range
strref strref::get_range_word(const strref range, strl_t pos) const
{
	if (pos >= length)
		return strref();

	bool include;
	strref rng = int_check_exclude(range, include);

	return get_substr(0, (strl_t)int_find_range(string + pos, length - pos, length, rng, !include));
}

int strref::find_any_not_in_range(const strref range, strl_t pos) const {
	if (pos>=length)
		return -1;
	
	if (pos>=length)
		return -1;
	
	bool include;
	strref rng = int_check_exclude(range, include);
	
	return int_find_range(string+pos, length-pos, length, rng, !include);
}

// search of a character in a given range while also checking that
// skipped characters are in another given range.
int strref::find_range_char_within_range(const strref range_find, const strref range_within, strl_t pos) const {
	if (pos>=length)
		return -1;

	const char *scan = get() + pos;
	strl_t l = length-pos;

	// check if range is inclusive or exclusive
	bool include_find;
	strref rng_f = int_check_exclude(range_find, include_find);

	bool include_within;
	strref rng_w = int_check_exclude(range_within, include_within);

	while (l) {
		uint8_t c = (uint8_t)*scan++;
		bool match_find = int_char_match_range_case(c, rng_f.get_u(), rng_f.get_len());
		if ((match_find && include_find) || (!match_find && !include_find))
			return int(length - l);

		// no match yet, check skipped character against allowed range
		bool match = int_char_match_range_case(c, rng_w.get_u(), rng_w.get_len());

		// check if character is allowed
		if ((match && !include_within) || (!match && include_within))
			return -1;
		l--;
	}
	return -1;
}

// check if character matches a given range (this)
bool strref::char_matches_ranges(uint8_t c) const {
	// check if range is inclusive or exclusive
	bool include;
	strref rng = int_check_exclude(*this, include);
	bool match = int_char_match_range_case(c, rng.get_u(), rng.get_len());
	return (match && include) || (!match && !include);
}

// count number of characters matching the range match as long
// as it is terminated by a given character or empty
strl_t strref::match_chars_str(const strref match, const strref term)
{
	bool include_match, include_term;
	strref rng = int_check_exclude(match, include_match);
	strref trm = int_check_exclude(term, include_term);
	strl_t ret = 0;
	const uint8_t *str = get_u();
	strl_t left = length;
	while (left) {
		uint8_t c = *str++;
		if (include_match == int_char_match_range_case(c, rng.get_u(), rng.get_len())) {
			ret++;
		} else if (!term || include_term == int_char_match_range_case(c, trm.get_u(), trm.get_len())) {
			return ret;
		} else
			return 0;
		left--;
	}
	return ret;
}


// wildcard search

#define MAX_WILDCARD_SEGMENTS 64
#define MAX_WILDCARD_STEPS 48
#define MAX_WILDCARD_SEARCH_STACK 32
enum WILDCARD_SEGMENT_TYPE {
	WCST_END,
	WCST_FIND_SUBSTR,
	WCST_FIND_SUBSTR_RANGE,
	WCST_SUBSTR_MATCH_RANGE,
	WCST_FIND_RANGE_CHAR,
	WCST_FIND_RANGE_CHAR_RANGED,
	WCST_FIND_WORD_START,
	WCST_FIND_WORD_START_RANGED,
	WCST_FIND_WORD_END,
	WCST_FIND_WORD_END_RANGED,
	WCST_FIND_LINE_START,
	WCST_FIND_LINE_START_RANGED,
	WCST_FIND_LINE_END,
	WCST_FIND_LINE_END_RANGED,
	WCST_NEXT_SUBSTR,
	WCST_NEXT_ANY_CHAR,
	WCST_NEXT_RANGE_CHAR,
	WCST_NEXT_WORD_START,
	WCST_NEXT_WORD_END,
	WCST_NEXT_LINE_START,
	WCST_NEXT_LINE_END,
};

// ? => any single
// # => any single number
// [] => any single between the brackets
// [-] => any single in the range from character before - to character after
// [!] => any single not between the brackets
// < => start of word
// > => end of word
// @ => start of line
// ^ => end of line
// * => any substring
// *% => any substring excluding whitespace
// *@ => any substring on same line
// *$ => any substring containing alphanumeric ascii characters
// *{} => any substring only containing characters between parenthesis
// *{!} => any substring not containing characters between parenthesis
// \?, \[, \*, etc. => search for character after backslash
// \n, \t, etc. => search for linefeed, tab etc.
//
// words are groups of letters not containing whitespace or separators
//	which are alphanumeric characters plus apostrophe (')

// predefined ranges for wildcard filters
static const strref _no_whitespace_range("!\01- ");
static const strref _no_enter_range("!\n\r");
static const strref _enter_range("\n\r");
static const strref _alphanumeric_range("0-9A-Za-z");
static const strref _numeric_range("0-9");
static const strref _wildcard_control("*?#[<>@^");

// convert wildcard to a set of search steps
static int _build_wildcard_steps(const strref wild, strref *segs, char *type, int &segments) {
	int numSeg = 0;
	int numType = 0;

	// segment separators are: *, ?, [, <
	int pos = 0, last = 0; // current position, last evaluated position
	bool search = true;	// in search mode
	strref range;
	for (;;) {
		// check for number of search segments / number of search steps overflow
		if (numSeg > (MAX_WILDCARD_SEGMENTS-4) || numType > (MAX_WILDCARD_STEPS-2))
			return strref();
		int next_pos = wild.find_any_char_of(_wildcard_control, (strl_t)pos);
		if (next_pos<0) { // completed? (found no wildcard token)
			// add last segment if there was one
			if (wild.get_len() > (strl_t)last) {
				segs[numSeg++] = wild.get_substr((strl_t)last, wild.get_len()-(strl_t)last);
				if (search && range)
					segs[numSeg++] = range;
				type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
			} else if (range) {
				segs[numSeg++] = range;
				type[numType++] = WCST_SUBSTR_MATCH_RANGE;
			}
			range.clear();
			break;
		}
		switch (wild.get_at((strl_t)next_pos)) {
			case '*':	// * => any substring with optional filter
				if (next_pos > last) {
					segs[numSeg++] = wild.get_substr((strl_t)last, (strl_t)(next_pos-last));
					if (search && range)
						segs[numSeg++] = range;
					type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
				}
				last = pos = next_pos+1;
				range.clear();
				// check for substring character filter
				if (strl_t(pos) < wild.get_len()) {
					switch (wild[pos]) {
						case '{': {	// user defined character filter
							int range_end = wild.find_after('}', (strl_t)pos);
							if (range_end > 0) {
								range = wild.get_substr(strl_t(pos+1), strl_t(range_end-pos-1));
								last = pos = range_end + 1;
							} else
								pos++;
							break;
						}
						case '%':	// % => no whitespaces
							range = _no_whitespace_range;
							last = ++pos;
							break;
						case '@':	// @ => no line break
							range = _no_enter_range;
							last = ++pos;
							break;
						case '$':	// $ => only alphanumeric characters
							range = _alphanumeric_range;
							last = ++pos;
							break;
					}
				}
				search = true;
				break;

			case '<': // < = first character of word
				if (next_pos > last) {
					segs[numSeg++] = wild.get_substr((strl_t)last, strl_t(next_pos-last));
					if (search && range)
						segs[numSeg++] = range;
					type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
					search = false;
					range.clear();
				}
				if (search && range)
					segs[numSeg++] = range;
				type[numType++] = (char)(search ? (range ? WCST_FIND_WORD_START_RANGED : WCST_FIND_WORD_START) : WCST_NEXT_WORD_START);
				search = false;
				range.clear();
				pos = last = next_pos+1;
				break;

			case '>': // > = first character after word
				if (next_pos > last) {
					segs[numSeg++] = wild.get_substr(strl_t(last), strl_t(next_pos-last));
					if (search && range)
						segs[numSeg++] = range;
					type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
					search = false;
					range.clear();
				}
				if (search && range)
					segs[numSeg++] = range;
				type[numType++] = (char)(search ? (range ? WCST_FIND_WORD_END_RANGED : WCST_FIND_WORD_END) : WCST_NEXT_WORD_END);
				search = false;
				range.clear();
				pos = last = next_pos+1;
				break;

			case '@': // < = first character of line
				if (next_pos > last) {
					segs[numSeg++] = wild.get_substr(last, next_pos-last);
					if (search && range)
						segs[numSeg++] = range;
					type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
					search = false;
					range.clear();
				}
				type[numType++] = (char)(search ? (range ? WCST_FIND_LINE_START_RANGED : WCST_FIND_LINE_START) : WCST_NEXT_LINE_START);
				search = false;
				range.clear();
				pos = last = next_pos+1;
				break;

			case '^': // > = first character after line
				if (next_pos > last) {
					segs[numSeg++] = wild.get_substr(last, next_pos-last);
					if (search && range)
						segs[numSeg++] = range;
					type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
					search = false;
					range.clear();
				}
				if (search && range)
					segs[numSeg++] = range;
				type[numType++] = (char)(search ? (range ? WCST_FIND_LINE_END_RANGED : WCST_FIND_LINE_END) : WCST_NEXT_LINE_END);
				search = false;
				range.clear();
				pos = last = next_pos+1;
				break;

			case '?':	// ? = any character
				// any character is redundant if currently searching
				if (!search) {
					if (next_pos > last) {
						segs[numSeg++] = wild.get_substr(last, next_pos-last);
						if (search && range)
							segs[numSeg++] = range;
						type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
					}
					range.clear();
					type[numType++] = (char)WCST_NEXT_ANY_CHAR;
					search = false;
					range.clear();
					pos = last = next_pos+1;
				} else
					last = ++pos;
				break;

			case '#':	// # = any number (hard coded range)
				if (next_pos > last) {
					segs[numSeg++] = wild.get_substr(last, next_pos-last);
					type[numType++] = (char)(search ? WCST_FIND_SUBSTR : WCST_NEXT_SUBSTR);
					search = false;
				}
				segs[numSeg++] = _numeric_range;
				type[numType++] = (char)(search ? WCST_FIND_RANGE_CHAR : WCST_NEXT_RANGE_CHAR);
				search = false;
				pos = last = next_pos+1;
				break;

			case '[': {	// [..] = limited range character
				int close_pos = wild.find_after(']', strl_t(next_pos+1));
				if (close_pos>=1) {
					if (next_pos > last) {
						segs[numSeg++] = wild.get_substr(last, next_pos-last);
						if (search && range)
							segs[numSeg++] = range;
						type[numType++] = (char)(search ? (range ? WCST_FIND_SUBSTR_RANGE : WCST_FIND_SUBSTR) : WCST_NEXT_SUBSTR);
						search = false;
						range.clear();
					}
					segs[numSeg++] = wild.get_substr(next_pos+1, close_pos-next_pos-1);
					if (search && range)
						segs[numSeg++] = range;
					type[numType++] = (char)(search ? (range ? WCST_FIND_RANGE_CHAR_RANGED : WCST_FIND_RANGE_CHAR) : WCST_NEXT_RANGE_CHAR);
					search = false;
					range.clear();
					pos = last = close_pos+1;
				} else
					pos = next_pos+1;
				break;
			}
		}
	}
	type[numType++] = WCST_END;
	segments = numSeg;
	return numType;
}

// search for a substring with wildcard rules
strref strref::find_wildcard(const strref wild, strl_t start, bool case_sensitive) const
{
	// collection of sub segments from wildcard
	strref segs[MAX_WILDCARD_SEGMENTS];
	char type[MAX_WILDCARD_STEPS];
	int numSeg = 0;

	// Convert the wildcard to a set of sequential searches and matches
	int numType = _build_wildcard_steps(wild, segs, type, numSeg);

	// start going through the steps to find a match
	int pos = (int)start;
	char last_valid_search_step[MAX_WILDCARD_SEARCH_STACK];
	char last_valid_search_seg[MAX_WILDCARD_SEARCH_STACK];
	strl_t last_valid_search_pos[MAX_WILDCARD_SEARCH_STACK];
	while ((strl_t)pos < length) {
		int first_pos = pos;
		int seg = 0;
		bool valid = true;
		int last_valid_stack = 0;
		int step = 0;
		while (step<numType) {
			bool find = false;
			strl_t found_pos = (strl_t)first_pos;
			int seg_step = seg;
			switch (type[step]) {
				case WCST_END:	// success!
					return strref(string+first_pos, pos-first_pos);

				case WCST_FIND_SUBSTR:
					find = true;
					pos = case_sensitive ? find_case_esc(segs[seg], (strl_t)pos) : find_esc(segs[seg], (strl_t)pos);
					if (pos<0) {
						valid = false;
						break;
					}
					found_pos = (strl_t)pos;
					pos += (int)segs[seg++].len_esc();
					break;

				case WCST_FIND_SUBSTR_RANGE:
					find = true;
					pos = case_sensitive ? find_case_esc_range(segs[seg], segs[seg + 1], (strl_t)pos) : find_esc_range(segs[seg], segs[seg + 1], (strl_t)pos);
					if (pos<0) {
						valid = false;
						break;
					}
					found_pos = (strl_t)pos;
					pos += (int)segs[seg].length;
					seg += 2;
					break;

				case WCST_SUBSTR_MATCH_RANGE:
					valid = false;
					while ((strl_t)pos < length && segs[seg].char_matches_ranges((uint8_t)get_at(pos))) {
						pos++;
						valid = true;
					}
					seg++;
					break;

				case WCST_FIND_RANGE_CHAR:
					find = true;
					pos = find_any_char_or_range(segs[seg++], (strl_t)pos);
					if (pos<0) {
						valid = false;
						break;
					}
					found_pos = (strl_t)pos;
					pos++;
					break;

				case WCST_FIND_RANGE_CHAR_RANGED:
					find = true;
					pos = find_range_char_within_range(segs[seg], segs[seg + 1], (strl_t)pos);
					if (pos<0) {
						valid = false;
						break;
					}
					found_pos = (strl_t)pos;
					pos++;
					seg += 2;
					break;

				case WCST_NEXT_ANY_CHAR:
					valid = pos != (int)length;
					if (!valid)
						break;
					pos++;
					break;

				case WCST_NEXT_RANGE_CHAR:
					valid = !(pos == (int)length || !segs[seg++].char_matches_ranges(get_at(pos)));
					if (!valid)
						break;
					pos++;
					break;

				case WCST_NEXT_SUBSTR:
					valid = same_substr_case_esc(segs[seg], pos);
					if (!valid)
						break;
					pos += (int)segs[seg++].length;
					break;

				case WCST_NEXT_WORD_START:
					valid = !(is_ws(string[pos]) || (pos && !is_ws(string[pos - 1])));
					if (!valid)
						break;
					pos++;
					break;

				case WCST_NEXT_WORD_END:
					valid = (pos==(int)length || is_ws(string[pos])) && pos && !is_ws(string[pos-1]);
					break;

				case WCST_FIND_WORD_START:
					find = true;
					// current position may be ok if first pos or prev=whitespace/separator
					// skip if: current = separator or previous is not separator
					if ((strl_t(pos)<length && is_sep_ws(string[pos])) || (pos && !is_sep_ws(string[pos-1]))) {
						if (strl_t(pos)<length && !is_sep_ws(string[pos]))
							pos += (int)len_non_sep_ws(pos);
						if (strl_t(pos)<length && is_sep_ws(string[pos]))
							pos += (int)len_sep_ws(pos);
					}
					valid = strl_t(pos) < length;
					found_pos = (strl_t)pos;
					break;

				case WCST_FIND_WORD_START_RANGED:
					find = true;
					// current position may be ok if first pos or prev=whitespace/separator
					// skip if: current = separator or previous is not separator
					if (is_sep_ws(string[pos]) || (pos && !is_sep_ws(string[pos-1]))) {
						while (strl_t(pos)<length && !is_sep_ws(string[pos])) {
							valid = segs[seg].char_matches_ranges(string[pos]);
							if (!valid)
								break;
							pos++;
						}
						while (strl_t(pos)<length && is_sep_ws(string[pos])) {
							valid = segs[seg].char_matches_ranges(string[pos]);
							if (!valid)
								break;
							pos++;
						}
					}
                    valid = valid && strl_t(pos) < length;
					found_pos = (strl_t)pos;
					seg++;
					break;

				case WCST_FIND_WORD_END:
					find = true;
					// current position may be ok if first pos or prev=whitespace/separator
					// skip if: current = separator or previous is not separator
					if (!is_sep_ws(string[pos]) || (pos && is_sep_ws(string[pos-1]))) {
						valid = pos != (int)length;
						if (!valid)
							break;
						else {
							if (is_sep_ws(string[pos]))
								pos += (int)len_sep_ws(pos);
							if (!is_sep_ws(string[pos]))
								pos += (int)len_non_sep_ws(pos);
						}
					}
					found_pos = (strl_t)pos;
					break;

				case WCST_FIND_WORD_END_RANGED:
					find = true;
					// current position may be ok if first pos or prev=whitespace/separator
					// skip if: current = separator or previous is not separator
					if (!is_sep_ws(string[pos]) || (pos && is_sep_ws(string[pos-1]))) {
						valid = pos != (int)length;
						if (!valid)
							break;
						else {
							while (strl_t(pos)<length && is_sep_ws(string[pos])) {
								valid = segs[seg].char_matches_ranges(string[pos]);
								if (!valid)
									break;
								pos++;
							}
							while (strl_t(pos)<length && !is_sep_ws(string[pos])) {
								valid = segs[seg].char_matches_ranges(string[pos]);
								if (!valid)
									break;
								pos++;
							}
						}
					}
					found_pos = (strl_t)pos;
					seg++;
					break;

				case WCST_NEXT_LINE_START:
					valid = !(pos && string[pos - 1] != 0xa && string[pos - 1] != 0x0d);
					if (!valid)
						break;
					break;

				case WCST_FIND_LINE_START:
					find = true;
					// current position may be ok if first pos or prev=line sep
					// skip if: current = separator or previous is not separator
					if (pos && string[pos-1]!=0xa && string[pos-1]!=0x0d) {
						while (strl_t(pos)<length && string[pos]!=0x0a && string[pos]!=0x0d)
							pos++;
						if (strl_t(pos)<length) {
							char p = string[pos];
							pos++;
							if (strl_t(pos)<length && ((p==0x0a && string[pos]==0x0d) || (p==0x0d && string[pos]==0x0a)))
								pos++;
						}
						if (strl_t(pos)>=length) {
							valid = false;
							break;
						}
					}
					found_pos = (strl_t)pos;
					break;

				case WCST_FIND_LINE_START_RANGED:
					find = true;
					// current position may be ok if first pos or prev=line sep
					// skip if: current = separator or previous is not separator
					if (pos && string[pos-1]!=0xa && string[pos-1]!=0x0d) {
						while (strl_t(pos)<length && string[pos]!=0x0a && string[pos]!=0x0d) {
							if (!segs[seg].char_matches_ranges(string[pos])) {
								valid = false;
								break;
							}
							pos++;
						}
						if (strl_t(pos)<length) {
							char p = string[pos];
							pos++;
							if (strl_t(pos)<length && ((p==0x0a && string[pos]==0x0d) || (p==0x0d && string[pos]==0x0a)))
								pos++;
						}
						if (strl_t(pos)>=length) {
							valid = false;
							break;
						}
					}
					found_pos = (strl_t)pos;
					seg++;
					break;

				case WCST_NEXT_LINE_END:
					if (strl_t(pos)<length && string[pos]!=0x0a && string[pos]!=0x0d) {
						valid = false;
						break;
					}
					break;

				case WCST_FIND_LINE_END:
					find = true;
					// current position may be ok if first pos or prev=whitespace/separator
					// skip if: current = separator or previous is not separator
					while (strl_t(pos)<length && string[pos]!=0x0a && string[pos]!=0x0d)
						pos++;
					found_pos = true;
					break;
				case WCST_FIND_LINE_END_RANGED:
					find = true;
					// current position may be ok if first pos or prev=whitespace/separator
					// skip if: current = separator or previous is not separator
					// note: end of string is also a valid end of line so make a custom step
					while (strl_t(pos)<length && string[pos]!=0x0a && string[pos]!=0x0d) {
						if (!segs[seg].char_matches_ranges(string[pos])) {
							valid = false;
							break;
						}
						pos++;
					}
					found_pos = (strl_t)pos;
					seg++;
					break;
			}

			// if current step is not valid go to the next character and try again
			if (!valid) {
				if (last_valid_stack) {
					last_valid_stack--;	// step back one level and try again
					step = last_valid_search_step[last_valid_stack];
					seg = last_valid_search_seg[last_valid_stack];
					pos = (int)last_valid_search_pos[last_valid_stack] + 1;
					valid = true;
				}
				else {
					pos = first_pos + 1;
					break;
				}
			} else {
				if (find) {
					if (!step)
						first_pos = (int)found_pos;
					if (last_valid_stack < MAX_WILDCARD_SEARCH_STACK) {
						last_valid_search_pos[last_valid_stack] = found_pos;
						last_valid_search_seg[last_valid_stack] = (char)seg_step;
						last_valid_search_step[last_valid_stack] = (char)step;
						last_valid_stack++;
					}
				}
				step++;
			}
		}
	}
	return strref();
}

// read one utf8 from the start of a string
size_t strref::get_utf8() const
{
	if (!valid())
		return 0;
	const uint8_t *scan = get_u();
	strl_t left = length-1;
	if (left>5)
		left = 5;
	uint8_t f = *scan++;
	size_t c = f, mask = 0x80;
	while ((mask&c) && left) {
		uint8_t n = *scan++;
		c = (c<<6)|(n&0x3f);
		mask <<= 5;
		left--;
	}
	return c;
}

// read one utf8 from the start of a string and move string
// move string forward by the size of the code.
size_t strref::pop_utf8()
{
	if (!valid())
		return 0;
	const uint8_t *scan = get_u();
	strl_t left = length-1;
	if (left>5)
		left = 5;
	uint8_t f = *scan++;
	size_t c = f, m = 0x80;
	while ((m&c) && left) {
		uint8_t n = *scan++;
		c = (c<<6)|(n&0x3f);
		m <<= 5;
		left--;
	}
	length -= strl_t((const char*)scan - string);
	string = (const char*)scan;
	return c;
}

bool strref::valid_ascii7() const
{
	const char *scan = get();
	size_t left = get_len();
	while (left) {
		char c = *scan++;
		if ((c<' ' || c>=127) && c!=0x0a && c!=0x0d && c!=0x09)
			return false;
		left--;
	}
	return true;
}

// find the character d outside of quoted xml text
int strref::find_quoted_xml(char d) const
{
	const char *scan = string;
	strl_t left = length;
	char q = 0;	// quote type is either " or '
	while (left) {
		char c = *scan++;
		if (q) {
			if (c==q)
				q = 0;
		} else if ((c=='"' || c=='\''))
			q = c;
		else if (c==d)
			return int(length-left);
		--left;
	}
	return -1;
}

// if this string begins as an xml quote return that.
strref strref::get_quote_xml() const
{
	char quote_char = get_first();
	if (quote_char!='"' && quote_char!='\'')
		return strref();

	const char *scan = string+1;
	strl_t left = length-1;
	while (left) {
		char c = *scan++;
		if (c==quote_char)
			return strref(string+1, length-left-1);
		--left;
	}
	return strref();
}

// find the character d outside of a quote
int strref::find_quoted(char d) const
{
	strl_t left = length;
	const char *scan = string;
	char quote_char = 0;
	char previous_char = 0;
	while (left) {
		char c = *scan++;
		if (quote_char) {
			if (c==quote_char && previous_char!='\\')
				quote_char = 0;
		} else if (c=='"' || c=='\'')
			quote_char = c;
		else if (c==d)
			return int(length-left);
		--left;
	}
	return -1;
}

strref strref::split( strl_t pos ) { 
	pos = limit_pos( pos );
	strref ret = strref( string, pos );
	*this += pos;
	return ret;
}

strref strref::split( int pos ) {
	return split( strl_t( pos ) );
}

strref strref::split_token( char c ) {
	int t = find( c );
	if ( t < 0 ) t = ( int )length;
	strref r = strref( string, strl_t( t ) );
	*this += t + 1;
	return r;
}

strref strref::split_token_any( const strref chars )
{
	strref r; int t = find_any_char_of( chars );
	if ( t >= 0 ) {
		r = strref( string, t );
		*this += t;
	}
	return r;
}

strref strref::split_token_trim( char c ) {
	strref r = split_token( c );
	skip_whitespace();
	r.trim_whitespace();
	return r;
}

strref strref::split_token_any_trim( const strref chars )
{
	int t = find_any_char_of( chars );
	if ( t < 0 ) t = ( int )length;
	strref r = strref( string, t );
	*this += t + 1;
	r.trim_whitespace();
	return r;
}

strref strref::split_range( const strref range, strl_t pos )
{
	int t = find_any_char_or_range( range, pos );
	if ( t < 0 ) t = ( int )length;
	strref r = strref( string, t );
	*this += t;
	return r;
}
strref strref::split_range_trim( const strref range, strl_t pos )
{
	int t = find_any_char_or_range( range, pos );
	if ( t < 0 ) t = ( int )length;
	strref r = strref( string, t );
	*this += t;
	r.trim_whitespace();
	trim_whitespace();
	return r;
}

strref strref::split_label() {
	skip_whitespace();
	strref r( string, len_label() );
	*this += r.length; 
	skip_whitespace(); 
	return r;
}

strref strref::split_num() {
	skip_whitespace();
	strref r( string, 0 );
	while( length && *string >= '0' && *string <= '9' ) {
		r.length++;
		string++;
		length--;
	}
	skip_whitespace();
	return r;
}


// split string based on common programming tokens (words, quotes, scopes, numbers)
strref strref::split_lang()
{
	skip_whitespace();
	char c = get_first();
	strl_t l = 1;
	if (c == '"')
	{
		int f = find_after('"', 1);
		l = f >= 0 ? (strl_t)(f+1) : get_len();
	}
	else if (c == '{' || c=='(')
	{
		strref f2(*this);
		l = f2.scoped_block_comment_skip(true).get_len();
	}
	else if (strref::is_number(c) || c == '-')
	{
		if ((c == '0' && string[1] == 'x') || string[1] == 'X')
			l = 2 + (*this + 2).len_hex();
		else
			l = len_float_number();
	}
	else
		l = len_grayspace();
	strref r = split(l);	// control character
	return r;
}

strref strref::get_snippet( strl_t pos )
{
	strref snippet(string, end_line_pos(pos));
	snippet.skip(snippet.prev_line_pos(snippet.start_line_pos(pos)));
	return snippet;
}

// grab a block of text starting with (, [ or { and end with the corresponding number of ), ] or }
strref strref::scoped_block_skip(bool quotes)
{
	char scope = get_first();
	if (length && (scope == '(' || scope == '[' || scope == '{')) {
		char close = scope=='(' ? ')' : (scope=='[' ? ']' : '}');
		const char *scan = string;
		bool inQuote = false;
		strl_t depth = 0;
		strl_t left = length;
		do {
			char c = *scan++;
			left--;
			if( inQuote ) {
				if( c == '"' ) { inQuote = false;  }
			} else if( quotes && c=='"' ) { inQuote = true; }
			else  if( c == scope )
				depth++;
			else if (c==close)
				depth--;
		} while (depth && left);
		if (!depth) {
			strl_t block_len = strl_t(scan - string);
			strref block = strref(string, block_len);
			block.string++;
			block.length -= 2;
			string += block_len;
			length -= block_len;
			return block;
		}
	}
	return strref();
}

// scoped_block_skip with C style comments
strl_t strref::scoped_block_comment_len()
{
	char scope = get_first();
	if (length && (scope == '(' || scope == '[' || scope == '{' || scope == '<'))
	{
		char close = scope == '<' ? '>' : (scope == '(' ? ')' : (scope == '[' ? ']' : '}'));
		const char *scan = string;
		strl_t depth = 0;
		strl_t left = length;
		do {
			char c = *scan++;
			left--;
			if (c == '/' && left && (*scan == '/' || *scan == '*')) {
				c = *scan++;
				left--;
				strl_t skip = c == '/' ? strref(scan, left).len_next_line() : strref(scan, left).find_or_full("*/");
				scan += skip;
				left -= skip;
			} else if (c == scope)
				depth++;
			else if (c == close)
				depth--;
		} while (depth && left);
		if (!depth)
			return strl_t(scan-string);
	}
	return 0;
}



// return the current line of text and move this string ahead to the next.
// note: supports all known line feed configurations.
strref strref::next_line()
{
	const char *start = string;
	const char *scan = start;
	strl_t left = length; // if not valid left=0 and no characters will be interpreted
	strref ret;
	while (left && *scan!=0x0a && *scan!=0x0d) {
		scan++;
		left--;
	}
	// this is the line to return
	ret = strref(start, strl_t(scan-start));
	if (left) {
		char c = *scan++;
		left--;
		if (left && ((c==0x0a && *scan==0x0d) || (c==0x0d && *scan==0x0a))) {
		scan++;
			left--;
		}
	}
	if (left) {
		string = scan;
		length = left;
	} else {
		string = nullptr;
		length = 0;
	}
	return ret;
}

// get line from current string
strref strref::get_line() const
{
	const char *start = string;
	const char *scan = start;
	strl_t left = length; // if not valid left=0 and no characters will be interpreted
	strref ret;
	while (left && *scan!=0x0a && *scan!=0x0d) {
		scan++;
		left--;
	}
	// this is the line to return
	return strref(start, strl_t(scan-start));
}

// get a specific line number (0 indexed)
strref strref::get_line(strl_t line_num) const {
	strref scan(*this);
	while (scan) {
		strref line = scan.next_line();
		if (!line_num)
			return line;
		line_num--;
	}
	return strref();
}

// determine how many characters can be used for floating point
strl_t strref::len_float_number() const
{
	// skip whitespace
	const char *scan = string;
	strl_t left = length;

	// valid check
	if (scan==nullptr || left==0)
		return 0;

	// not a floating point if just spaces and a dot
	bool has_value = false;

	// include whitespace
	strl_t ws = len_whitespace();
	scan += ws;
	left -= ws;

	// include sign
	if (left && (*scan=='-' || *scan=='+')) {
		scan++;
		left--;
		if (!left || !is_number(*scan))
			return 0;
	}

	// integer portion
	while (left && is_number(*scan)) {
		scan++;
		left--;
		has_value = true;
	}

	// decimal
	if (left && *scan=='.') {
		scan++;
		left--;
	}

	// fraction
	while (left && is_number(*scan)) {
		scan++;
		left--;
		has_value = true;
	}

	// exponent
	if (left && (*scan=='e' || *scan=='E')) {
		strl_t e = left;
		scan++;
		left--;
		if (left && (*scan=='-' || *scan=='+')) {
			scan++;
			left--;
		}
		if (!left || !is_number(*scan))
			return length-e;
		while (left && is_number(*scan)) {
			scan++;
			left--;
			has_value = true;		// e-10 is a fine floating point number
		}
	}

	// return size of floating point number
	return has_value ? length-left : 0;
}

// count number of valid hexadecimal characters, leading 0x not valid
strl_t strref::len_hex() const
{
	for (strl_t i = 0; i < length; i++) {
		if (!is_hex((uint8_t)string[i]))
			return i;
	}
	return length;
}

// insert a substring into a string
strl_t _strmod_insert(char *string, strl_t length, strl_t cap, const strref sub, strl_t pos)
{
	if (pos>length)
		return 0;

	if (sub.get_len()==0)
		return 0;

	strl_t ins = sub.get_len();
	strl_t end = length;
	strl_t last = ins+end;
	if (last>cap) {
		if (ins+pos>cap) {
			if (ins>cap)
				ins = 0;
			else
				ins = cap-pos;
		}
	} else {
		const char *src = string+end;
		char *dst = string+ins+end;
		strl_t move = length-pos;
		for (; move; move--)
			*--dst = *--src;
	}
	const char *src = sub.get();
	uint8_t *dst = (uint8_t*)string + pos;
	const uint8_t *e = (uint8_t*)string + cap;
	strl_t left = sub.get_len();
	while (left && dst < e) {
		uint8_t c = (uint8_t)*src++;
		left--;
		*dst++ = c;
	}

	return ins + length;
}

// determine the size of this string with evaluated escape codes
static strl_t int_string_size_esc(const uint8_t *string, strl_t length)
{
	strl_t size = 0;
	while (length) {
		uint8_t c = *string++;
		length--;
		if (c=='\\' && length) {
			strl_t skip = int_get_esc_code(string, length, c);
			string += skip;
			length -= skip;
		}
		size++;
	}
	return size;
}

// insert a substring into a string allowing for escape codes
strl_t _strmod_insert_esc(char *string, strl_t length, strl_t cap, const strref sub, strl_t pos)
{
	if (pos>length)
		return 0;

	if (sub.get_len()==0)
		return 0;

	strl_t ins = int_string_size_esc(sub.get_u(), sub.get_len());
	strl_t end = length;
	strl_t last = ins+end;
	if (last>cap) {
		if ((ins+pos)>cap) {
			if (pos>cap)
				ins = 0;
			else
				ins = cap-pos;
		}
	} else {
		const char *src = string+end;
		char *dst = string+ins+end;
		strl_t move = length-pos;
		for (; move; move--)
			*--dst = *--src;
	}
	const uint8_t *src = sub.get_u();
	uint8_t *dst = (uint8_t*)string + pos;
	const uint8_t *e = (uint8_t*)string + cap;
	strl_t left = sub.get_len();
	while (left && dst < e) {
		uint8_t c = *src++;
		left--;
		if (c=='\\' && left) {
			strl_t skip = int_get_esc_code(src, left, c);
			src += skip;
			left -= skip;
		}
		*dst++ = c;
	}

	return ins + length;
}

// insert substrings by {n} notation
strl_t _strmod_format_insert(char *string, strl_t length, strl_t cap, strl_t pos,
							 strref format, const strref *args) {
	// insert many things..

	// can't insert at a position that is beyond the current size.
	if (pos > length)
		return length;

	while (format) {
		// scan for '{'
		strl_t ins = format.find_or_full_esc('{', 0);
		int close = format.find_after('}', ins);
		if (close<0) {
			ins = format.get_len();
		}

		// insert block before '{'
		if (ins > 0) {
			strl_t prev = length;
			length = _strmod_insert_esc(string, length, cap, format.get_clipped(ins), pos);
			pos += length - prev;
			format += ins;
		}

		// if there was a {} process that..
		if (format.get_first()=='{' && close>0) {
			int64_t which = format.get_substr(1, close-ins).atoi();
			strl_t prev = length;
			length = _strmod_insert(string, length, cap, args[which], pos);
			pos += length - prev;
			format += close-ins+1;
		}
	}
	return length;
}

// remove all instances of a character from a string
strl_t _strmod_remove(char *string, strl_t length, char a)
{
	char *scan = string;
	strl_t left = length;
	while (left && *scan!=a) {
		left--;
		scan++;
	}
	if (left) {
		strl_t n = left;
		char *write = scan;
		while (left) {
			while (left && *scan==a) {
				left--;
				scan++;
			}
			while (left && *scan!=a) {
				*write++ = *scan++;
				left--;
				n--;
			}
		}
		length -= n;
	}
	return length;
}

// remove a substring from a string
strl_t _strmod_remove(char *string, strl_t length, strl_t start, strl_t len)
{
	if (start<length) {
		if ((start+len)>length)
			len = length-start;
		int left = int(length-start-len);
		if (left>0) {
			const char *source = string+start+len;
			char *dest = string+start;
			for (int i = left; i; i--)
				*dest++ = *source++;
		}
		length = length-len;
	}
	return length;
}

// exchange a substring
strl_t _strmod_exchange(char *string, strl_t length, strl_t cap, strl_t start, strl_t size, const strref insert)
{
    if (start > length)
        return length;

    if ((start + size) > length)
        size = length - start;

    strl_t copy = insert.get_len();
    if ((start + copy) > cap)
        copy = cap - start;

    if (copy < size) {
        strl_t rem = size - insert.get_len();
        length = _strmod_remove(string, length, start+size-rem, rem);
    } else if (copy > size) {
        strl_t ins = insert.get_len() - size;
        strl_t left = length - size - start;
        char *end = string + length + ins;
        char *orig = string + length;
        while (left--)
            *--end = *--orig;
        length += ins;
    }
    memcpy(string + start, insert.get(), copy);
    return length;
}


// search and replace occurences of a string within a string
strl_t _strmod_inplace_replace_int(char *string, strl_t length, strl_t cap, const strref a, const strref b)
{
	char *scan = string;
	strl_t left = length;
	strl_t c = cap;
	strl_t len_a = a.get_len(), len_b = b.get_len();
	if (len_a>left || !len_a)
		return left;

	char *ps = scan, *pd = scan;
	if (len_a>=len_b) {
		int ss = strref(ps, left-strl_t(ps-scan)).find(a);
		if (ss>=0) {
			pd += ss;
			ps += ss;
			while (ss>=0 && strl_t(ss)<left) {
				ps += len_a;
				int sl = strref(ps, left-ss-len_a).find(a);
				if (sl<0)
					sl = int(left-ss-len_a);
				if (len_b) {
					const char *po = b.get();
					strl_t r = len_b;
					while (r--)
						*pd++ = *po++;
				}
				if (ps!=pd && sl) {
					int r = sl;
					while (r--)
						*pd++ = *ps++;
				}
				ss += (int)len_a+sl;
			}
			return strl_t(pd-scan);
		}
	} else if (int cnt = strref(scan, left).substr_count(a)) {
		strl_t nl = cnt * (len_b-len_a) + left;	// new length
		if (nl>c)
			return left;	// didn't fit in space
		int ss = strref(scan, left).find_last(a);
		int se = (int)left;
		pd += nl;
		ps += left;
		while (ss>=0) {
			strl_t cp = se-ss-len_a;
			while (cp--)
				*--pd = *--ps;
			ps -= len_a;
			const char *be = b.get()+len_b;
			cp = len_b;
			while (cp--)
				*--pd = *--be;
			se = ss;
			ss = strref(scan, se).find_last(a);
		}
		return nl;
	}
	return left;
}

// search and replace occurences of a string within a string
strl_t _strmod_inplace_replace_bookend_int(char *string, strl_t length, strl_t cap, const strref a, const strref b, const strref bookend)
{
	char *scan = string;
	strl_t left = length;
	strl_t c = cap;
	strl_t len_a = a.get_len(), len_b = b.get_len();
	if (len_a>left || !len_a)
		return left;

	char *ps = scan, *pd = scan;
	if (len_a >= len_b) {
		int ss = strref(ps, left - strl_t(ps - scan)).find_bookend(a, bookend);
		if (ss >= 0) {
			pd += ss;
			ps += ss;
			while (ss >= 0 && strl_t(ss)<left) {
				ps += len_a;
				int sl = strref(ps, left - ss - len_a).find_bookend(a, bookend);
				if (sl<0)
					sl = int(left - ss - len_a);
				if (len_b && b.get()) {
					const char *po = b.get();
					strl_t r = len_b;
					while (r--)
						*pd++ = *po++;
				}
				if (sl) {
					if (ps != pd) {
						int r = sl;
						while (r--)
							*pd++ = *ps++;
					} else {
						pd += sl;
						ps += sl;
					}
				}
				ss += (int)len_a + sl;
			}
			return strl_t(pd - scan);
		}
	} else if (int cnt = strref(scan, left).substr_count_bookend(a, bookend)) {
		strl_t nl = cnt * (len_b - len_a) + left;	// new length
		if (nl>c)
			return left;	// didn't fit in space
		int ss = strref(scan, left).find_last_bookend(a, bookend);
		int se = (int)left;
		pd += nl;
		ps += left;
		while (ss >= 0) {
			strl_t cp = se - ss - len_a;
			while (cp--)
				*--pd = *--ps;
			ps -= len_a;
			if (b.get()) {
				const char *be = b.get() + len_b;
				cp = len_b;
				while (cp--)
					*--pd = *--be;
			}
			se = ss;
			ss = strref(scan, se).find_last_bookend(a, bookend);
		}
		return nl;
	}
	return left;
}

// convert a string to lowercase (7 bit ascii)
void _strmod_tolower(char *string, strl_t length)
{
	if (string) {
		uint8_t *s = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*s = int_tolower_ascii7(*s);
			s++;
		}
	}
}

// convert a string to lowercase (windows extended ascii)
void _strmod_tolower_win_ascii(char *string, strl_t length)
{
	if (string) {
		uint8_t *s = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*s = int_tolower_win_ascii(*s);
			s++;
		}
	}
}

// convert a string to lowercase (amiga extended ascii)
void _strmod_tolower_amiga_ascii(char *string, strl_t length)
{
	if (string) {
		uint8_t *scan = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*scan = int_tolower_amiga_ascii(*scan);
			scan++;
		}
	}
}

// convert a string to lowercase (mac os extended ascii)
void _strmod_tolower_macos_ascii(char *string, strl_t length)
{
	if (string) {
		uint8_t *scan = (uint8_t*)string;
		for (strl_t r = length; r>0; r--) {
			*scan = int_tolower_macos_roman_ascii(*scan);
			scan++;
		}
	}
}

// convert a string to uppercase
void _strmod_toupper(char *string, strl_t length)
{
	if (string) {
		uint8_t *scan = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*scan = int_toupper_ascii7(*scan);
			scan++;
		}
	}
}

// convert a string to uppercase
void _strmod_toupper_win_ascii(char *string, strl_t length)
{
	if (string) {
		uint8_t *scan = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*scan = int_toupper_win_ascii(*scan);
			scan++;
		}
	}
}

// convert a string to uppercase
void _strmod_toupper_amiga_ascii(char *string, strl_t length)
{
	if (string) {
		uint8_t *scan = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*scan = int_toupper_amiga_ascii(*scan);
			scan++;
		}
	}
}


// convert a string to uppercase
void _strmod_toupper_macos_ascii(char *string, strl_t length)
{
	if (string) {
		uint8_t *scan = (uint8_t*)string;
		for (strl_t left = length; left>0; left--) {
			*scan = int_toupper_macos_roman_ascii(*scan);
			scan++;
		}
	}
}

strl_t _strmod_copy(char *string, strl_t cap, const char *str)
{
	strl_t length = 0;
	if (str) {
		while (*str && length < cap)
			string[length++] = *str++;
	}
	return length;
}

strl_t _strmod_copy(char *string, strl_t cap, strref str)
{
	strl_t length = 0;
	if (str.valid()) {
		const char *_str = str.get();
		for (strl_t len = str.get_len(); len && length<cap; len--)
			string[length++] = *_str++;
	}
	return length;
}

strl_t _strmod_append(char *string, strl_t length, strl_t cap, const char *str)
{
	if (str) {
		while (*str && length < cap)
			string[length++] = *str++;
	}
	return length;
}

strl_t _strmod_append(char *string, strl_t length, strl_t cap, strref str)
{
	if (str.valid()) {
		const char *_str = str.get();
		strl_t len = str.get_len();
		for (; len && length<cap; len--)
			string[length++] = *_str++;
	}
	return length;
}

void _strmod_substrcopy(char *string, strl_t length, strl_t cap, strl_t src, strl_t dst, strl_t chars) {
	if (src<length && src!=dst && chars) {
		if ((src+chars)>length)
			chars = length - src;
		if ((dst+chars)>cap)
			chars = cap - dst;
		char *ps = string+src, *pd = string+dst;
		if (src>dst) {
			while (chars--)
				*pd++ = *ps++;
		} else {
			pd += chars;
			ps += chars;
			while (chars--)
				*--pd = *--ps;
		}
	}
}

void _strmod_shift(char *string, int offs, int len) {
	char *dest = string + offs;
	if (offs>0) {
		string += len;
		dest += len;
		while (len--)
			*--dest = *--string;
	} else {
		while (len--)
			*dest++ = *string++;
	}
}

size_t _strmod_read_utf8(char *string, strl_t length, strl_t pos, strl_t &skip) {
	if (pos >= length) {
		skip = 0;
		return 0;
	}

	string += pos;
	length -= pos;
	const char *start = string;
	const char *end = string + length;
	size_t c = (size_t)*string++;
	c &= 0x7f;
	for (size_t m = 0x40; (m & c) && string<end; m <<= 5) {
		size_t n = (size_t)*string++;
		c = ((c & ~m) << 6) | (n & 0x3f);
	}
	skip = strl_t(string - start);
	return c;
}

strl_t _strmod_write_utf8(char *string, strl_t cap, size_t code, strl_t pos) {
	if (pos>=cap)
		return 0;
	uint8_t *write = (uint8_t*)string + pos;
	cap -= pos;
	if (code < 0x80) {
		*write++ = (uint8_t)code;
		return 1;
	} else if (cap>=2 && code < 0x800) {
		*write++ = uint8_t(0xc0 | (code >> 6));
		*write++ = 0x80 | (code & 0x3f);
		return 2;
	} else if (cap>=3 && code < 0x10000) {
		*write++ = uint8_t(0xe0 | (code >> 12));
		*write++ = 0x80 | ((code >> 6) & 0x3f);
		*write++ = 0x80 | (code & 0x3f);
		return 3;
	} else if (cap>=4) {
		*write++ = 0xf0 | ((code >> 18) & 7);
		*write++ = 0x80 | ((code >> 12) & 0x3f);
		*write++ = 0x80 | ((code >> 6) & 0x3f);
		*write++ = 0x80 | (code & 0x3f);
		return 4;
	}
	return 0;
}

strl_t _strmod_utf8_tolower(char *string, strl_t length, strl_t cap) {
	char *scan = string;
	char *write = string;
	char *end = string + length;

	while (scan<end) {
		// get one character
		strl_t skip;
		size_t c = _strmod_read_utf8(scan, strl_t(end-scan), 0, skip);
		scan += skip;

		c = int_tolower_unicode(c);

		strl_t add = c<size_t(0x80) ? strl_t(1) : (c<size_t(0x800) ? strl_t(2) : (c<size_t(0x10000) ? strl_t(3) : 4));

		if (add > cap)
			return (strl_t)(write-string);

		// need to make room for new character code
		if ((write+add)>scan) {
			int m = (int)((write+add) - scan);
			_strmod_shift(scan, m, (int)(end-scan));
			scan += m;
			end += m;
		}
		skip = _strmod_write_utf8(write, cap, c, 0);
		write += skip;
		cap -= skip;
	}
	return (strl_t)(end-string);
}

strl_t _strmod_utf8_toupper(char *string, strl_t length, strl_t cap) {
	char *scan = string;
	char *write = string;
	char *end = string + length;

	while (scan<end) {
		// get one character
		strl_t skip;
		size_t c = _strmod_read_utf8(scan, strl_t(end-scan), 0, skip);
		scan += skip;

		c = int_toupper_unicode(c);

		strl_t add = c<0x80 ? strl_t(1) : (c<0x800 ? strl_t(2) : (c<0x10000 ? strl_t(3) : strl_t(4)));

		if (add > cap)
			return (strl_t)(write-string);

		// need to make room for new character code?
		if ((write+add)>scan) {
			int m = (int)((write+add) - scan);
			_strmod_shift(scan, m, (int)(end-scan));
			scan += m;
			end += m;
		}
		skip = _strmod_write_utf8(write, cap, c, 0);
		write += skip;
		cap -= skip;
	}
	return (strl_t)(end-string);
}

strl_t _strmod_cleanup_path(char *file, strl_t len)
{
	strl_t pos = 0;
	char *trg = file;
	while(len>4 && file[len-3]==DIR_SEP && file[len-2]=='.' && file[len-1]=='.') {
		 len -= 4;
		 while(len>1 && file[len]!=DIR_SEP) {--len;}
	 }
	while (len) {
		len--;
		char c = *file++;
		if (c==NOT_DIR_SEP) { c=DIR_SEP; }
		trg[pos] = c;
		if ((c==NOT_DIR_SEP || c==DIR_SEP) && len>=3 && *file=='.' && file[1]=='.' && (file[2] =='/' || file[2]=='\\') && pos) {
			// attempt to rewind
			strl_t rew = pos-1;
			while (rew) {
				if (trg[rew-1] == NOT_DIR_SEP || trg[rew-1]==DIR_SEP) { break; }
				--rew;
			}
			pos = rew;
			file += 3;
			len -= 3;
		} else {
			++pos;
		}
	}
	return pos;
}

// determine a 
strl_t _strmod_relative_path(char *out, strl_t cap, strref orig, strref target)
{
	// remove trailing folder separator from source
	char c = orig.get_last();
	if (c=='/' || c=='\\')
		orig.clip(1);

	// skip as many characters as matches at start of path
	strl_t same_count = orig.prefix_len(target, '/', '\\');

	// make sure that prior character is a folder (sub paths may share a prefix)
	if (same_count != orig.get_len()) {
		while( same_count ) {
			c = orig.get()[same_count-1];
			if (c == '/' || c=='\\')
				break;
			--same_count;
		}
	}

	// orig may be a path without ending in a folder separator
	if (same_count == orig.get_len() && target.get_len() > same_count && (target.get()[same_count] == '/' || target.get()[same_count]=='\\'))
		++same_count;

	// skip the path of the path that is the same
	orig += same_count;
	target += same_count;

	// count the number of folders to step up (number of folder separators)
	strl_t step_up = orig.count_char('/') + orig.count_char('\\');
	strl_t len = 0;
	while (step_up-- && cap) {
		strl_t add = _strmod_copy(out, cap, "../");
		out += add;
		len += add;
		cap -= add;
	}
	c = target.get_first();
	if (c=='/' || c=='\\')
		++target;

	if (cap)
		len += _strmod_copy(out, cap, target);
	return len;
}

#endif // STRUSE_IMPLEMENTATION

/* revision history
	0.990	(2015-09-14)	first public version
    1.000   (2015-09-15)    added XML parser sample
    1.001   (2015-09-16)    cleaned up XML parser
    1.002   (2015-09-17)    added JSON parser sample
	1.003	(2015-09-20)	straightening up of things
                            - wildcard add rewind & retry for multi step search, this caused valid finds to be ignored if invalid sub find occured
                            - fixed some minor wildcard search bugs, including word end including an extra character (whitespace)
                            - slightly more compact implementation, combining common code segments into static functions
                            - next_line() will return empty lines to match actual line count, line() works as before (returns only nonempty lines)
	1.004	(2015-09-22)	added text file diff / patch sample
	1.005	(2015-09-28)	added 6502 macro assembler sample
	1.006	(2015-10-04)	added get_line() to get first line, fixed errors with getting hex/binary value+skip (ahextoui_skip, abinarytoui_skip)
	1.007	(2016-05-20)	fixed wildcard errors and warnings in various configurations
*/

#endif // __STRUSE_H__
