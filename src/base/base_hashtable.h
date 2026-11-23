#ifndef BASE_HASH_TABLE_H
#define BASE_HASH_TABLE_H

typedef struct HashTableNode HashTableNode;
struct HashTableNode {
    HashTableNode *next;
    U64 key;
    U64 hash;
    void *value;
};

typedef struct HashTable HashTable;
struct HashTable {
    Arena *arena;     // Owns the table, the bucket array and all nodes.
    HashTableNode **buckets;
    U64 bucket_count; // Must be a power of 2.
    U64 key_count;
    HashTableNode *free_nodes; // Reserved for a future remove().
};

internal HashTable *hash_table_alloc(U64 bucket_count);
internal void hash_table_release(HashTable *ht);
internal B32 hash_table_add(HashTable *ht, U64 key, void *value);
internal HashTableNode *hash_table_lookup(HashTable *ht, U64 key);
internal B32 hash_table_contains(HashTable *ht, U64 key);

#endif // BASE_HASH_TABLE_H
