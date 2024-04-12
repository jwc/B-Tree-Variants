#include <bits/stdc++.h>

#include "asbtree.h"

using namespace std;

void ASBTree::erase(int key) {
    Node * root = getNode(this->root);
    Node * node = findNode(key, root);
    localEraseLeaf(node, key);
}

void ASBTree::localEraseLeaf(Node * node, int key) {
    remove(node, key);
    if (node->id == root) {
        setNode(node);
        return;
    }
    if (node->card < NUM_LEAF_VALS / 2) {
        int parent = findParent(node->id, key);
        Node * parentNode = getNode(parent);
        int rightId = 0;
        int leftId = 0;
        int i = 0;
        while (parentNode->b.nids[i] != node->id) i++;
        if (i == 0) {
            rightId = parentNode->b.nids[i + 1];
            leftId = -1;
        } else if (i == parentNode->card - 1) {
            rightId = -1;
            leftId = parentNode->b.nids[i - 1];
        } else {
            rightId = parentNode->b.nids[i + 1];
            leftId = parentNode->b.nids[i - 1];
        }

        if (rightId != -1) {
            Node * sibling = getNode(rightId);
            if (sibling->card > NUM_LEAF_VALS / 2) {
                int key = sibling->l.keys[0];
                char * value = sibling->l.values[0];
                put(node, key, value);
                remove(sibling, key);
                int newKey = sibling->l.keys[0];
                int j = 0;
                while (parentNode->b.nids[j] != sibling->id) j++;
                parentNode->b.discrim[j - 1] = newKey;
                setNode(parentNode);
                return;
            }
        }

        if (leftId != -1) {
            Node * sibling = getNode(leftId);
            if (sibling->card > NUM_LEAF_VALS / 2) {
                int key = sibling->l.keys[sibling->card - 1];
                char * value = sibling->l.values[sibling->card - 1];
                put(node, key, value);
                remove(sibling, key);
                int j = 0;
                while (parentNode->b.nids[j] != sibling->id) j++;
                parentNode->b.discrim[j] = key;
                setNode(parentNode);
                return;
            }
        }

        if (rightId != -1) {
            Node * sibling = getNode(rightId);
            mergeLeaf(node, sibling, parentNode, i);
        } else {
            Node * sibling = getNode(leftId);
            mergeLeaf(sibling, node, parentNode, i - 1);
        }
    }
}

void ASBTree::localEraseBranch(Node * node, int key) {
    removeDiscrim(node, key);
    if (node->id == root) {
        setNode(node);
        return;
    }
    if (node->card < NUM_BRANCH_VALS / 2) {
        int parent = findParent(node->id, key);
        Node * parentNode = getNode(parent);
        int rightId = 0;
        int leftId = 0;
        int i = 0;
        while (parentNode->b.nids[i] != node->id) i++;
        if (i == 0) {
            rightId = parentNode->b.nids[i + 1];
            leftId = -1;
        } else if (i == parentNode->card - 1) {
            rightId = -1;
            leftId = parentNode->b.nids[i - 1];
        } else {
            rightId = parentNode->b.nids[i + 1];
            leftId = parentNode->b.nids[i - 1];
        }

        if (rightId != -1) {
            Node * sibling = getNode(rightId);
            if (sibling->card > NUM_BRANCH_VALS / 2) {
                int key = sibling->b.discrim[0];
                int nid = sibling->b.nids[0];
                //removeDiscrim(sibling, key);
                for (int j = 0; j < sibling->card - 1; j++) {
                    sibling->b.discrim[j] = sibling->b.discrim[j + 1];
                    sibling->b.nids[j] = sibling->b.nids[j + 1];
                }
                sibling->card--;
                setNode(sibling);

                int j = 0;
                while (parentNode->b.nids[j] != sibling->id) j++;
                node->b.discrim[node->card - 1] = parentNode->b.discrim[j - 1];
                node->b.nids[node->card] = nid;
                node->card++;
                setNode(node);

                parentNode->b.discrim[j - 1] = key;
                setNode(parentNode);
                return;
            }
        }

        if (leftId != -1) {
            Node * sibling = getNode(leftId);
            if (sibling->card > NUM_LEAF_VALS / 2) {
                int key = sibling->b.discrim[sibling->card - 2];
                int nid = sibling->b.nids[sibling->card - 1];
                removeDiscrim(sibling, key);
                
                Node* child = getNode(node->b.nids[1]);
                node->b.discrim[0] = getMin(child);
                node->b.nids[0] = nid;
                setNode(node);

                int j = 0;
                while (parentNode->b.nids[j] != sibling->id) j++;
                for (int k = node->card - 2; k >= 0; k--) {
                    node->b.discrim[k] = node->b.discrim[k - 1];
                    node->b.nids[k + 1] = node->b.nids[k];
                }
                node->b.nids[1] = node->b.nids[0];
                node->b.discrim[0] = parentNode->b.discrim[j];
                node->b.nids[0] = nid;
                node->card++;
                setNode(node);

                parentNode->b.discrim[j] = key;
                setNode(parentNode);
                return;
            }
        }

        if (rightId != -1) {
            Node * sibling = getNode(rightId);
            mergeBranch(node, sibling, parentNode, i);
        } else {
            Node * sibling = getNode(leftId);
            mergeBranch(sibling, node, parentNode, i - 1);
        }
    }
}

void ASBTree::mergeLeaf(Node * node, Node * sibling, Node * parent, int i) {
    int key = parent->b.discrim[i];
    for (int j = 0; j < sibling->card; j++) {
        node->l.keys[node->card] = sibling->l.keys[j];
        memcpy(node->l.values[node->card], sibling->l.values[j], VALUE_SIZE);
        node->card++;
    }
    setNode(node);
    deleteNode(sibling->id);
    localEraseBranch(parent, key);
    parent = getNode(parent->id);
    if (parent->card == 1) {
        root = node->id;
        return;
    }
}

void ASBTree::mergeBranch(Node * node, Node * sibling, Node * parent, int i) {
    int key = parent->b.discrim[i];
    node->b.discrim[node->card - 1] = key;
    for (int j = 0; j < sibling->card; j++) {
        node->b.discrim[node->card] = sibling->b.discrim[j];
        node->b.nids[node->card] = sibling->b.nids[j];
        node->card++;
    }
    setNode(node);
    deleteNode(sibling->id);
    localEraseBranch(parent, key);
    parent = getNode(parent->id);
    if (parent->card == 1) {
        root = node->id;
        deleteNode(parent->id);
        return;
    }
}