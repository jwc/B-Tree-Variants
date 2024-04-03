#include <cstring>

#include "ubpTree.h"

using namespace std;

char * uBPlusTree::read(int key) {
    Node * r = getNode(metadata.root);
    char * val;
    int discrim;
    Node * newChld;
    tie(val, discrim, newChld) = search(r, key);

    if (newChld) {
        // Need new root
        Node * newRoot = allocateNode(BRANCH);
        metadata.root = getIndex(newRoot);

        newRoot->b.offsets[0] = getIndex(r);
        newRoot->b.offsets[1] = getIndex(newChld);
        newRoot->b.discrim[0] = discrim;
        newRoot->b.card = 2;

        setNode(newRoot);
    }

    return val;
}

tuple<char *, int, Node *> uBPlusTree::search(Node * node, int key) {
    char * val = NULL;
    int discrim;
    
    if (isLeaf(node)) {
        if (!hasOvfl(node) || key <= getMax(node)) {
            // check node
            for (int i = 0; i < node->l.card; i++) {
                if (key == node->l.keys[i]) {
                    val = new char[VALUE_SIZE]();

                    memcpy(val, node->l.values[i], VALUE_SIZE);
                    return {val, 0, NULL};
                }
            }
            // key not in node
            return {NULL, 0, NULL};
        }

        Node * node_ovfl = getOvfl(node);
        // check node_ovfl
        for (int i = 0; i < node_ovfl->l.card; i++) {
            if (key == node_ovfl->l.keys[i]) {
                val = new char[VALUE_SIZE]();
                memcpy(val, node_ovfl->l.values[i], VALUE_SIZE);
                break;
            }
        }

        if (gainExpensed(node_ovfl)) {
            unsetOvfl(node, node_ovfl);
            discrim = redistributeLeaf(node, node_ovfl, 0, NULL);
            return {val, discrim, node_ovfl};

        } else {
            return {val, 0, NULL};
        }


    } else {
        Node * chld = findChld(node, key);
        Node * newChld;
        tie(val, discrim, newChld) = search(chld, key);

        if (newChld == NULL) {
            // no rebalancing.
            return {val, 0, NULL};

        } else if (getCard(node) < NUM_BRANCH_VALS) {
            // must rebalance
            putNode(node, discrim, newChld);
            return {val, 0, NULL};

        } else {
            Node * newSibling = allocateNode(BRANCH);
            int newDiscrim = redistributeNode(node, newSibling, discrim, newChld);
            return {val, newDiscrim, newSibling};
        }
    }

    #ifdef DEBUG
    assert(false);
    #endif
    
    return {NULL, 0, NULL};
}

Node * uBPlusTree::findChld(Node * node, int key) {
    #ifdef DEBUG
    assert(node);
    assert(node->b.type == BRANCH);
    assert(node->b.card > 0);
    #endif

    if (node->b.card == 1) return getNode(node->b.offsets[0]);

    for (int i = 0; i < node->b.card - 1; i++) 
        if (node->b.discrim[i] > key) 
            return getNode(node->b.offsets[i]);

    return getNode(node->b.offsets[getCard(node) - 1]);
}
