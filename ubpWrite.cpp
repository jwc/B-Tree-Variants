#include "ubpTree.h"

using namespace std;

void uBPlusTree::write(int key, char * value) {
    #ifdef DEBUG
    assert(value);
    #endif

    int k;
    Node * n;
    Node * r = getNode(metadata.root);

    tie(k, n) = insert(r, key, value);

    if (n) {
        // Need new root
        Node * newRoot = allocateNode(BRANCH);
        metadata.root = getIndex(newRoot);

        newRoot->b.offsets[0] = getIndex(r);
        newRoot->b.offsets[1] = getIndex(n);
        newRoot->b.discrim[0] = k;
        newRoot->b.card = 2;

        setNode(newRoot);
    }
}

tuple<int, Node *> uBPlusTree::insert(Node * R, int key, char * value) {
    #ifdef DEBUG
    assert(R);
    assert(value);
    #endif
    
    if (isLeaf(R)) {
        if (!hasOvfl(R) && getCard(R) < NUM_LEAF_VALS) {
            // no ovfl, has room. just put val in
            putLeaf(R, key, value);
            return {0, NULL};

        } else if (!hasOvfl(R)) {
            // no ovfl, at capacity. create ovfl, redistribute vals
            Node * R_ovfl = allocateNode(OVFL);
            setOvfl(R, R_ovfl);

            #ifdef DEBUG
            assert(R);
            assert(R->l.index > 0);
            assert(R_ovfl);
            assert(R_ovfl->l.index > 0);
            #endif

            redistributeLeaf(R, R_ovfl, key, value);
            return {0, NULL};
        } 
        Node * R_ovfl = getOvfl(R);
        if (getCard(R) + getCard(R_ovfl) + 1 == 2 * NUM_LEAF_VALS) {
            // At capacity. Must split
            unsetOvfl(R, R_ovfl);
            int discrim = redistributeLeaf(R, R_ovfl, key, value);
            return {discrim, R_ovfl};

        } else if (key < getMin(R_ovfl) && getCard(R) < NUM_LEAF_VALS && getCard(R_ovfl) < NUM_LEAF_VALS) {
            // place into R
            putLeaf(R, key, value);
            return {0, NULL};

        } else if (gainExpensed(R_ovfl)) {
            // gain expensed. Must split
            unsetOvfl(R, R_ovfl);
            int discrim = redistributeLeaf(R, R_ovfl, key, value);
            return {discrim, R_ovfl};

        } else if (key > getMax(R) && getCard(R_ovfl) < NUM_LEAF_VALS && getCard(R) < NUM_LEAF_VALS) {
            // place into R_ovfl
            putLeaf(R_ovfl, key, value);
            return {0, NULL};

        } else {
            // must redistribute btwn R and R_ovfl
            redistributeLeaf(R, R_ovfl, key, value);
            return {0, NULL};
        }

    } else {
        int discrim;
        Node * node;
        tie(discrim, node) = insert(findChld(R, key), key, value);

        #ifdef DEBUG
        assert(getCard(R) > 0);
        #endif
        
        if (node) {
            // Must insert node into R
            if (getCard(R) < NUM_BRANCH_VALS) {
                // Has room
                putNode(R, discrim, node);
                return {0, NULL};
            } else {
                // no room. must split, redistribute, pass up new node. 
                Node * newSibling = allocateNode(BRANCH);
                discrim = redistributeNode(R, newSibling, discrim, node);
                return {discrim, newSibling};
            }
        } else {
            // No additional work needed
            return {0, NULL};
        }
    }

    #ifdef DEBUG
    assert(false);
    #endif

    return {0, NULL};
} 
