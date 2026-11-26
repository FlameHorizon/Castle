#ifndef PARSING_XML_H
#define PARSING_XML_H

// Minimal XML parser: elements, attributes, text and CDATA.
// <?...?> and <!--...--> are skipped. No entities, no DOCTYPE.
// Whitespace-only text between elements is dropped.

typedef enum XmlNodeKind {
	XmlNodeKind_Element,
	XmlNodeKind_Text,   // Plain text and CDATA alike.
} XmlNodeKind;

typedef struct XmlAttr XmlAttr;
struct XmlAttr {
	XmlAttr *next;
	String8 name;
	String8 value;
};

typedef struct XmlNode XmlNode;
struct XmlNode {
	XmlNode *parent;
	XmlNode *first_child;
	XmlNode *last_child;
	XmlNode *next;
	XmlNodeKind kind;
	String8 name;  // Element only.
	String8 text;  // Text only.
	XmlAttr *first_attr;
	XmlAttr *last_attr;
};

typedef struct XmlDoc XmlDoc;
struct XmlDoc {
	Arena *arena;      // Owns the doc and every node/string in it.
	XmlNode *root;     // 0 if parsing failed.
	String8 error;     // Empty on success.
	U64 error_offset;  // Byte offset into the input where parsing failed.
};

internal XmlDoc *xml_parse(String8 data);  // Always returns a doc; check doc->root.
internal void xml_release(XmlDoc *doc);
internal XmlNode *xml_child(XmlNode *node, String8 name);
internal String8 xml_attr_value(XmlNode *node, String8 name);  // Empty if missing.

#endif // PARSING_XML_H
