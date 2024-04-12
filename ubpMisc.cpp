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
        if (i == node->l.card) {
            memcpy(node->l.values[i], value, VALUE_SIZE);
            node->l.keys[i] = key;
            node->l.card++;
            break;
        } 

        if (key == node->l.keys[i]) {
            memcpy(node->l.values[i], value, VALUE_SIZE);
            node->l.keys[i] = key;
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

void uBPlusTree::removeNode(Node * node, int index) {
    vector<int> discriminators;
    map<int, int> indices;

    #ifdef DEBUG
    assert(node->b.type == BRANCH);
    #endif
    
    if (node->b.card > 0 && node->b.offsets[0] != index) {
        discriminators.push_back(INT_MIN);
        indices[INT_MIN] = node->b.offsets[0];
    }

    for (int i = 0; i < node->b.card - 1; i++) {
        if (node->b.offsets[i + 1] != index) {
            discriminators.push_back(node->b.discrim[i]);
            indices[node->b.discrim[i]] = node->b.offsets[i + 1];
        }
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

Node * uBPlusTree::removeLeaf(Node * node, int key) {
    #ifdef DEBUG
    assert(node->b.type != BRANCH);
    #endif

    int i;
    for (i = 0; i < getCard(node); i++) {
        if (node->l.keys[i] == key) {
            break;
        }
    }

    if (i == getCard(node)) {
        // key not found.
        return NULL;
    }

    for (int j = i; j < getCard(node); j++) {
        node->l.keys[j] = node->l.keys[j + 1];
        memcpy(node->l.values[j], node->l.values[j + 1], VALUE_SIZE);
    }

    node->l.card--;

    setNode(node);

    return NULL;
}

Node * uBPlusTree::merge(Node * n1, Node * n2, int key) {
    if (isLeaf(n1)) 
        return mergeLeaf(n1, n2, key);
    else {
        for (int i = 0; i < getCard(n2); i++) {
            if (i != 0) 
                n1->b.discrim[getCard(n1) - 1] = n2->b.discrim[i - 1];
            else 
                n1->b.discrim[getCard(n1) - 1] = getMin(n2);

            n1->b.offsets[getCard(n1)] = n2->b.offsets[i];
            n1->b.card++;
        }

        setNode(n1);
        deallocateNode(n2);
        return NULL;

        #ifdef DEBUG 
        assert(false);
        #endif
    }
}

Node * uBPlusTree::mergeLeaf(Node * n1, Node * n2, int key) {
    #ifdef DEBUG
    assert(getCard(n1) + getCard(n2) <= NUM_LEAF_VALS);
    #endif

    vector<int> keys;
    map<int, char *> values;
    Node temp1, temp2;

    memcpy(&temp1, n1, sizeof(Node));
    for (int i = 0; i < temp1.l.card; i++) {
        if (temp1.l.keys[i] != key) {
            values[temp1.l.keys[i]] = temp1.l.values[i];
            if (find(keys.begin(), keys.end(), temp1.l.keys[i]) == keys.end()) {
                keys.push_back(temp1.l.keys[i]);
            } 
        }
    }

    memcpy(&temp2, n2, sizeof(Node));
    for (int i = 0; i < temp2.l.card; i++) {
        if (temp2.l.keys[i] != key) {
            values[temp2.l.keys[i]] = temp2.l.values[i];
            if (find(keys.begin(), keys.end(), temp2.l.keys[i]) == keys.end()) {
                keys.push_back(temp2.l.keys[i]);
            }
        }
    }
    
    sort(keys.begin(), keys.end());

    n1->l.card = 0;
    for (auto i : keys) {
        n1->l.keys[n1->l.card] = i;
        memcpy(n1->l.values[n1->l.card], values[i], VALUE_SIZE);
        n1->l.card++;
    }

    n1->l.overflow = n2->l.overflow;

    setNode(n1);
    deallocateNode(n2);

    return NULL;
}

Node * uBPlusTree::allocateNode(enum NodeType type) {
    Node * node = new Node;
    memset(node, 0, sizeof(Node));

    if (metadata.numFreePages > 0) {
        node->l.index = metadata.freelist[metadata.numFreePages - 1];
        metadata.numFreePages--;
    } else {
        node->l.index = metadata.numPages++;
    }

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

void uBPlusTree::deallocateNode(Node * node) {
    if (metadata.numFreePages < FREELIST_SIZE) {
        metadata.freelist[metadata.numFreePages] = getIndex(node);
        metadata.numFreePages++;
    } 
}
