#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h> //log10
#include <assert.h>
#include <sys/types.h> //stat
#include <sys/stat.h> //stat


//all these are example values
#define DEFAULT_LEN_KEY 3
#define DEFAULT_LEN_VALUE 6

/*
The overhead is the number of bytes in an entry that is neither key nor value.
1 for newline
4 for 0xdeadd00d or "emty"
1 for : which seperates key and value
2 for [ and ] which go around index number
NEED TO TAKE INTO ACCOUNT LENGTH OF INDEX NUMBER
+1 for single digit of index
*/
#define ENTRY_OVERHEAD 9

#define DEFAULT_LEN_ENTRY (DEFAULT_LEN_KEY + DEFAULT_LEN_VALUE + ENTRY_OVERHEAD)

/*
Fills space in an empty entry. An empty byte must:
1. take up a byte, obv.
2. not print when you do cat. (It will show up in an editor.)

I was going to use \0 but printf can't write it and it was a pain to use write.
\a is the bell character. It shows up as ^G in editors.
*/
#define EMPTY_BYTE "\a"


FILE* store;
long hashTableStart;
int lenFile, numEntries, lenEntry, lenKey, lenValue, storefd;


/*
Writing 0xDEADD00D to a file results in 0x0DD0ADDE being in the file. You can look with a hex editor.
This happens because:
First, the number is reversed, because endian.
Then, each byte is read backwards again, because fuck you.

To get 0xdeadd00d in the file, write 0x0dd0adde

I'm not sure which one I will use.
*/
const int MAGIC_NUM = 0xdeadd00d;

/**
* djb2 hash algorithm by Dan Bernstein
* from http://www.cse.yorku.ca/~oz/hash.html
*/
unsigned long hash(char *str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	printf("hash=%zu\n", hash);
	return hash;
}

int getIndex(char* key) {
	int index = hash(key) % numEntries;
	printf("index=%d\n", index);
	return index;
}


/**
Function initNewFile
Make a store from a new file.
Helper function to initialize().

length: total length of the file.
size: number of entries in the hash table.

Returns: 0 on success, -1 on error.
*/
int initNewFile(int length, int size){

	if(	fprintf(store, "lenFile=%d\n", length) <= 0 ||
		fprintf(store, "numEntries=%d\n", size) <= 0 ||
		fprintf(store, "lenEntry=%d\n", DEFAULT_LEN_ENTRY) <= 0 ||
		fprintf(store, "lenKey=%d\n", DEFAULT_LEN_KEY) <= 0 ||
		fprintf(store, "lenValue=%d\n", DEFAULT_LEN_VALUE) <= 0
	) {
		printf("Couldn't write header.\n");
		return -1;
	}

	lenFile = length;
	numEntries = size;
	lenEntry = DEFAULT_LEN_ENTRY;
	lenKey = DEFAULT_LEN_KEY;
	lenValue = DEFAULT_LEN_VALUE;

		//save where the end of the header is
	hashTableStart = ftell(store);

	//generate a format string for human-readable index
	char formatString[10];
	sprintf(formatString, "emty[%%0%dd]", (int)log(numEntries)); //"emty" replaces 0xdeadd00d


	//write empty hash table
	for(int i=0; i<numEntries; i++) {

		// (1) Write human-readable index.
		fprintf(store, formatString, i);

		// (2) Write empty key.
		for(int j=0; j<lenKey; j++){
			fprintf(store, EMPTY_BYTE);
		}

		// (3) Seperate key and value.
		fprintf(store, "%s", ":");

		// (4) Write empty value.
		for(int j=0; j<lenValue; j++){
			fprintf(store, EMPTY_BYTE);
		}

		fprintf(store, "\n");

	}

	//todo maybe: see if the size of the file matches




	fflush(store);

	return 0;
}

