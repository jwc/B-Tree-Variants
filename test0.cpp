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
    string space = "     ";
    cout << space << "sizeof(int): " << sizeof(int) << endl;
    cout << space << "sizeof(char *): " << sizeof(char *) << endl;
    cout << space << "PAGE_SIZE: " << PAGE_SIZE << endl;
    cout << space << "VALUE_SIZE: " << VALUE_SIZE << endl;

    #ifdef UBPTREE
    cout << space << "NUM_LEAF_VALS: " << NUM_LEAF_VALS << "\n";
    cout << space << "NUM_BRANCH_VALS: " << NUM_BRANCH_VALS << "\n";
    cout << space << "FREELIST_SIZE: " << FREELIST_SIZE << "\n";
    cout << space << "sizeof(enum NodeType): " << sizeof(enum NodeType) << "\n";
    cout << space << "sizeof(struct Leaf): " << sizeof(struct Leaf) << "\n";
    cout << space << "sizeof(struct Branch): " << sizeof(struct Branch) << endl;
    cout << space << "sizeof(Node): " << sizeof(Node) << "\n";
    cout << space << "sizeof(uBPlusTree): " << sizeof(uBPlusTree) << "\n";
    #endif

    struct stat fileStats;
    stat("Makefile", &fileStats);
    cout << space << "File System block size: " << fileStats.st_blksize << endl;

    return 0;
}