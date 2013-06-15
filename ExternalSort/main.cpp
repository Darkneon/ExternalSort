#define _CRT_SECURE_NO_DEPRECATE

#include <iostream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#define BUFFER_SIZE 20
#define WRITES      1000

#define UINT_SIZE sizeof(unsigned int);

using namespace std;

int compare (const void * a, const void * b) {
  return ( *(int*)a - *(int*)b );
}

int minArray(unsigned int* array, int n, int* offset) {
	unsigned int result = array[0];
	*offset = 0;

	for(int i = 1; i != n; i++) {
		if (array[i] < result) {
			*offset = i;
			result  = array[i];			
		}
	}

	return result;
}

int test(const char* filename) {
	FILE *file = fopen(filename, "rb");
	unsigned int result = 0;
	unsigned int buffer = 0;

	int counter = 0;
	while (!feof(file))
	{
		int n = fread(&buffer, sizeof(unsigned int), 1, file);				

		if (n > 0) {					
			if (buffer < result) { return 0; }

			cout << "Elem : " << counter << " " << buffer << endl;

			result = buffer;
			counter += 1; 
		}
	}

	fclose(file);

	return 1;
}

void initOffsets(unsigned int *offset, unsigned int *maxoffset, int numberOfChunks, int totalElements) {
	for (int i = 0; i < numberOfChunks; i++ ) {
		offset[i]    = i * BUFFER_SIZE * sizeof(unsigned int);
		maxoffset[i] = (i + 1) * BUFFER_SIZE * sizeof(unsigned int);

		if (maxoffset[i] > totalElements * sizeof(unsigned int)) {
			maxoffset[i] = totalElements * sizeof(unsigned int);
		}
	}
}

void initValues(unsigned int *values, unsigned int *offset, int numberOfChunks, FILE* tempfile) {
	for(int i = 0; i < numberOfChunks; i++) {			
		fseek(tempfile, offset[i], SEEK_SET);
		fread(&values[i], sizeof(unsigned int), 1, tempfile);			
	}
}


void generateFile(string filepath) {		
	FILE* file = fopen(filepath.c_str(), "w+b");	

	unsigned int number = rand() % UINT_MAX;

	int writes = 0;
	while (writes < WRITES) {	
		cout << number << endl;
		fwrite(&number, sizeof(unsigned int), 1, file);
		number = rand() % UINT_MAX;
		writes += 1;		
	}

	fclose(file);
}


void sortChunks(FILE* infile, FILE* tempfile, int* chunks, int*totalElements) {
	unsigned int *memory = new unsigned int[BUFFER_SIZE];
	int read = -1;

	while ( !feof(infile) )
	{
		read = fread(memory,  sizeof(unsigned int), BUFFER_SIZE, infile);
		if (read > 0) {
			qsort (memory, read, sizeof(unsigned int), compare);
			fwrite(memory, sizeof(unsigned int), read, tempfile);
			
			*chunks		   += 1;
			*totalElements += read;
		}
	}
	
	delete[] memory;
}

void updateValues(FILE* tempfile, unsigned int* offset, unsigned int* maxoffset, unsigned int* values, int chunkOffset) {
	if (offset[chunkOffset] < maxoffset[chunkOffset]) { 
		//update value at offset
		fseek(tempfile, offset[chunkOffset], SEEK_SET);			
		fread(&values[chunkOffset], sizeof(unsigned int), 1, tempfile);			
	}
	else {
		values[chunkOffset] = UINT_MAX;
	}
}

void mergeChunks(FILE* tempfile, FILE* outfile, int numberOfChunks, int totalElements) {	
	unsigned int *offset    = new unsigned int[numberOfChunks];	 //for each chunk, were we start reading
	unsigned int *maxoffset = new unsigned int[numberOfChunks];  //max reading offset, so we don't overlap into another chunk
	unsigned int *values    = new unsigned int[numberOfChunks];

	initOffsets(offset, maxoffset, numberOfChunks, totalElements);
	initValues(values, offset, numberOfChunks, tempfile);	

	int min, chunkOffset;
	for(int i = 0; i != totalElements; i++) {
		min = minArray(values, numberOfChunks, &chunkOffset);
					
		fwrite(&values[chunkOffset], sizeof(unsigned int), 1, outfile);
		offset[chunkOffset] += sizeof(unsigned int);
		updateValues(tempfile, offset, maxoffset, values, chunkOffset);
	}		

	delete[] values;
	delete[] offset;
	delete[] maxoffset;	
}

void sort(string filepath)
{
	FILE *infile   = fopen(filepath.c_str(), "rb");
	FILE *outfile  = fopen((filepath + ".out").c_str(), "w+b");		
	FILE* tempfile = fopen((filepath + ".tmp").c_str(), "w+b");
		
	int numberOfChunks = 0;
	int totalElements  = 0;
		
	sortChunks(infile, tempfile, &numberOfChunks, &totalElements);

	fflush(tempfile); //so we don't start reading while there is still unwritten data in the buffer

	mergeChunks(tempfile, outfile, numberOfChunks, totalElements);

	fclose(tempfile);
	fclose(outfile);
	fclose(infile);			
}


int main(int args, char** argv) {
	string filepath;	
	int action = 0;
	srand((unsigned int)time(NULL));
	
	if (args == 3) {
		filepath += argv[1];
		action = atoi(argv[2]);
	}
	else {
		cout << "Forgot filename or action?";
		return 1;
	}

	switch (action)
	{
		case 0:
			generateFile(filepath);
			break;
		case 1: {
			sort(filepath);
			
			int result = test( (filepath + ".out").c_str());
			if (result) {
				cout << endl << "Test passed";
			}
			else {
				cout << endl << "Test failed";
			}
			break;
		}
		default:
			cout << "Invalid action value?";
			break;
	}

	return 0;
}
