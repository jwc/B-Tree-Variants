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

// Tests reopening and Deleting from an existing tree.

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
    assert(allKeys.size() == t->getCardinality());
    #endif
    
    // Deleting all values from the tree. Note: there are repeats.
    char * temp = NULL;
    for (auto &i : testValues) {
        // cout << "DELETE(" << i << ")" << endl;
        t->erase(i);

        if (allKeys.find(i) != allKeys.end()) {
            allKeys.erase(allKeys.find(i));
        }

        #ifdef DEBUG
        // t->printTree();
        // cout << "\tExp. Card: " << allKeys.size() << endl;
        assert(allKeys.size() == t->getCardinality());
        #endif

    }

    #ifdef DEBUG
    // t->printTree();
    // cout << "\tExp. Card: " << allKeys.size() << endl;
    assert(0 == t->getCardinality());
    #endif
    
    t->close();

    cout << "    Pages Wrote: " << t->getNumWrites();
    cout << "\n    Pages  Read: " << t->getNumReads() << endl;

    delete t;
    return 0;
}