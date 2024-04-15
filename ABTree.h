#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cassert>
#include <random>
#include <cmath>
#include <bitset>
#include "tree.h"

// table name is <schema-name>.<table-name> e.g. mydb.table1
#define SCHEMA_NAME_MAX_LEN 32
#define TABLE_NAME_MAX_LEN 32
#define TABLE_SUFFIX "dat"  // filename for table data: mydb.table1.dat
#define TABLE_SUFFIX_LEN 3
#define ABT_PAGE_SIZE 512 // PAGE_SIZE
#define PAGE0_VERSION 1
#define MAX_VAL_LEN VALUE_SIZE

#define DEFAULT_ROOT_PAGE_ID 2  // initial root page number

// error codes
#define RC_DUPLICATE_KEY 32
#define RC_NOT_FOUND 33

// node values
#define MAX_NUM_KEYS (3)
#define MAX_NUM_BUCKETS MAX_NUM_KEYS + 1
#define MAX_BUCKET_SIZE 4

static_assert(MAX_BUCKET_SIZE > MAX_NUM_KEYS, "MAX_BUCKET_SIZE must be greater than MAX_NUM_KEYS");
static_assert(MAX_NUM_KEYS <= 8, "MAX_NUM_KEYS must be less than 8");

extern int debug;
extern char g_search_buff[];
extern bool insertIsReplace;

struct Table;

typedef struct ABTree : public Tree {
    Table *table;
    bool tableOpened;
    ABTree(const char * schema, const char * tableName, bool doCreateTable=true);
    ~ABTree();
    void write(int key, char value[MAX_VAL_LEN]) override;
    char * read(int key) override;
    void erase(int key) override;

    #ifdef DEBUG
    int getCardinality() override;
    #endif

    void open(std::string filename) override {
        // Dummy implementation (no actual functionality)
        return;
    }


    void close() override {
        // Dummy implementation (no actual functionality)
        return;
    }

    #ifdef DEBUG
    void printTree() override {
        // Dummy implementation (no actual functionality)
        return;
    }
    #endif

    
} ABTree;

int abtTest1();
int abtTest2();
int abtTest3();
