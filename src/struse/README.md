# STRUSE DOCUMENTATION

USAGE

Add this #define to *one* C++ file before #include "struse.h" to create the implementation:

 #define STRUSE_IMPLEMENTATION

[Samples](../../tree/master/samples) contains basic, precompile hash, xml parsing and diff/patch samples using struse classes in a variety of ways.

[x65](https://github.com/sakrac/x65) Is a 6502 assembler built around struse.h.

struse is a collection of string parsing and string modification classes that are intended to work together to simplify parsing blocks of text, building blocks of text.

StrUse is a little bit different from built-in string types and classes in that it has multiple classes to interface with string data.

* '**strref**' is a string reference class that refers to text rather than copying. There are a variety of ways to search and iterate over text blocks using strref.
* '**strown**' is a scope-based (stack) modifiable string (char[])
* '**strovl**' is similar to 'strown' but leaves the string pointer (char*) to be set by the implementation rather than a scope array.
* '**strcol**' is a string collection

None of these classes perform allocations so string data and string memory is provided by the caller.

Think of strref as a const char pointer with a length. This allows for searches and iteration on constant text data and is primarily used as a parsing tool.

**strown** is close to a stringbuilder in c# or java. You can create this on the stack or new it and as part of creating it the maximum capacity is required. Most strref functions are mirrored in strown so all the search and iteration (using strref) can be done with strown strings.

**strovl** has the same features as strown but with a user provided memory block. This allows for usage suc as reading in a file to memory and search / modify in place. Just make sure there is some margin in the allocated capacity in case the insertions are larger than the removals.

**strcol** is a very basic list of string copies that shares a single block of memory. Use cases will be included when I have something shareable.

To support printf formatting with non-zero terminated strings there are two macros that work together:

```
printf("\"" STRREF_FMT "\"\n", STRREF_ARG(strref));
```

### Reliability:

An effort has been made to check that all write operations fit within given space and that all reading operations access only the part of memory that has been assigned to a string. That is not to say that this code is without bugs, use at your own risk but let me know of any issues. This code is provided to assist with often tedious string management issues and a certain knowledge of how string manipulation works may be required to understand these classes.

If anything is unclear I strongly suggest looking at the header file or the code. As far as reasonable the code is written to be easy to read.

### Next steps

* Clean up test code and add to the depot
* Clean up project files and include in the depot
* Clean up sample code (xml, json, cfg, csv, etc. parsing) and include
* More documentation

### Future considerations

* wchar_t support

---

## strref overview

The intent of strref is to iterate over text and easily reference substrings without allocations to hold what is basically copies. To achieve this strref holds both a pointer to text and the length of the text.

The most common application of strref is to load in a text file into memory and create a strref from the pointer and file size.

With a strref it is easy to iterate through each line of the text by:

```
while (strref current_line = text.line()) {
    do something with current_line
}
```

If the format is fairly straightforward, current_line may be as simple as variable = value. Separating the two sides of the equal sign can be done by splitting it up and trimming remaining whitespace by calling

```
strref value = current_line.split_token_trim('=');
```

Assuming that the original line was "    numbers = 73.4, 12.2, 13, 19.2"
current_line would now be "numbers" and value would be "73.4, 12.2, 13, 19.2"

In order to iterate over the individual numbers:

```
while (strref number = value.next_token(',')) {
    int value = number.atoi();
    ...
}
```

So putting this together:

```
while (strref current_line = text.line()) {
  strref value = current_line.split_token_trim('=');
  if (current_line.same_str_case("numbers")) {
    while (strref number = value.next_token(',')) {
      int value = number.atoi();
      ...
    }
  }
  ...
}
```

This will go through all lines in a file, split by equal signs and handle tokenized lists of numbers.

strref has a variety of helper functions to avoid string duplication and code duplication including comparisons, searches and wildcard matching.



## strref specification

Function naming rules:

In order to simplify the features of class functions there are some naming conventions that are followed:

* find* returns a position of a string where a match if found or -1 if not
* skip*/clip* trims beginning or end of string based on condition
* before*/after* returns a substring matching the condition
* is_* returns a bool for a character or whole string test
* len_* return the number of characters matching the condition from the start of the string
* *_last indicates that search is done from the end of the string
* *_rh indicates a rolling hash search is used
* *_esc indicates that the search string allows for escape codes (\x => x)
* same_str* is a full string compare and returns true or false based on condtions
* *_range indicates that the result is filtered by a given set of valid characters

### Operators

* strref += (int) / strref++: Move start of string forward and reduce length
* strref > strref / strref < strref: Returns alphabetical sorting order greater than / less than
* strref[(int)]: Return character at position

For function descriptions, please refer to the struse.h.



## strref wildcard find

As an alternative to setting up a series of string scanning calls there is built-in wildcard matching support. Using wildcards is similar to most software searches that allow wildcards.

Wildcard control characters:

* **?**: any single character
* **#**: any single number
* **[]**: any single between the brackets
* **[-]**: any single in the range from character before - to character after
* **[!]**: any single not between the brackets
* **&lt;**: start of word
* **&gt**: end of word
* **@**: start of line
* **^**: end of line
* **\***: any substring
* **\*%**: any substring excluding whitespace
* **\*@**: any substring on same line
* **\*$**: any substring containing alphanumeric ascii characters
* **\*{}**: any substring only containing characters between parenthesis
* **\*{!}**: any substring not containing characters between parenthesis
* **\?**, **\[**, **\\\***, etc.: search for character after backslash
* **\n**, **\t**, etc.: search for linefeed, tab etc.

(words are groups of letters not containing whitespace or separators which are alphanumeric characters plus apostrophe ('))

```
result = text.find_wildcard(pattern, position=0, case_sensitive=true)
```

will find the pattern in the strref text and return as a strref result.

```
result = text.next_wildcard(pattern, prev, case_sensitive=true)
```

will find the pattern in text after the previously found 'prev' begins.

```
result = text.wildcard_after(pattern, prev, case_sensitive=true)
```

will find the pattern in text after the previously found 'prev' ends.

### Example:

```
strref result;
while (result = text.wildcard_after("full*{! }.png", result)) {
    printf(STROP_FMT "\n", STROP_ARG(reslt));
}
```

will find all matches of the pattern in the text and print them.


## Token iteration support:

Reading in token separated values is a common function of text parsers. This
can be done in a number of ways depending on what is needed.

Given a string like: '23,12,5,91,54,8,23,17,67' each number can be fetched with
this loop:

```
while (strref num = line.next_token(',')) {
	int value = num.atoi();
}
```


## strown / strovl support:

strown and strovl share the same base (strmod) and share the same code. The difference is that strown includes the memory for the string and strovl requires user provided space.

The most straightforward way to put text into strown is to create the string with the text:

```
strown<256> test("test string");
```

You can also copy a string or strref into a strown with the copy function:

test.copy(test_strref);

Other ways to add text to a strown/strovl include:

* **insert**(string, pos): move text forward at pos by string size and insert string at position.
* **append**(string): add string at end
* **append**(char): add character at end
* **prepend**(string): insert at start
* **format**(format_string, args): format a string c# string style with {n} where n is a number indicating which of the strref args to insert
* **sprintf**(format, ...): use sprintf formatting with zero terminated c style strings and other data types.

format and sprint have appending versions, format can also insert and sprint can overwrite.

STRREF FUNCTIONS

(table is not complete yet)

* uint means unsigned int

constructors

return|name|description
------|----|-----------
 |()|empty constructor
 |(const char*)|zero terminated string constructor
 |(const char*, strl_t)|provided length string constructor
 |(const char*, int)|provided length string constructor

string

return|name|description
------|----|-----------
bool|valid()|null and length check
void|clear()|invalidate string
const char*|get()|get string pointer
strl_t|get_len()|get string length
char|get_first()|get first character in string
char|get_last()|get last character in string
bool|is_empty|true if length is zero
bool|valid_ascii7|true if string is likely an ascii string (otherwise possibly unicode)

check if a string is within another string

return|name|description
------|----|-----------
strl_t|limit_pos(strl_t)|get lesser of argument and string length
bool|is_substr(const char*)|true if argument is a substring of string
strl_t|substr_offs(strref)|offset, nonzero if argument is part of string
strl_t|substr_end_offs(strref)|as above but last offset instead of first

fnv1a hashes

return|name|description
------|----|-----------
uint|fnv1a([opt. seed])|fnv1a hash from string
uint|fnv1a_lower([opt. seed])|fnv1a hash from lowercase string
uint|fnv1a_upper([opt. seed])|fnv1a hash from uppercase string
uint|fnv1a_ws([opt. seed])|fnv1a ignore whitespace (ws repl. with single space)

numeric conversion

return|name|description
------|----|-----------
int|atoi()|convert ascii to integer
float|atof()|convert ascii to floating point
double|atod()|convert ascii to double precision
int|atoi_skip()|convert ascii to int and skip string forward
int|ahextoi()|convert ascii hexadecimal to int
uint|ahextoui()|convert unsigned ascii hex to uint
uint|ahextoui_skip()|convert unsigned ascii hex to uint and skip string forward
uint|abinarytoui_skip()|convert unsigned ascci binary to uint and skip str fwd

print

return|name|description
------|----|-----------
void|writeln()|printf string with a return

static character checks and conversions

return|name|description
------|----|-----------
bool|is_ws(unsigned char)|static, true if char is a whitespace
bool|is_number(unsigned char)|static, true if char is a number
bool|is_hex(unsigned char)|static, true if char is a hexadecimal number
bool|is_alphabetic(unsigned char)|static, true if char is a-z or A-Z
bool|is_alphanumeric(unsigned char)|static, true if char is 0-9, a-z or A-Z
bool|is_valid_label(unsigned char)|static, true if alphanumeric or underscore
bool|is_sep_ws(unsigned char)|static, true if not \' or alphanumeric
bool|is_control(unsigned char)|static, true if not whitespace or alphanumeric or underscore
char|tolower(char c)|static, returns english lowercase
char|toupper(char c)|static, returns english uppercase
char|tolower_win(char c)|static, returns windows lowercase
char|toupper_win(char c)|static, returns windows uppercase
char|tolower_amiga(char c)|static, returns amiga lowercase
char|toupper_amiga(char c)|static, returns amiga uppercase
char|tolower_macos(char c)|static, returns mac os lowercase
char|toupper_macos(char c)|static, returns mac os uppercase
int|tolower_unicode(char c)|static, returns reasonable unicode lowercase
int|toupper_unicode(char c)|static, returns reasonable unicode uppercase

operators

return|name|description
------|----|-----------
void|+=|skip forward by argument
strref|+|returns a strref skipped forward by right side value
 |bool()|true if valid
 |++|step string forward one character
bool|>|check if this is a higher value string than right side string
bool|<|check if this is a lower value string than right side string
char|[]|get character of string at given position

adjust string start / end

return|name|description
------|----|-----------
void|skip(strl_t)|move this string forward by argument
void|clip(strl_t)|clip argument number of characters off end of this string

utf8 access

return|name|description
------|----|-----------
uint|get_utf8()|get first utf-8 character in string
uint|pop_utf8()|get first utf-8 character in string and step forward to next utf-8

get charcter or category at position

return|name|description
------|----|-----------
char|get_at(strl_t)|return character at position of argument if less than length
unsigned char|get_u_at(strl_t)|same as above but return unsigned
bool|whitespace_at(strl_t)|true if position of argument is a whitespace character

counts

return|name|description
------|----|-----------
int|count_char(char)|return number of times a character occurs in string
int|len_eol|count characters to end of line
int|len_next_line()|characters start of next line (may be end of string)
strl_t|len_float_number()|count characters that can be converted to a floating point number
strl_t|len_hex()|count characters that can be converted to a hexadecimal number
bool|is_float_number()|check if string is a valid floating point number
strref|find_wildcard(strref, [strl_t], [bool])|find wildcard starting at optional offset [case {in}sensitive]
strref|next_wildcard(strref, strref, [bool])|find next wildcard match after given match
strref|wildcard_after(strref, strref, [bool])|find next wildcard after given match
bool|char_matches_ranges(unsigned char)|character filter by string, as in a wildcard [] operator
bool|same_str(strref)|true if strings match (case ignore)
bool|same_str_case(strref)|true if strings match (case sensitive)
bool|same_str(const char *)|true if strings match (case ignore)
bool|same_str_case(const char *)|true if strings match (case sensitive)
bool|same_str(strref, char, char)|true if strings match and treat two characters as the same (case ignore)
bool|same_str_case(strref, char, char)|true if strings match and treat two characters as the same (case sensitive)
bool|same_substr(strref, strl_t)|true if provided string is a substring at given position (case ignore)
bool|same_substr_esc(strref, strl_t)|as above and allow escape codes in search string
bool|same_substr_case(strref, strl_t)|same as sam_substr but case sensitive
bool|same_substr_case_esc(strref, strl_t)|as two above
uint|prefix_len(strref)|count characters matching from start of string (case ignore)
uint|prefix_len_case(strref)|count characters matching from start of string (case sensitve)
uint|prefix_len(const char *)|count characters matching from start of string (case ignore)
uint|prefix_len_case(const char *)|count characters matching from start of string (case sensitve)
uint|prefix_len(strref, char, char)|count characters matching from start of string and treat two characters as the same (case ignore)
bool|has_prefix(strref)|true if full argument is a prefix of string
bool|has_prefix(const char*)|true if full argument is a prefix of string
bool|is_prefix_of(strref)|true if full string is a prefix of argument
bool|is_prefix_of(strref, char, char)|true if full string is a prefix of argument treat two characters as if the same
bool|is_prefix_word(strref)|true if full string is a prefix of argument and next character is not alphanumeric 
bool|is_prefix_case_of(strref)|true if string is prefix of argument (case sensitive)
bool|is_prefix_float_number()|true if string starts with a valid floating point value
strl_t|suffix_len(strref)|returns length of matching characters from end in string and argument (case ignore)
strl_t|suffix_len_case(strref)|returns length of matching characters from end in string and argument (case sensitive)
bool|is_suffix_of(strref)|true if full string is a suffix of argument (case ignore)
bool|is_suffix_case_of(strref)|true if full string is a suffix of argument (case sensitive)
bool|has_suffix(const char *)|true if full string is a suffix of argument (case ignore)
int|find(char)|find first index of character, if not found return -1
int|find_at(char, strl_t)|find first index of character at or after position, if not found return -11
int|find_after(char, strl_t)|find first index of character after position, if not found return -11
int|find_or_full(char, strl_t)|find first index of character after index pos or return length for full string
int|find_or_full_esc(char, strl_t)|find first index of character after index pos or return length for full string, allowing escape codes
int|find_last(char)|return last position of character or -1 if not found
int|find(char, char)|return first position of either char c or char d or -1 if not found
int|find_last(char, char)|return last position of either character or -1 if not found
int|find_after_last(char a, char b)|return first 'b' after last 'a' in string
int|find_after_last(char a1, char a2, char b)|as above but after last 'a1' or 'a2' in string
int|find(strref)|return position in this string of the first occurrence of the argument or -1 if not found (case ignore)
int|find_bookend(strref, strref)|return position in this string og the first occurence of the argument but only if bookended by range or -1 if not found
