FILE* initialize(char* name);
FILE* create_file(char* name);
FILE* access_file(char* name);
void populate(FILE* store);
int insert(FILE* store, char* key, void* value, int length);

/**
* djb2 hash algorithm by Dan Bernstein
* from http://www.cse.yorku.ca/~oz/hash.html
*/
unsigned long hash(char *str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}
