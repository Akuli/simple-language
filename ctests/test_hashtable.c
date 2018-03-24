#include <src/common.h>
#include <src/hashtable.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

int intcmp(void *a, void *b, void *userdata)
{
	buttert(userdata == (void *)0xdeadbeef);
	return (*((int *)a)) == (*((int *)b));
}

unsigned long inthash(int i) {
	return i + 20;
}


void test_basic_stuff(void)
{
	int i=1, j=2, k=3;
	int *ptr;
	struct HashTable *ht = hashtable_new(intcmp);
	buttert(ht);
	buttert(ht->size == 0);

	buttert(hashtable_set(ht, &i, inthash(i), &j, (void *)0xdeadbeef) == STATUS_OK);
	buttert(hashtable_get(ht, &i, inthash(i), (void **)(&ptr), (void *)0xdeadbeef) == 1);
	buttert(ptr == &j && j == 2);
	buttert(ht->size == 1);

	buttert(hashtable_set(ht, &i, inthash(i), &k, (void *)0xdeadbeef) == STATUS_OK);
	buttert(hashtable_get(ht, &i, inthash(i), (void **)(&ptr), (void *)0xdeadbeef) == 1);
	buttert(ptr == &k && k == 3);
	buttert(ht->size == 1);

	// test trying to get a missing item
	buttert(hashtable_get(ht, &j, inthash(i), (void **)(&ptr), (void *)0xdeadbeef) == 0);
	buttert(ptr == &k && k == 3);   // didn't change
	buttert(ht->size == 1);   // didn't change

	hashtable_free(ht);   // should clear the table
}

#define HOW_MANY 1000
void test_many_values(void)
{
	int *keys[HOW_MANY];
	int keyhashes[HOW_MANY];
	char *values[HOW_MANY];

	struct HashTable *ht = hashtable_new(intcmp);
	buttert(ht);
	for (int i = 0; i < HOW_MANY; i++) {
		keys[i] = bmalloc(sizeof(int));
		*keys[i] = i;
		keyhashes[i] = inthash(i);
		values[i] = bmalloc(10);
		sprintf(values[i], "lol %d", i);
	}

	char *ptr;
	int counter = 3;
	while (counter--) {   // repeat 3 times
		for (int i = 0; i < HOW_MANY; i++) {
			buttert(hashtable_set(ht, keys[i], keyhashes[i], values[i], (void *)0xdeadbeef) == STATUS_OK);
		}
		buttert(ht->size == HOW_MANY);
		for (int i = 0; i < HOW_MANY; i++) {
			buttert(hashtable_get(ht, keys[i], keyhashes[i], (void **)(&ptr), (void *)0xdeadbeef) == 1);
			buttert(ptr == values[i]);
		}
		buttert(ht->size == HOW_MANY);
		hashtable_clear(ht);
		buttert(ht->size == 0);
	}

	hashtable_free(ht);
	for (int i = 0; i < HOW_MANY; i++) {
		free(keys[i]);
		free(values[i]);
	}
}
#undef HOW_MANY

#define HOW_MANY 3
void test_iterating(void)
{
	int *keys[HOW_MANY];
	int *values[HOW_MANY];
	int got[HOW_MANY] = {0};

	struct HashTable *ht = hashtable_new(intcmp);
	buttert(ht);
	for (int i = 0; i < HOW_MANY; i++) {
		keys[i] = bmalloc(sizeof(int));
		*keys[i] = i;
		values[i] = bmalloc(sizeof(int));
		*values[i] = i + 10;
		buttert(hashtable_set(ht, keys[i], i, values[i], (void *)0xdeadbeef) == STATUS_OK);
	}

	struct HashTableIterator iter;
	hashtable_iterbegin(ht, &iter);
	for (int i=0; i < HOW_MANY; i++) {
		buttert(hashtable_iternext(&iter) == 1);
		buttert(*((int*)iter.value) == *((int*)iter.key)+10);
		got[*((int*)iter.key)] = 1;
	}
	buttert(hashtable_iternext(&iter) == 0);
	for (int i=0; i < HOW_MANY; i++)
		buttert(got[i]);

	hashtable_free(ht);
	for (int i=0; i < HOW_MANY; i++) {
		free(keys[i]);
		free(values[i]);
	}
}
#undef HOW_MANY

TESTS_MAIN(test_basic_stuff, test_many_values, test_iterating);
