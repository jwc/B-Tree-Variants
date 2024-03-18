#include "ubptree.h"

using namespace std;

char * uBPlusTree::read(int key) {
    return search(key, getNode(root)).first;
}

// Algorithm 4. search(k, R)
// input : key k and current root R
// output: value v associated with k and record <k', p'> where k is 
//         a discriminator value and p' a pointer to a tree node; 
//         ⊥ indicates no further processing

// 1 if R is a leaf then
// 2 if R.ovfl = ⊥ then return R.find(k), ⊥;
// 3 else if k < R.max then return R.find(k), ⊥;
// 4 else
// 5    v = R.find(k);
// 6    if gain has been expensed then R.ovfl = ⊥; return v, <R.ovfl.min, R.ovfl>;
// 7    else return v, ⊥;
// 8 else Q = R.find(k); v, <l, p> = search(k, Q); return v, localInsert(<l, p>, R);

std::pair<char *, std::pair<int, char *>> uBPlusTree::search(int k, Node * R) {
    cout << "\tSearch:" << R->l.index;
    if (isLeaf(R)) {
        cout << "\tLeaf:";
        Node * R_ovfl = NULL;
        if (R->l.overflow > -1) R_ovfl = getNode(R->l.overflow);

        if (R_ovfl == NULL) {
            cout << "\tRetA\t";
            return {find(R, k), {-1, NULL}};
        } else if (k <= getMax(R)) {
            cout << "\tRetB\t";
            return {find(R, k), {-1, NULL}};
        } else {
            cout << "\tRetC\t";
            auto v = find(R_ovfl, k);
            
            if (gainExpensed(R_ovfl)) {
                cout << "GAIN EXPENSED (S)" << endl;
                R->l.overflow = -1;
                R_ovfl->l.type = LEAF;
                setNode(R);
                setNode(R_ovfl);
                return {v, {getMin(R_ovfl), (char *) R_ovfl}};
            } else {
                return {v, {-1, NULL}};
            }
        }
    } else {
        cout << "\tNode:";
        Node * Q = (Node *) find(R, k);
        cout << Q->l.index;
        auto x = search(k, Q);
        return {x.first, localInsert(x.second, R)};
    }
}


