#include <bits/stdc++.h> 

#include "ubpTree.h"

#define UBPTREE

using namespace std;

void uBPlusTree::putNode(Node * node, int newDiscrim, Node * newNode) {
    vector<int> discriminators;
    map<int, int> indices;

    #ifdef DEBUG
    assert(node->b.type == BRANCH);
    #endif
    
    if (node->b.card > 0) {
        discriminators.push_back(INT_MIN);
        indices[INT_MIN] = node->b.offsets[0];
    }

    for (int i = 0; i < node->b.card - 1; i++) {
        discriminators.push_back(node->b.discrim[i]);
        indices[node->b.discrim[i]] = node->b.offsets[i + 1];
    }

    if (find(discriminators.begin(), discriminators.end(), newDiscrim) == discriminators.end()) {
        discriminators.push_back(newDiscrim);
        indices[newDiscrim] = newNode->l.index;
    }

    sort(discriminators.begin(), discriminators.end());

    node->b.card = 0;
    for (auto &i : discriminators) {
        if (node->b.card == 0) {
            node->b.offsets[0] = indices[i];
        } else {
            node->b.offsets[node->b.card] = indices[i];
            node->b.discrim[node->b.card - 1] = i;
        }
        node->b.card++;
    }

    setNode(node);
}

void uBPlusTree::putLeaf(Node * node, int key, char * value) {
    #ifdef DEBUG
    assert(node->b.type != BRANCH);
    #endif

    for (int i = 0; i < NUM_LEAF_VALS; i++) {
        if (key == node->l.keys[i]) {
            memcpy(node->l.values[i], value, VALUE_SIZE);
            node->l.keys[i] = key;
            break;
        } 
        
        if (i == node->l.card) {
            memcpy(node->l.values[i], value, VALUE_SIZE);
            node->l.keys[i] = key;
            node->l.card++;
            break;
        } 

        if (key < node->l.keys[i]) {
            for (int j = node->l.card; j > i; j--) {
                node->l.keys[j] = node->l.keys[j - 1];
                memcpy(node->l.values[j], node->l.values[j - 1], VALUE_SIZE);
            }

            memcpy(node->l.values[i], value, VALUE_SIZE);
            node->l.keys[i] = key;
            node->l.card++;
            break;
        }
    }
    setNode(node);
}

Node * uBPlusTree::allocateNode(enum NodeType type) {
    Node * node = new Node;
    memset(node, 0, sizeof(Node));

    node->l.index = numPages++;
    node->l.card = 0;
    node->l.type = type;

    switch (type) {
    case BRANCH:
        break;

    case LEAF:
    case OVFL:
        node->l.overflow = -1;
        break;
    
    default:
        #ifdef DEBUG
        assert(false);
        #endif
        ;
    }

    return node;
}
