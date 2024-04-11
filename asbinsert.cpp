#include "asbtree.h"

using namespace std;

void ASBTree::insert(int key, char *value) {
    Node *root = getNode(this->root);

    Node* node = findNode(key, root);
    localInsert(key, value, node);
}

void ASBTree::localInsert(int key, char *value, Node *node) {
    if (node->card == NUM_LEAF_VALS) {
        splitLeaf(node, key, value);
    } else {
        put(node, key, value);
    }
}