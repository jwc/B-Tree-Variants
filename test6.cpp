#include <cstring>
#include <vector>
#include <set>
#include <chrono>

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

#define MAX 100

using namespace std;

// Tests Writing, Reading and Deleting from a new tree.

int main() {
    set<int> allKeys;
    Tree * t = NULL;

    #ifdef UBPTREE
    t = new uBPlusTree("ubp6.db");
    #endif
    #ifdef ASBTREE
    t = new ASBTree("asbtree");
    #endif
    #ifdef ABTREE
    t = new ABTree("mydb", "table6", true);
    #endif

    #ifdef DEBUG
    assert(t);
    assert(t->getCardinality() == 0);
    #endif

    #ifndef DEBUG
    // Base values for the tree.
    for (int i = 0; i < COUNT; i++) {
        int x = rand();
        x = rand() % 2 ? -x : x;

        string y = to_string(x);
        char * z = new char[VALUE_SIZE]();
        memset(z, '\0', VALUE_SIZE);
        strcpy(z, y.c_str());

        #ifdef DEBUG
        assert(strlen(z) < VALUE_SIZE);
        #endif

        t->write(x, z);
    }
    #endif


    t->clearReads();
    t->clearWrites();

    auto start = std::chrono::high_resolution_clock::now();

    int x;
    char * z, * temp;
    for (int i = 0; i <= COUNT; i++) {
        switch (rand() % 3) {
            case 0:
                // writing a value.
                x = rand() % MAX;

                allKeys.insert(x);

                z = new char[VALUE_SIZE]();
                memset(z, '\0', VALUE_SIZE);
                strcpy(z, to_string(x).c_str());

                #ifdef DEBUG
                assert(strlen(z) < VALUE_SIZE);
                #endif

                t->write(x, z);

                #ifdef DEBUG
                assert(t->read(x) != NULL);
                #endif

                break;
            
            case 1: 
                // Reading a value.
                x = rand() % MAX;

                temp = t->read(x);
                z = new char[VALUE_SIZE]();
                strcpy(z, to_string(x).c_str());

                if (allKeys.find(x) == allKeys.end()) {
                    // temp should be null.
                    #ifdef DEBUG
                    assert(temp == NULL);
                    #endif
                    
                } else if (z != NULL && temp != NULL) {
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
                break;

            case 2:
                // Deleting a value.
                x = rand() % MAX;

                t->erase(x);

                if (allKeys.find(x) != allKeys.end()) {
                    allKeys.erase(allKeys.find(x));
                }

                break;

            default:
                break;
        }

    }

    auto stop = std::chrono::high_resolution_clock::now();

    #ifdef DEBUG
    // t->printTree();
    // cout << "\tExp. Card: " << allKeys.size() << endl;
    assert(allKeys.size() == t->getCardinality());
    #endif

    t->close();

    cout << "    Pages Wrote: " << t->getNumWrites();
    cout << "\n    Pages  Read: " << t->getNumReads() << endl;
    cout << "    Time to Perform Operations: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms\n";

    delete t;
    return 0;
}