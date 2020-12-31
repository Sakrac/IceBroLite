#ifndef __XML_STRREF_H__
#define __XML_STRREF_H__

/*
strref no allocation XML parser

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

USAGE:

See xml_example.cpp for reference.

* Load an xml file to memory
* Create a callback function to handle the xml tags
* Call ParseXML with a user structure, the xml data as a strref and your callback function

*/

enum XML_TYPE {
	XML_TYPE_TAG_OPEN,				// <Tag>
	XML_TYPE_TAG_CLOSE,				// </Tag>
	XML_TYPE_TAG_SELF_CLOSE,		// <Tag/>
	XML_TYPE_TEXT					// <Tag>Text</Tag>
};

typedef bool(*XMLDataCB)(void* /*user*/, strref /*tag_or_data*/, const strref* /*tag_stack*/, int /*depth*/, XML_TYPE /*type*/);

// scan through an xml file
bool ParseXML(strref xml, XMLDataCB callback, void *user);

// get a single attribute by name
strref XMLFindAttr(strref tag, strref name);

// get the attributes section of an xml tag
strref XMLGetAttributes(strref tag);

// get the string of attributes from a tag
strref XMLFirstAttribute(strref tag);

// move string to the next attribute of the tag
strref XMLNextAttribute(strref attribute);

// get the name of the current attribute
strref XMLAttributeName(strref attribute);

// get the value of the current attribute
strref XMLAttributeValue(strref attribute);

#endif
