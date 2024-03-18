#include <cstring>
#include <vector>

#include "ubptree.h"

using namespace std;

// (a) R.find(k): for leaves, the method returns either the value associated with k, 
//      or ⊥ if no such key exists; for branches, the method returns the outgoing pointer 
//      to the node containing values that are greater than or equal to k; 
char * uBPlusTree::find(Node * node, int key) {
    if (isLeaf(node)) {
        for (int i = 0; i < node->l.card; i++) {
            if (key == node->l.keys[i]) {
                char * val = new char[VALUE_SIZE]();
                memcpy(val, node->l.values[i], VALUE_SIZE);
                return val;
            }
            // if (node->b.discrim[i] >= key) {
            //     return getPage(node->b.offsets[i]);
            // }
        }
        return NULL;
        assert(false);
    } else {
        for (int i = 0; i < node->b.card; i++) {
            if (key < node->b.discrim[i]) {
                return getPage(node->b.offsets[i]);
            }
        }
        return getPage(node->b.offsets[getCard(node) - 1]);
    }
    assert(false);
    return NULL;
}

// (b) R.put(k, v): insert (or replace) key k and value v into the node; 
void uBPlusTree::put(Node * node, std::pair<int, char*>p) {
    int key = p.first; 
    char * val = p.second;

    if (node->b.type == BRANCH) {
        Node * addition = (Node *) val;
        cout << "put(BRANCH) k:" << key << "\ti:" << addition->l.index << endl;
        bool inserted = false;
        int tempKey = -3;
        int tempIndex = -3;
        for (int i = 1; i < node->b.card; i++) {
            if (key < node->b.discrim[i - 1]) {
                inserted = true;
                node->b.card++;

                tempKey = node->b.discrim[i - 1];
                tempIndex = node->b.offsets[i];

                node->b.discrim[i - 1] = key;
                node->b.offsets[i] = addition->b.index;
            } else if (inserted) {
                tempKey                = node->b.discrim[i - 1] ^ tempKey;
                node->b.discrim[i - 1] = node->b.discrim[i - 1] ^ tempKey;
                tempKey                = node->b.discrim[i - 1] ^ tempKey;

                tempIndex          = node->b.offsets[i] ^ tempIndex;
                node->b.offsets[i] = node->b.offsets[i] ^ tempIndex;
                tempIndex          = node->b.offsets[i] ^ tempIndex;

                // node->b.discrim[i - 1] = tempKey; // & node->b.discrim[i - 1];
                // node->b.offsets[i] = tempIndex;
                // tempKey = node->b.discrim[i];
                // tempIndex = node->b.offsets[i + 1];
            }
        }

        if (!inserted) {
            // cout << "STRAGGLER" << endl;
            node->b.discrim[node->b.card - 1] = key;
            node->b.offsets[node->b.card] = addition->b.index;
            node->b.card++;
        }

        setNode(node);
        return;
    }
    // if (key > getMax(node)) node->l.max = key;
    // if (key < getMin(node)) node->l.min = key;

    // if (node->l.card == 0) {
    //     node->l.keys[0] = key;
    //     memcpy(node->l.values[0], val, VALUE_SIZE);
    //     node->l.card++;        
    // } else

    for (int i = 0; i < NUM_LEAF_VALS; i++) {
        if (key == node->l.keys[i]) {
            memcpy(node->l.values[i], val, VALUE_SIZE);
            node->l.keys[i] = key;
            break;
        } 
        
        if (i == node->l.card) {
            memcpy(node->l.values[i], val, VALUE_SIZE);
            node->l.keys[i] = key;
            node->l.card++;
            break;
        } 

        if (key < node->l.keys[i]) {
            for (int j = node->l.card; j > i; j--) {
                // shift vals over
                node->l.keys[j] = node->l.keys[j - 1];
                memcpy(node->l.values[j], node->l.values[j - 1], VALUE_SIZE);
            }

            memcpy(node->l.values[i], val, VALUE_SIZE);
            node->l.keys[i] = key;
            node->l.card++;
            break;
        }
    }

    // setPage(node->l.index, node);
    setNode(node);
}

// (c) R.remove(k): remove the value associated with key k, if k exists.
void uBPlusTree::remove(Node * node, int key) {
    assert(node->l.type == LEAF);
    for (int i = 0; i < node->l.card; i++) {
        if (node->l.keys[i] == key) {
            for (int j = i + 1; j < node->l.card; j++) {
                node->l.keys[j - 1] = node->l.keys[j];
                memcpy(node->l.values[j - 1], node->l.values[j], VALUE_SIZE);
            }

            node->l.card--;
            break;
        }
    }

    // setPage(node->l.index, (void *) &node);
    setNode(node);
}

Node * uBPlusTree::allocateNode() {
    Node * node = new Node;
    memset(node, -2, sizeof(Node));
    node->l.card = 0;
    node->l.type = BRANCH;
    node->l.index = numPages++;
    // setNode(node);
    return node;
}

// This is achieved by function redistribute(), which returns
// a Boolean indicating whether redistribution is possible.
bool uBPlusTree::redistribute_l(Node *N1, Node *N2, std::pair<int, char *>p) {
    return N1->l.card + N2->l.card + 1 < 2 * NUM_LEAF_VALS;
}

