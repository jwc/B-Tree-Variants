#include "asbtree.h"

using namespace std;

char * ASBTree::search(int key) {
    Node * curr = getNode(root);

    while (curr->type != LEAF) {
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

    return findInNode(curr, key);
}


