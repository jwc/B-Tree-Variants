#include "ubptree.h"

using namespace std;

int main() {
    // cout << "INT_SIZE: " << sizeof(int) << "\n";
    // cout << "PTR_SIZE: " << sizeof(char *) << "\n";
    cout << "PAGE_SIZE: " << PAGE_SIZE << "\n";
    // cout << "VALUE_SIZE: " << VALUE_SIZE << "\n";
    cout << "NUM_LEAF_VALS: " << NUM_LEAF_VALS << "\n";
    cout << "NUM_BRANCH_VALS: " << NUM_BRANCH_VALS << "\n";
    cout << "ENUM: " << sizeof(enum NodeType) << "\n";
    cout << "struct Leaf: " << sizeof(struct Leaf) << "\n";
    cout << "struct Branch: " << sizeof(struct Branch) << "\n";
    // cout << "struct Node: " << sizeof(Node) << "\n";
    // cout << "struct Tree: " << sizeof(uBPlusTree) << "\n";

    uBPlusTree * t = new uBPlusTree("X:\\ubptree.db");
    bool printIns = false;
    char testValue[VALUE_SIZE] = "x";

    if (printIns) cout << "\nCP 1\tins(20)\n";
    t->write(20, testValue);
    if (printIns) cout << "\nCP 2\tins(30)\n";
    t->write(30, testValue);
    if (printIns) cout << "\nCP 3\tins(40)\n";
    t->write(40, testValue);
    if (printIns) cout << "\nCP 4\tins(50)\n";
    t->write(50, testValue);
    if (printIns) cout << "\nCP 5\tins(60)\n";
    t->write(60, testValue);
    if (printIns) cout << "\nCP 6\tins(70)\n";
    t->write(70, testValue);
    if (printIns) cout << "\nCP 7\tins(10)\n";
    t->write(10, testValue);
    if (printIns) cout << "\nCP 8\tins(15)" << endl;
    t->write(15, testValue);
    if (printIns) cout << "\nCP 9\tins(25)" << endl;
    t->write(25, testValue);
    if (printIns) cout << "\nCP10\tins(80)" << endl;
    t->write(80, testValue);
    if (printIns) cout << "\nCP11\tins(90)" << endl;
    t->write(90, testValue);
    if (printIns) cout << "\nCP12\tins(100)" << endl;
    t->write(100, testValue);
    if (printIns) cout << "\nCP13\tins(55)" << endl;
    t->write(55, testValue);
    if (printIns) cout << "\nCP14\tins(65)" << endl;
    t->write(65, testValue);
    if (printIns) cout << "\nCP15\tins(75)" << endl;
    t->write(75, testValue);
    if (printIns) cout << "\nCP16\tins(-75)" << endl;
    t->write(-75, testValue);

    t->printTree();

    char null[5] = "NULL";
    char * temp;

    temp = t->read(55);
    if (temp == NULL) temp = null;
    cerr << "read(55):" << temp << endl;

    temp = t->read(10);
    if (temp == NULL) temp = null;
    cerr << "read(10):" << temp << endl;

    temp = t->read(50);
    if (temp == NULL) temp = null;
    cerr << "read(50):" << temp << endl;

    temp = t->read(100);
    if (temp == NULL) temp = null;
    cerr << "read(100):" << temp << endl;

    for(int i = 0; i < GAIN_THRESHOLD + 2; i++) {
        t->read(30);
        cout << endl;
    }

    t->printTree();
    cout << endl;
    delete t;
    return 0;
}