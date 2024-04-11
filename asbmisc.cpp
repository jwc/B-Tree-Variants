#include <cstring>
#include <vector>

#include "asbtree.h"

using namespace std;

int ASBTree::getCardinality() { 
    return getCardinalityRecursive(getNode(root)); 
}

int ASBTree::getCardinalityRecursive(Node * node) {
    int temp = 0;

    if (node->type != BRANCH) {
        temp = node->card;
    } else {
        for (int i = 0; i < node->card; i++)
            temp += getCardinalityRecursive(getNode(node->b.nids[i]));
    }   
    
    return temp;
}

char * ASBTree::findInNode(Node * node, int key) {
    for (int i = 0; i < node->card; i++) {
        if (key == node->l.keys[i]) {
            char * val = new char[VALUE_SIZE]();
            memcpy(val, node->l.values[i], VALUE_SIZE);
            return val;
        }
    }
    return NULL;
}

Node* ASBTree::findNode(int key, Node * root) {
    Node * curr = root;

    while (curr->type != LEAF) {
        for (int i = 0; i < curr->card; i++) {
        }
        int i = 0;
        bool last = true;
        while (i < curr->card - 1) {
            if (key < curr->b.discrim[i]) {
                curr = getNode(curr->b.nids[i]);
                last = false;
                break;
            }
            i++;
        }
        if (last) {
            curr = getNode(curr->b.nids[curr->card - 1]);
        }
    }

    return curr;
}

Node * ASBTree::allocateNode(enum NodeType type) {
    Node * node = new Node;
    memset(node, -2, sizeof(Node));
    node->card = 0;
    node->type = type;
    node->id = nextId++;
    return node;
}

int ASBTree::findParent(int nid, int key) {
    Node * curr = getNode(root);
    Node * prev = NULL;

    while (curr->id != nid) {
        int i = 0;
        bool last = true;
        while (i < curr->card - 1) {
            if (key < curr->b.discrim[i]) {
                prev = curr;
                curr = getNode(curr->b.nids[i]);
                last = false;
                break;
            }
            i++;
        }
        if (last) {
            prev = curr;
            curr = getNode(curr->b.nids[curr->card - 1]);
        }
    }

    if (!prev) {
        return -1;
    }
    return prev->id;
}

void ASBTree::remove(Node * node, int key) {
    for (int i = 0; i < node->card; i++) {
        if (node->l.keys[i] == key) {
            for (int j = i + 1; j < node->card; j++) {
                node->l.keys[j - 1] = node->l.keys[j];
                memcpy(node->l.values[j - 1], node->l.values[j], VALUE_SIZE);
            }

            node->card--;
            break;
        }
    }

    setNode(node);
}

void ASBTree::removeDiscrim(Node * node, int key) {
    for (int i = 0; i < node->card - 1; i++) {
        if (node->b.discrim[i] == key) {
            for (int j = i + 1; j < node->card - 1; j++) {
                node->b.discrim[j - 1] = node->b.discrim[j];
                node->b.nids[j] = node->b.nids[j + 1];
            }
            node->card--;
            break;
        }
    }

    setNode(node);
}