int uBPlusTree::redistribute_b(Node *N1, Node *N2, std::pair<int, char *>p) {
    int num = (N1->l.card + N2->l.card + 1) / 2;
    bool pairInserted = false;
    int retVal = -1;
    
    if (isLeaf(N1)) {
        vector<int> keys;
        vector<char *> vals;
        Node temp1, temp2;
    
        memcpy(&temp1, N1, sizeof(Node));
        for (int i = 0; i < temp1.l.card; i++) {
            if (temp1.l.keys[i] >= p.first && !pairInserted) {
                keys.push_back(p.first);
                vals.push_back(p.second);
                pairInserted = true;
                i--;
            } else {
                keys.push_back(temp1.l.keys[i]);
                vals.push_back(temp1.l.values[i]);
            }
        }

        memcpy(&temp2, N2, sizeof(Node));
        for (int i = 0; i < temp2.l.card; i++) {
            if (temp2.l.keys[i] >= p.first && !pairInserted) {
                keys.push_back(p.first);
                vals.push_back(p.second);
                pairInserted = true;
                i--;
            } else {
                keys.push_back(temp2.l.keys[i]);
                vals.push_back(temp2.l.values[i]);
            }
        }

        if (!pairInserted) {
            keys.push_back(p.first);
            vals.push_back(p.second);
        }

        N1->l.card = 0;
        N2->l.card = 0;

        auto ik = keys.begin();
        auto iv = vals.begin();
        int i = 0;
        int j = 0;
        int retVal = -1;
        while (ik != keys.end()) {
            if (N1->l.card < num) {
                N1->l.keys[i] = *ik;
                retVal = *ik;
                memcpy(N1->l.values[i], *iv, VALUE_SIZE);
                N1->l.card++;
                i++;
            } else {
                N2->l.keys[j] = (int) *ik;
                memcpy(N2->l.values[j], *iv, VALUE_SIZE);
                N2->l.card++;
                j++;
            }

            ik++;
            iv++;
        }
    } else {
        vector<int> discriminators;
        vector<int> offsets;
        vector<pair<int, int>> children;

        children.push_back({-1, N1->b.offsets[0]});
        for (int i = 1; i < N1->b.card; i++) 
            children.push_back({N1->b.discrim[i - 1], N1->b.offsets[i]});

        if (N2->b.card > 0) children.push_back({getMin(getNode(N2->b.index)), N2->b.offsets[0]});
        for (int i = 1; i < N2->b.card; i++) 
            children.push_back({N2->b.discrim[i - 1], N2->b.offsets[i]});
        
        children.push_back({p.first, ((Node *) p.second)->b.index});

        for (int i = 0; i < children.size(); i++) {
            for (int j = i; j < children.size(); j++) {
                if (children.at(i).first > children.at(j).first) {
                    pair<int, int> temp = children.at(i);
                    children.emplace(children.begin() + i, children.at(j));
                    children.erase(children.begin() + i + 1);
                    children.emplace(children.begin() + j, temp);
                    children.erase(children.begin() + j + 1);
                }
            }
        }

        // cout << "chldn size:" << children.size() << endl;
        // for (auto &x : children) cout << "(" << x.first << ", " << x.second << ")";
        // cout << endl;

        N1->b.card = 0;
        N2->b.card = 0;

        int i = 0;
        int j = 0;
        int retVal = -1;
        for (auto &help : children) {
            if (N1->b.card < num) {
                if (i > 0) N1->b.discrim[i - 1] = help.first;
                if (i > 0) retVal = help.first;
                N1->b.offsets[i] = help.second;
                N1->b.card++;
                i++;
            } else {
                if (j > 0) N2->b.discrim[j - 1] = help.first;
                N2->b.offsets[j] = help.second;
                N2->b.card++;
                j++;
            }
        }
    }

    setNode(N1);
    setNode(N2);
    return retVal;
}

std::pair<int, char *> uBPlusTree::split(Node * R, Node * R_ovfl, std::pair<int, char *>pair) {
    assert(R->l.type == LEAF);
    assert(R_ovfl->l.type == OVERFLOW);
    assert(R_ovfl->l.overflow == -1);
    assert(R->l.card + R_ovfl->l.card == (2 * NUM_LEAF_VALS) - 1);
    // cout << "SPLIT\n";

    R->l.overflow = -1;
    R_ovfl->l.type = LEAF;

    redistribute_b(R, R_ovfl, pair);


    Node * N = allocateNode();
    N->l.type = BRANCH;
    N->b.discrim[0] = getMin(R_ovfl);
    // N->b.discrim[1] = getMax(R);
    // N->b.discrim[2] = getMax(R_ovfl);

    N->b.offsets[0] = R->l.index;
    N->b.offsets[1] = R_ovfl->l.index;

    N->b.card = 2;

    // N->b.min = getMin(R);
    // N->b.max = getMax(R_ovfl);

    // setPage(N->b.index, N);
    setNode(N);

    if (R->l.index == root) root = N->l.index;

    return {-1, (char *) N};
}

// merge()