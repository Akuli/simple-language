#ifndef MAPPING_H
#define MAPPING_H

#include <stddef.h>

// should return 1 for a equals b, 0 for a does not equal b or something negative for error
// must not return 2 or greater
typedef int (*hashtable_cmpfunc)(void *a, void *b, void *userdata);

// should be considered an implementation detail
struct HashTableItem {
	unsigned long keyhash;    // longs might be useful for a huuuuuge hashtable
	void *key;
	void *value;
	struct HashTableItem *next;
};

// everything except size should be considered implementation details
struct HashTable {
	struct HashTableItem **buckets;
	size_t nbuckets;
	size_t size;
	hashtable_cmpfunc keycmp;
};

struct HashTable *hashtable_new(hashtable_cmpfunc keycmp);

// return STATUS_OK on success, and on failure STATUS_NOMEM or an error code from cmpfunc
int hashtable_set(struct HashTable *ht, void *key, unsigned long keyhash, void *value, void *userdata);

// return 1 if key was found, 0 if not (*res is not set) or an error code from cmpfunc
int hashtable_get(struct HashTable *ht, void *key, unsigned long keyhash, void **res, void *userdata);

// delete an item from the hashtable and set it to *res, or ignore it if res is NULL
// returns 1 if the key was found, 0 if not and an error code from cmpfunc on failure
int hashtable_pop(struct HashTable *ht, void *key, unsigned long keyhash, void **res, void *userdata);

// pop all keys, never fails
void hashtable_clear(struct HashTable *ht);

// deallocate a hashtable
void hashtable_free(struct HashTable *ht);

//int hashtable_contains(struct HashTable *ht, void *key, unsigned long keyhash, void *userdata);
//int hashtable_equals(struct HashTable *ht1, struct HashTable *ht2, hashtable_cmpfunc valuecmp, void *userdata);

//unsigned long hashtable_stringhash(unicode_t *string, size_t stringlen);

#endif   // MAPPING_H