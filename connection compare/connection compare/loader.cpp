#include "connections.h"


template<typename T>
void serialize(FILE* file, T toWrite) {
	static_assert(std::is_trivially_copyable<T>::value, "");
	fwrite(&toWrite, sizeof(toWrite), 1, file);
}

template<typename T>
void serialize(FILE* file, std::vector<T> toWrite) {
	auto size = toWrite.size();
	fwrite(&size, sizeof(toWrite.size()), 1, file);
	for (int i = 0; i < toWrite.size(); ++i) {
		serialize(file, toWrite[i]);
	}
}

void serialize(FILE* file, ConnectionList toWrite) {
	fwrite(&toWrite.numIntersections, sizeof(toWrite.numIntersections), 1, file);
	serialize(file, toWrite.c);
}

template<typename T>
void deserialize(FILE* file, T* toRead) {
	static_assert(std::is_trivially_copyable<T>::value, "");
	fread(toRead, sizeof(T), 1, file);
}

template<typename Y>
void deserialize(FILE* file, std::vector<Y>* toRead) {
	
	typename std::vector<Y>::size_type size;
	fread(&size, sizeof(size), 1, file);
	toRead->resize(size);
	
	for (int i = 0; i < size; ++i) {
		deserialize(file, &toRead[i]);
	}
}
void deserialize(FILE* file, ConnectionList* toRead) {
	fread(&toRead->numIntersections, sizeof(toRead->numIntersections), 1, file);
	deserialize(file, &toRead->c);
}



void writeConnectionLists(const char* filename, std::vector<ConnectionList> connections) {
	FILE* file = fopen(filename, "w");
	serialize(file, connections);
}
std::vector<ConnectionList> readConnectionLists(const char* filename) {
	FILE* file = fopen(filename, "r");
	
	std::vector<ConnectionList> toReturn;
	deserialize(file, &toReturn);
	return toReturn;
}
