#include <bits/stdc++.h> 

#include "ubpTree.h"

using namespace std;

int uBPlusTree::redistributeNode(Node * N1, Node * N2, int discriminator, Node * child) {
    vector<int> discriminators;
    map<int, int> offsets;
    int runningMax = INT_MIN;

    if (N1->b.card > 0) {
        discriminators.push_back(INT_MIN);
        offsets[INT_MIN] = N1->b.offsets[0];
    }
    for (int i = 0; i < N1->b.card - 1; i++) {
        discriminators.push_back(N1->b.discrim[i]);
        offsets[N1->b.discrim[i]] = N1->b.offsets[i + 1];
        if (runningMax < N1->b.discrim[i]) runningMax = N1->b.discrim[i] + 1;
    }

    if (N2->b.card > 0) {
        discriminators.push_back(runningMax);
        offsets[runningMax] = N2->b.offsets[0];
    }
    for (int i = 0; i < N2->b.card - 1; i++) {
        discriminators.push_back(N2->b.discrim[i]);
        offsets[N2->b.discrim[i]] = N2->b.offsets[i + 1];
    }

    if (child && find(discriminators.begin(), discriminators.end(), discriminator) == discriminators.end()) {
        discriminators.push_back(discriminator);
        offsets[discriminator] = getIndex(child);
    }

    sort(discriminators.begin(), discriminators.end());

    N1->b.card = 0;
    N2->b.card = 0;

    int i = 0;
    int j = 0;
    int retVal = -1;
    for (auto &help : discriminators) {
        if (N1->b.card < discriminators.size() / 2) {
            if (i > 0) N1->b.discrim[i - 1] = help;
            N1->b.offsets[i] = offsets[help];
            N1->b.card++;
            i++;
            retVal = help + 1;

        } else {
            if (j > 0) N2->b.discrim[j - 1] = help;
            N2->b.offsets[j] = offsets[help];
            N2->b.card++;
            j++;
        }
    }

    setNode(N1);
    setNode(N2);
    return getMin(N2);
}

int uBPlusTree::redistributeLeaf(Node * N1, Node * N2, int key, char * value) {
    int num = (N1->l.card + N2->l.card + 1) / 2;
    vector<int> keys;
    map<int, char *> values;
    Node temp1, temp2;

    #ifdef DEBUG
    assert(isLeaf(N1));
    #endif

    memcpy(&temp1, N1, sizeof(Node));
    for (int i = 0; i < temp1.l.card; i++) {
        values[temp1.l.keys[i]] = temp1.l.values[i];
        if (find(keys.begin(), keys.end(), temp1.l.keys[i]) == keys.end()) {
            keys.push_back(temp1.l.keys[i]);
        } 
    }

    memcpy(&temp2, N2, sizeof(Node));
    for (int i = 0; i < temp2.l.card; i++) {
        values[temp2.l.keys[i]] = temp2.l.values[i];
        if (find(keys.begin(), keys.end(), temp2.l.keys[i]) == keys.end()) {
            keys.push_back(temp2.l.keys[i]);
        } 
    }

    if (value != NULL) {
        values[key] = value;
        if (find(keys.begin(), keys.end(), key) == keys.end()) {
            keys.push_back(key);
        }
    }

    sort(keys.begin(), keys.end());

    N1->l.card = 0;
    N2->l.card = 0;

    auto ik = keys.begin();
    int i = 0;
    int j = 0;
    int retVal = -1;
    while (ik != keys.end()) {
        if (N1->l.card < num) {
            N1->l.keys[i] = *ik;
            retVal = *ik;
            memcpy(N1->l.values[i], values[*ik], VALUE_SIZE);
            N1->l.card++;
            i++;
        } else {
            N2->l.keys[j] = (int) *ik;
            memcpy(N2->l.values[j], values[*ik], VALUE_SIZE);
            N2->l.card++;
            j++;
        }
        ik++;
    }

    #ifdef DEBUG
    assert(N1->l.index > 0);
    assert(N2->l.index > 0);
    assert(getMax(N1) < getMin(N2));
    #endif

    setNode(N1);
    setNode(N2);
    return getMin(N2);
}
