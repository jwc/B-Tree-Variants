#include <cassert>
#include <iostream>

#define PAGE_SIZE 64
#define VALUE_SIZE 8

class Tree {
    public:
        explicit Tree(std::string filename) {
            open(filename);
        }

        ~Tree() {
            close();
        }

        void open(std::string filename) {
            numReads = 0;
            numWrites = 0;

            database = fopen(filename.c_str(), "wb+");
            assert(database);
        }

        void write(int, char[VALUE_SIZE]) {
            return;
        }

        char * read(int) {
            return new char[VALUE_SIZE]();
        }

        void erase(int) {
            return;
        }

        void close() {
            fclose(database);
            database = NULL;
        }

        int getNumWrites() {
            return numWrites;
        }

        int getNumReads() {
            return numReads;
        }

    protected:
        FILE * database;
        int numWrites;
        int numReads;
};