/**
Function initExistingFile
Load an existing store from a file.
Helper function to initialize().

length: total length of the file.
size: number of entries in the hash table.

Returns: 0 on success, -1 on error.
*/
int initExistingFile(int length, int size){

	if(	fscanf(store, "lenFile=%d\n", &lenFile) <= 0 ||
		fscanf(store, "numEntries=%d\n", &numEntries) <= 0 ||
		fscanf(store, "lenEntry=%d\n", &lenEntry) <= 0 ||
		fscanf(store, "lenKey=%d\n", &lenKey) <= 0 ||
		fscanf(store, "lenValue=%d\n", &lenValue) <= 0
	) {
		fprintf(stderr, "Couldn't read header.\n");
		return -1;
	}


	if(length != lenFile) {
		fprintf(stderr, "Specefied length was %d, but length of file is %d.\n", length, lenFile);
		return -1;
	}

	if(size != numEntries){
		fprintf(stderr, "Specefied size was %d, but size of file is %d.\n", size, numEntries);
		return -1;
	}

	if(lenKey + lenValue + ENTRY_OVERHEAD != lenEntry){
		fprintf(stderr, "Length of entry does not add up.\n");
		return -1;
	}

		//save where the end of the header is
	hashTableStart = ftell(store);


	/*
	struct stat fileStat = malloc(sizeof(struct stat));
	assert(fileStat != NULL);
	if((fstat(storefd, fileStat)) == -1) {
		fprintf(stderr, "Couldn't stat.\n");
		return -1;
	}
	off_t size = fileStat.st_size; //size in bytes in file
	if(lenFile != size){
		fpritnf(stderr, "corrupt file. File should be %d bytes but is %d bytes.\n", lenFile, size);
		return -1;
	}
	*/

	return 0;
}

/**
Function insert
key:
value:
length:

Returns: slot location of where the value is stored, or -1 if the hash table is full or error.
*/
int insert(char* key, void* value, int length) {

	if(strlen(key)==0){
		fprintf(stderr, "Insert needs a key.\n");
		return -1;
	}

	int len = (int)strlen(key);
	if(len > lenKey) {
		fprintf(stderr, "Key length is %d, but maximum length is %d.\n", (int)len, lenKey);
		return -1;
	}

	if(length > lenValue) {
		fprintf(stderr, "Value length is %d, but maximum length is %d.\n", length, lenValue);
		return -1;
	}

	int index = getIndex(key);


	printf("hashTableStart=%d\nlenEntry=%d\nlenEntry*index=%d\nseek to %d\n", (int)hashTableStart, lenEntry,  lenEntry * index, (int)hashTableStart + (lenEntry * index));


	lseek(storefd, (int)hashTableStart + (lenEntry * index), SEEK_SET);

	int check = -1;

	read(storefd, &check, 4); //check for 0xdeadd00d or "emty"

	if(check == MAGIC_NUM) {
		printf("slot %d has somethign in it.\n", index);
		return index;
	} else if (check == 0x79746d65) { //"ytme" which is empty backwards
		printf("slot %d is empty.\n", index);
		lseek(storefd, -4, SEEK_CUR); //go back
		write(storefd, &MAGIC_NUM, 4);
		printf("wrote magic number!\n");

	} else {
		printf("neither thing, read %x\n", check);
	}

	value = value;
	length = length;



	return 0;
}

/**
Function initialize
Open or create a file that will contain the hash table.

length: total length of the file.
size: number of entries in the hash table.

Returns: a file descriptor, or -1 for error.
*/
int initialize(char* file, int length, int size) {

	/* apparently there is no open option which does all of these:
	1. open for reading and writing
	2. create if does not exist
	3. if does exist, do NOT truncate
	*/

	//Q note: another way to check if file exists is with if( access( "testfile", W_OK ) != -1 ) {//exists}else{//does not exist} location: #include <unistd.h>
	//open for reading and writing, without creation
	if((store = fopen(file, "r+")) == NULL) {

		/*
		If doesn't exist, re-do open with creation.
		This option would also truncate the file if it does exist,
		so we can't do it in the first step.
		*/
		if((store = fopen(file, "w+")) == NULL) {
			perror("Could not open file");
			return -1;
		} else {
			//created a file
			storefd = fileno(store);
			if((initNewFile(length, size)) == -1) {
				fprintf(stderr, "Couldn't init new file.\n");
				return -1;
			}
		}
	} else {
		//opened existing file
		storefd = fileno(store);
		if((initExistingFile(length, size)) == -1) {
			fprintf(stderr, "Couldn't init existing file.\n");
			return -1;
		}
	}
	//Q: will this work on both cases? Esp we should check the one after we have appended data to a file already?
	return storefd;
}



int main() {
	lenFile = numEntries = lenEntry = lenKey = lenValue = storefd = hashTableStart = -1;
	store = NULL;

	//31 and 12 are example numbers. length, numEntry
	if(initialize("example.store", 3, 4) == -1) {
		fprintf(stderr, "Couldn't initialize.\n");
		return EXIT_FAILURE;
	}

	insert("key", "value", 6);

	if(fclose(store) == EOF){
		perror("Couldn't close file.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
int fetch(char* key, void* value, int* length) {
        return 0;
}

int probe(char* key) {
        return 0;
}

int delete(char* key) {
        return 0;
}
*/