void ASBTree::splitLeaf(Node *node, int insKey, char* insValue) {
    int pid = findParent(node->id, node->l.keys[0]);
    if (pid != -1 && getNode(pid)->card == NUM_BRANCH_VALS) {
        Node * parent = getNode(pid);
        splitBranch(parent);
        splitLeaf(node, insKey, insValue);
        return;
    }
    int mid = NUM_LEAF_VALS / 2;
    Node * sibling = allocateNode(LEAF);
    sibling->type = node->type;
    sibling->card = node->card - mid;

    for (int i = 0; i < sibling->card; i++) {
        sibling->l.keys[i] = node->l.keys[mid + i];
        memcpy(sibling->l.values[i], node->l.values[mid + i], VALUE_SIZE);
    }

    node->card = mid;

    if (insKey < sibling->l.keys[0]) {
        put(node, insKey, insValue);
        setNode(sibling);
    } else {
        put(sibling, insKey, insValue);
        setNode(node);
    }

    int key = sibling->l.keys[0];
    if (pid == -1) {
        Node * newRoot = allocateNode(BRANCH);
        newRoot->type = BRANCH;
        root = newRoot->id;
        newRoot->b.discrim[0] = key;
        newRoot->b.nids[0] = node->id;
        newRoot->b.nids[1] = sibling->id;
        newRoot->card = 2;
        setNode(newRoot);
        return;
    }

    Node * p = getNode(pid);

    int i = 0;
    while (i < p->card - 1) {
        if (key < p->b.discrim[i]) {
            break;
        }
        i++;
    }

    for (int j = p->card - 1; j > i; j--) {
        p->b.discrim[j] = p->b.discrim[j - 1];
        p->b.nids[j + 1] = p->b.nids[j];
    }
    if (i == 0) {
        p->b.nids[1] = p->b.nids[0];
    }

    p->b.discrim[i] = key;
    p->b.nids[i + 1] = sibling->id;
    p->card++;
    setNode(p);
}

void ASBTree::splitBranch(Node *node) {
    int pid = findParent(node->id, node->b.discrim[0]);
    if (pid != -1 && getNode(pid)->card == NUM_BRANCH_VALS) {
        Node * parent = getNode(pid);
        splitBranch(parent);
        splitBranch(node);
        return;
    }
    int mid = NUM_BRANCH_VALS / 2;
    Node * sibling = allocateNode(BRANCH);
    sibling->type = node->type;
    sibling->card = node->card - mid;

    sibling->b.nids[0] = node->b.nids[mid];
    for (int i = 0; i < sibling->card - 1; i++) {
        sibling->b.discrim[i] = node->b.discrim[mid + i];
        sibling->b.nids[i + 1] = node->b.nids[mid + i + 1];
    }

    int key = node->b.discrim[mid - 1];
    node->card = mid;

    setNode(node);
    setNode(sibling);

    if (pid == -1) {
        Node * newRoot = allocateNode(BRANCH);
        newRoot->type = BRANCH;
        root = newRoot->id;
        newRoot->b.discrim[0] = key;
        newRoot->b.nids[0] = node->id;
        newRoot->b.nids[1] = sibling->id;
        newRoot->card = 2;
        setNode(newRoot);
        return;
    }

    Node * p = getNode(pid);

    int i = 0;
    while (i < p->card - 1) {
        if (key < p->b.discrim[i]) {
            break;
        }
        i++;
    }

    for (int j = p->card - 1; j > i; j--) {
        p->b.discrim[j] = p->b.discrim[j - 1];
        p->b.nids[j + 1] = p->b.nids[j];
    }
    if (i == 0) {
        p->b.nids[1] = p->b.nids[0];
    }

    p->b.discrim[i] = key;
    p->b.nids[i] = node->id;
    p->b.nids[i + 1] = sibling->id;
    p->card++;
    setNode(p);
}

void ASBTree::put(Node * node, int key, char * val) {
    for (int i = 0; i < NUM_LEAF_VALS; i++) {
        if (i == node->card) {
            memcpy(node->l.values[i], val, VALUE_SIZE);
            node->l.keys[i] = key;
            node->card++;
            break;
        }

        if (key == node->l.keys[i]) {
            memcpy(node->l.values[i], val, VALUE_SIZE);
            node->l.keys[i] = key;
            break;
        } 

        if (key < node->l.keys[i]) {
            for (int j = node->card; j > i; j--) {
                // shift vals over
                node->l.keys[j] = node->l.keys[j - 1];
                memcpy(node->l.values[j], node->l.values[j - 1], VALUE_SIZE);
            }

            memcpy(node->l.values[i], val, VALUE_SIZE);
            node->l.keys[i] = key;
            node->card++;
            break;
        }
    }
    setNode(node);
}