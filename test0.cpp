#include <sys/stat.h>

#ifdef UBPTREE
#include "ubpTree.h"
#endif
#ifdef ASBTREE
#include ""
#endif
#ifdef ABTREE
#include ""
#endif
#ifndef TREE
#include "tree.h"
#endif

using namespace std;

int main() {
    cout << "INT_SIZE: " << sizeof(int) << endl;
    cout << "PTR_SIZE: " << sizeof(char *) << endl;
    cout << "PAGE_SIZE: " << PAGE_SIZE << endl;
    cout << "VALUE_SIZE: " << VALUE_SIZE << endl;

    #ifdef UBPTREE
    cout << "NUM_LEAF_VALS: " << NUM_LEAF_VALS << "\n";
    cout << "NUM_BRANCH_VALS: " << NUM_BRANCH_VALS << "\n";
    cout << "ENUM: " << sizeof(enum NodeType) << "\n";
    cout << "struct Leaf: " << sizeof(struct Leaf) << "\n";
    cout << "struct Branch: " << sizeof(struct Branch) << endl;
    cout << "struct Node: " << sizeof(Node) << "\n";
    cout << "struct Tree: " << sizeof(uBPlusTree) << "\n";
    #endif

    struct stat fileStats;
    stat("Makefile", &fileStats);
    cout << "File System block size: " << fileStats.st_blksize << endl;

    return 0;
}