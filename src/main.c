// Used only to compile the library.

#define BUILD_CONSOLE_INTERFACE 1
#include "base/base_inc.h"
#include "base/base_inc.c"

internal void
entry_point(CmdLine *cmd) 
{
	HashTable *ht = hash_table_alloc(64);
	B32 added = hash_table_add(ht, 1, 0);          // 1: inserted
	B32 added_again = hash_table_add(ht, 1, 0);    // 0: duplicate
	B32 contains = hash_table_contains(ht, 1);     // 1
	hash_table_release(ht);

	Arena *arena    = arena_alloc();
	String8 content = data_from_file_path(arena, str8_lit("C:\\DEV\\c\\github-castle\\src\\basic.xml"));
	XmlDoc *xml     = xml_parse(content);
	printf("%.*s\n", str8_varg(xml->root->name));

	xml_release(xml);

}
