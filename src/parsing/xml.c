typedef struct XmlParser XmlParser;
struct XmlParser {
	XmlDoc *doc;
	String8 data;
	U64 pos;
};

internal U8
peek(XmlParser *p)
{
	return p->data.str[p->pos];
}

internal B32
xml_fail(XmlParser *p, char *msg)
{
	if (p->doc->error.size == 0)
	{
		p->doc->error = str8_cstring(msg);
		p->doc->error_offset = p->pos;
	}
	return 0;
}

internal B32
xml_at_end(XmlParser *p)
{
	return p->pos >= p->data.size;
}

internal B32
xml_looking_at(XmlParser *p, char *lit)
{
	String8 s = str8_cstring(lit);
	return p->pos + s.size <= p->data.size &&
		str8_match(str8_substr(p->data, r1u64(p->pos, p->pos + s.size)), s, 0);
}

// Advances past the next occurrence of `lit`. Fails if there is none.
internal B32
xml_skip_past(XmlParser *p, char *lit)
{
	String8 needle = str8_cstring(lit);
	U64 at = str8_find_needle(p->data, p->pos, needle, 0);
	if (at >= p->data.size) { return xml_fail(p, "unterminated construct"); }
	p->pos = at + needle.size;
	return 1;
}

internal void
xml_skip_space(XmlParser *p)
{
	while (!xml_at_end(p) && char_is_space(peek(p))) { p->pos += 1; }
}

internal B32
xml_is_name_char(U8 c)
{
	return char_is_alpha(c) || char_is_digit(c, 10) || c == '_' || c == ':' || c == '-' || c == '.';
}

internal String8
xml_parse_name(XmlParser *p)
{
	U64 start = p->pos;
	if (!xml_at_end(p) && (char_is_alpha(peek(p)) || peek(p) == '_' || peek(p) == ':'))
	{
		p->pos += 1;
		while (!xml_at_end(p) && xml_is_name_char(peek(p))) { p->pos += 1; }
	}
	return str8_substr(p->data, r1u64(start, p->pos));
}

internal void
xml_append_child(XmlNode *parent, XmlNode *child)
{
	child->parent = parent;
	if (parent->last_child) { parent->last_child->next = child; } else { parent->first_child = child; }
	parent->last_child = child;
}

internal B32
xml_parse_attrs(XmlParser *p, XmlNode *el)
{
	for (;;)
	{
		xml_skip_space(p);

		if (xml_at_end(p)) { return xml_fail(p, "unexpected end inside tag"); }

		U8 c = peek(p);
		if (c == '>' || c == '/') { return 1; }

		String8 name = xml_parse_name(p);

		if (name.size == 0) { return xml_fail(p, "expected attribute name"); }
		xml_skip_space(p);

		if (xml_at_end(p) || peek(p) != '=') { return xml_fail(p, "expected '='"); }

		p->pos += 1;

		xml_skip_space(p);
		if (xml_at_end(p) || (peek(p) != '"' && peek(p) != '\''))
		{
			return xml_fail(p, "expected quoted attribute value");
		}

		U8 quote = p->data.str[p->pos++];
		U64 start = p->pos;

		while (!xml_at_end(p) && peek(p) != quote) { p->pos += 1; }
		if (xml_at_end(p)) { return xml_fail(p, "unterminated attribute value"); }

		XmlAttr *attr = push_array(p->doc->arena, XmlAttr, 1);
		attr->name = str8_copy(p->doc->arena, name);
		attr->value = str8_copy(p->doc->arena, str8_substr(p->data, r1u64(start, p->pos)));
		p->pos += 1;

		if (el->last_attr)
		{
			el->last_attr->next = attr;
		}
		else
		{ 
			el->first_attr = attr; 
		}

		el->last_attr = attr;
	}
}

internal void
xml_add_text(XmlParser *p, XmlNode *parent, String8 text)
{
	XmlNode *t = push_array(p->doc->arena, XmlNode, 1);
	t->kind = XmlNodeKind_Text;
	t->text = str8_copy(p->doc->arena, text);
	xml_append_child(parent, t);
}

