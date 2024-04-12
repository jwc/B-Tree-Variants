#include <iostream>
#ifndef _TREE_H
#define _TREE_H 1

#include <fcntl.h>
#include <unistd.h>

// #define DEBUG

#ifdef DEBUG
#include <cassert>
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 64
#endif

#ifndef VALUE_SIZE
#define VALUE_SIZE 8
#endif

#define TREE

class Tree {
    public:
        explicit Tree(std::string filename) {
            database = 0;
            numReads = 0;
            numWrites = 0;
        }

        virtual ~Tree() { ; }

        virtual void open(std::string filename) = 0;

        virtual void write(int, char[VALUE_SIZE]) = 0;

        virtual char * read(int) = 0; 

        virtual void erase(int) = 0;

        virtual void close() = 0;

        virtual char * getPage(int pos) {
            char * page = new char[PAGE_SIZE]();
            lseek(database, PAGE_SIZE * pos, SEEK_SET);
            ::read(database, page, PAGE_SIZE);
            numReads++;

            return page;
        }

        virtual void setPage(int pos, void * page) {
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
        
        #ifdef DEBUG
        virtual int getCardinality() = 0;

        virtual void printTree() = 0;
        #endif

    protected:
        int database;
        int numWrites;
        int numReads;
};

#endif // #ifndef _TREE_H

