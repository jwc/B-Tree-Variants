#include <cstring>
#include <vector>
#include <set>

#ifdef UBPTREE
#include "ubpTree.h"
#endif
#ifdef ASBTREE
#include "asbtree.h"
#endif
#ifdef ABTREE
#include "ABTree.h"
#endif
#ifndef TREE
#include "tree.h"
#endif

#ifndef COUNT
#define COUNT 10000
#endif

using namespace std;

// Tests reopening and Reading an existing tree.

int main() {
    vector<int> testValues;
    set<int> allKeys;
    Tree * t = NULL;

    #ifdef UBPTREE
    t = new uBPlusTree("ubp1.db");
    #endif
    #ifdef ASBTREE
    //t = new ASBTree("asbtree");
    delete t;
    return 0;
    #endif
    #ifdef ABTREE
    t = new ABTree("mydb", "table1", false);
    #endif

    #ifdef DEBUG
    assert(t);
    // t->printTree();
    #endif
    
    // Generating all values that should be in the tree.
    for (int i = 0; allKeys.size() < COUNT; i++) {
        int x = rand();
        x = rand() % 2 ? -x : x;

        testValues.push_back(x);
        allKeys.insert(x);
    }

    #ifdef DEBUG
    cout << "\tExp. Card: " << allKeys.size() << endl;
    assert(allKeys.size() == t->getCardinality());
    #endif
    
    // Reading values from the tree. 
    char * temp = NULL;
    for (auto &i : testValues) {
        temp = t->read(i);
        string y = to_string(i);
        char * z = new char[VALUE_SIZE]();
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
    
    t->close();

    cout << "    Pages Wrote: " << t->getNumWrites();
    cout << "\n    Pages  Read: " << t->getNumReads() << endl;

    delete t;
    return 0;
}