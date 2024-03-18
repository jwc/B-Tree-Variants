#include <cassert>
#include <iostream>
#include <fcntl.h>

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

            database = ::open(filename.c_str(), O_RDWR | O_CREAT);
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
            ::close(database);
            database = 0;
        }

        char * getPage(int pos) {
            char * page = new char[PAGE_SIZE]();
            lseek(database, PAGE_SIZE * pos, SEEK_SET);
            ::read(database, page, PAGE_SIZE);
            numReads++;

            return page;
        }

        void setPage(int pos, void * page) {
            lseek(database, PAGE_SIZE * pos, SEEK_SET);
            ::write(database, page, PAGE_SIZE);
            numWrites++;
        }

        int getNumWrites() {
            return numWrites;
        }

        int getNumReads() {
            return numReads;
        }

    protected:
        int database;
        int numWrites;
        int numReads;
};