internal XmlDoc *
xml_parse(String8 data)
{
	Arena *arena = arena_alloc();
	XmlDoc *doc = push_array(arena, XmlDoc, 1);
	doc->arena = arena;
	XmlParser parser = { doc, data, 0 };
	XmlParser *p = &parser;

	XmlNode *cur = 0;  // Currently open element.
	B32 ok = 1;
	while (ok && !xml_at_end(p))
	{
		if (peek(p) != '<')
		{
			U64 start = p->pos;
			while (!xml_at_end(p) && peek(p) != '<') { p->pos += 1; }
			String8 text = str8_substr(data, r1u64(start, p->pos));

			if (str8_skip_chop_whitespace(text).size == 0) { continue; }

			if (cur) { xml_add_text(p, cur, text); }
			else { p->pos = start; ok = xml_fail(p, "text outside root element"); }
		}
		// Skipping over comments and <? ?> symbols.
		else if (xml_looking_at(p, "<!--")) { p->pos += 4; ok = xml_skip_past(p, "-->"); }
		else if (xml_looking_at(p, "<?"))   { p->pos += 2; ok = xml_skip_past(p, "?>"); }
		else if (xml_looking_at(p, "<![CDATA["))
		{
			p->pos += 9;
			U64 start = p->pos;

			if (!cur) { ok = xml_fail(p, "CDATA outside root element"); }
			else if ((ok = xml_skip_past(p, "]]>")))
			{
				xml_add_text(p, cur, str8_substr(data, r1u64(start, p->pos - 3)));
			}
		}
		else if (xml_looking_at(p, "</"))
		{
			p->pos += 2;
			String8 name = xml_parse_name(p);
			xml_skip_space(p);

			if (!cur) { ok = xml_fail(p, "unexpected closing tag"); }
			else if (xml_at_end(p) || peek(p) != '>') { ok = xml_fail(p, "malformed closing tag"); }
			else if (!str8_match(name, cur->name, 0)) { ok = xml_fail(p, "mismatched closing tag"); }
			else { p->pos += 1; cur = cur->parent; }
		}
		else
		{
			p->pos += 1;
			String8 name = xml_parse_name(p);
			if (name.size == 0) { ok = xml_fail(p, "expected element name"); break; }
			if (!cur && doc->root) { ok = xml_fail(p, "multiple root elements"); break; }

			XmlNode *el = push_array(arena, XmlNode, 1);
			el->kind = XmlNodeKind_Element;
			el->name = str8_copy(arena, name);
			if (!(ok = xml_parse_attrs(p, el))) { break; }

			if (cur) { xml_append_child(cur, el); } else { doc->root = el; }

			if (xml_looking_at(p, "/>"))     { p->pos += 2; }
			else if (xml_looking_at(p, ">")) { p->pos += 1; cur = el; }
			else { ok = xml_fail(p, "malformed tag"); }
		}
	}

	if (ok && cur)        { ok = xml_fail(p, "unclosed element"); }
	if (ok && !doc->root) { ok = xml_fail(p, "no root element"); }
	if (!ok) { doc->root = 0; }

	return doc;
}

internal void
xml_release(XmlDoc *doc)
{
	arena_release(doc->arena);
}

internal XmlNode *
xml_child(XmlNode *node, String8 name)
{
	for (XmlNode *n = node ? node->first_child : 0; n != 0; n = n->next)
	{
		if (n->kind == XmlNodeKind_Element && str8_match(n->name, name, 0)) { return n; }
	}
	return 0;
}

internal String8
xml_attr_value(XmlNode *node, String8 name)
{
	for (XmlAttr *a = node ? node->first_attr : 0; a != 0; a = a->next)
	{
		if (str8_match(a->name, name, 0)) { return a->value; }
	}
	return str8_zero();
}
