#include "struse.h"
#include "xml.h"

#define XML_DEPTH_MAX 256

// scan through an xml file
bool ParseXML(strref xml, XMLDataCB callback, void *user)
{
	strref stack[XML_DEPTH_MAX];
	int	sp = XML_DEPTH_MAX;
	strref parse = xml;

	while (strref tag = parse.next_chunk_xml('<', '>')) {
		tag.skip_whitespace();
		char c = tag.get_first();
		if (c=='!' || c=='?') {
			// this is a comment or control tag so can probably be ignored
		} else  if (c=='/') {
			// this is a closing tag
			++tag;	// skip '/'
			tag.skip_whitespace();
			if (sp>=XML_DEPTH_MAX || !callback(user, tag, stack+sp, XML_DEPTH_MAX-sp, XML_TYPE_TAG_CLOSE))
				return false;
			strref id = tag.get_clipped(tag.len_grayspace());
			char next_char = stack[sp].get()[id.get_len()];
			if (sp>=XML_DEPTH_MAX || !id.is_prefix_of(stack[sp]) || (!strref::is_ws(next_char) && next_char!='>')) {
				// XML syntax error
				return false;
			}
			sp++;
		} else if (tag.get_last()=='/') {
			// this is a self-closing tag
			strref tag_name = tag.get_substr(0, tag.get_len()-1);
			tag_name.clip_trailing_whitespace();
			if (!callback(user, tag_name, stack + sp, XML_DEPTH_MAX - sp, XML_TYPE_TAG_SELF_CLOSE))
				return false;
		} else {
			// this is an opening tag
			if (!callback(user, tag, stack+sp, XML_DEPTH_MAX-sp, XML_TYPE_TAG_OPEN))
				return false;
			if (sp) {
				--sp;
				stack[sp] = tag;
			} else
				return false;	// stack was too deep
		}

		parse.skip_chunk(tag);
		parse.skip_whitespace();
		int lt = parse.find('<');
		if (lt>0 && !callback(user, strref(parse.get(), lt), stack+sp, XML_DEPTH_MAX-sp, XML_TYPE_TEXT))
			return false;
	}
	return true;
}

// get a single attribute by name
strref XMLFindAttr(strref tag, strref name)
{
	// skip tag and go to the first attribute
	tag.next_word_ws();
	while (tag) {
		strref attr_name = tag.split_token_trim('=');
		if (tag) {
			char q = tag.get_first();
			if (q=='"' || q=='\'') {
				if (attr_name.same_str(name)) {
					// found the attribute, return the value
					++tag;
					return tag.before(q);
				}
			} else {
				// this is an error, attribute value is not in quotes
				return strref();
			}
			++tag;
			tag = tag.after(q);
		}
	}
	return strref();
}

// get the string of attributes from a tag
strref XMLFirstAttribute(strref tag) {
	strref first = tag;
	first.next_word_ws();
	return first;
}

// move string to the next attribute of the tag
strref XMLNextAttribute(strref attribute) {
	attribute = attribute.after('=');
	if (attribute) {
		attribute.skip_whitespace();
		char q = attribute.get_first();
		if (q=='"' || q=='\'') {
			++attribute;
			return attribute.after(q).get_skip_ws();
		}
	}
	return strref();
}

// get the name of the current attribute
strref XMLAttributeName(strref attribute) {
	strref name = attribute.before('=');
	name.trim_whitespace();
	return name;
}

// get the value of the current attribute
strref XMLAttributeValue(strref attribute) {
	attribute = attribute.after('=');
	if (attribute) {
		attribute.skip_whitespace();
		char q = attribute.get_first();
		if (q=='"' || q=='\'') {
			++attribute;
			return attribute.before(q);
		}
	}
	return strref();
}
