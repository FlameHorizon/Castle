internal HashTable *
hash_table_alloc(U64 bucket_count)
{
    Assert(IsPow2(bucket_count));
    Arena *arena = arena_alloc();
    HashTable *ht = push_array(arena, HashTable, 1);
    ht->arena = arena;
    ht->bucket_count = bucket_count;
    ht->buckets = push_array(arena, HashTableNode *, bucket_count);
    return ht;
}

internal void
hash_table_release(HashTable *ht)
{
    arena_release(ht->arena);
}

internal HashTableNode *
hash_table_lookup(HashTable *ht, U64 key)
{
    U64 hash = u64_hash_from_u64(key);
    U64 slot = hash & (ht->bucket_count - 1);
    for (HashTableNode *node = ht->buckets[slot]; node != 0; node = node->next)
    {
        if (node->hash == hash && node->key == key)
        {
            return node;
        }
    }
    return 0;
}

internal B32
hash_table_contains(HashTable *ht, U64 key)
{
    return hash_table_lookup(ht, key) != 0;
}

// Returns 1 if inserted, 0 if the key was already present (table unchanged).
internal B32
hash_table_add(HashTable *ht, U64 key, void *value)
{
    if (hash_table_contains(ht, key))
    {
        return 0;
    }
    U64 hash = u64_hash_from_u64(key);
    U64 slot = hash & (ht->bucket_count - 1);

    HashTableNode *node = push_array(ht->arena, HashTableNode, 1);
    node->key = key;
    node->hash = hash;
    node->value = value;
    SLLStackPush(ht->buckets[slot], node);
    ht->key_count += 1;
    return 1;
}
