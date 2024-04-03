#include <cstring>
#include <vector>
#include <set>

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

#ifndef COUNT
#define COUNT 1000
#endif

using namespace std;

// Tests Reading and Writing from a new tree.

int main() {
    vector<int> testValues;
    set<int> allKeys;
    Tree * t = NULL;

    #ifdef UBPTREE
    t = new uBPlusTree("ubp2.txt");
    #endif
    #ifdef ASBTREE
    t = NULL;
    #endif
    #ifdef ABTREE
    t = NULL;
    #endif

    #ifdef DEBUG
    assert(t);
    #endif

    // Writing random values to the tree.
    for (int i = 0; allKeys.size() < COUNT; i++) {
        int x = rand();
        x = rand() % 2 ? -x : x;
        testValues.push_back(x);
        allKeys.insert(x);

        string y = to_string(x);
        char * z = new char[VALUE_SIZE]();
        memset(z, '\0', VALUE_SIZE);
        strcpy(z, y.c_str());

        #ifdef DEBUG
        assert(strlen(z) < VALUE_SIZE);
        #endif

        t->write(x, z);
    }

    // Reading values from the tree. 
    char * temp = NULL;
    for (auto &i : allKeys) {
        temp = t->read(i);
        string y = to_string(i);
        char * z = new char[VALUE_SIZE]();
        memset(z, '\0', VALUE_SIZE);
        strcpy(z, y.c_str());

        if (z != NULL && temp != NULL) {
            if (strcmp(z, temp) != 0) {
                cerr << "ERROR: Incorrect Value. Expected:" << z << ", Returned:" << temp << endl;

                #ifdef DEBUG
                assert(false);
                #endif
            }
        } else {
            cerr << "ERROR: Null Value" << endl;

            #ifdef DEBUG
            assert(false);
            #endif
        }
    }

    #ifdef DEBUG
    // t->printTree();
    // cout << "\tExp. Card: " << allKeys.size() << endl;
    assert(allKeys.size() == t->getCardinality());
    #endif

    cout << "    Pages Wrote: " << t->getNumWrites();
    cout << "\n    Pages  Read: " << t->getNumReads() << endl;

    delete t;
    return 0;
}