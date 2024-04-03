#include "ubpTree.h"

using namespace std;

void uBPlusTree::erase(int key) {
    Node * r = getNode(metadata.root);

    if (isLeaf(r)) {
        if (!hasOvfl(r) ) {
            removeLeaf(r, key);
            return;
        }
        Node * rOvfl = getOvfl(r);
        if (getCard(r) + getCard(rOvfl) <= NUM_LEAF_VALS) {
            mergeLeaf(r, rOvfl, key);
        } else if (getMax(r) >= key) {
            removeLeaf(r, key);
        } else {
            removeLeaf(rOvfl, key);
        }
        return;
    }

    Node * undercap;
    Node * overcap;
    int discrim;
    tie(undercap, discrim, overcap) = remove(r, key);

    if (undercap) {
        // Need to handle underflow
        if (getCard(r) == 1) {
            metadata.root = r->b.offsets[0];
        }
    }

    if (overcap) {
        // Need new root
        Node * newRoot = allocateNode(BRANCH);
        metadata.root = getIndex(newRoot);

        newRoot->b.offsets[0] = getIndex(r);
        newRoot->b.offsets[1] = getIndex(overcap);
        newRoot->b.discrim[0] = discrim;
        newRoot->b.card = 2;

        setNode(newRoot);
    }
}

tuple<Node *, int, Node *> uBPlusTree::remove(Node * node, int key) {
    if (isLeaf(node)) {

        if (!hasOvfl(node) && getCard(node) > (NUM_LEAF_VALS + 1) / 2) {
            removeLeaf(node, key);
            return {NULL, 0, NULL};

        } else if (!hasOvfl(node)) {
            // Node under capacity
            removeLeaf(node, key);
            return {node, 0, NULL}; 
        }
        Node * nodeOvfl = getOvfl(node);
        if (getCard(node) + getCard(nodeOvfl) <= NUM_LEAF_VALS) {
            // Under capacity
            mergeLeaf(node, nodeOvfl, key);

        } else if (getMax(node) >= key && getCard(node) > NUM_LEAF_VALS / 2) {
            // capacity is fine, key in node
            removeLeaf(node, key);

        } else if (gainExpensed(nodeOvfl) && key < getMin(nodeOvfl)) {
            // split, key in node
            unsetOvfl(node, nodeOvfl);
            removeLeaf(node, key);
            int discrim = redistributeLeaf(node, nodeOvfl, 0, NULL);
            return {NULL, discrim, nodeOvfl};
        
        } else if (gainExpensed(nodeOvfl)) {
            // split, key in overflow
            unsetOvfl(node, nodeOvfl);
            removeLeaf(nodeOvfl, key);
            int discrim = redistributeLeaf(node, nodeOvfl, 0, NULL);
            return {NULL, discrim, nodeOvfl};
        
        } else if (getMax(node) < key && getCard(nodeOvfl) > NUM_LEAF_VALS / 2) {
            // capacity is fine, key in overflow
            removeLeaf(nodeOvfl, key);

        } else {
            // Must redistribute
            if (key <= getMax(node))
                removeLeaf(node, key);
            else 
                removeLeaf(nodeOvfl, key);

            redistributeLeaf(node, nodeOvfl, 0, NULL);
        }

        return {NULL, 0, NULL};
    } else {
        Node * chld = findChld(node, key);

        int discrim;
        Node * undercap;
        Node * overcap;
        tie(undercap, discrim, overcap) = remove(chld, key);

        if (undercap) {
            // Need to handle underflow
            Node * undercapNeighbor = NULL;
            int i;
            for (i = 0; i < getCard(node) - 1; i++)
                if (node->b.offsets[i] == getIndex(undercap)) {
                    // // Allowing last node to be under filled - not needed, but easier bcus of overflow nodes.
                    // return {NULL, 0, NULL}; 

                    undercapNeighbor = getNode(node->b.offsets[i + 1]);
                    break;
                }
            
            if (undercapNeighbor == NULL) {
                #ifdef DEBUG
                assert(i == getCard(node) - 1);
                #endif
                return {NULL, 0, NULL};
                // Lets us say the neighbor is always the node to the right of undercap
                undercapNeighbor = undercap;
                undercap = getNode(node->b.offsets[getCard(node) - 1]);
                if (hasOvfl(undercap)) {
                    // If the left node being merged/redistributed has an overflow node, must instead use the overflow node.
                    return {NULL, 0, NULL};
                    undercap = getOvfl(undercap);
                }
                i--;

            } 

            int capacity = isLeaf(undercap) ? NUM_LEAF_VALS : NUM_BRANCH_VALS;
            if (getCard(undercap) + getCard(undercapNeighbor) > capacity) {

                discrim = redistribute(undercap, undercapNeighbor);
                node->b.discrim[i] = discrim;
                setNode(node);

            } else {
                merge(undercap, undercapNeighbor, key);
                
                removeNode(node, getIndex(undercapNeighbor));

                if (getCard(node) < NUM_BRANCH_VALS / 2) return {node, 0, NULL};
            }
        }

        if (overcap) {
            if (getCard(node) < NUM_BRANCH_VALS) {
                putNode(node, discrim, overcap);
                return {NULL, 0, NULL};

            } else {
                Node * newSibling = allocateNode(BRANCH);
                discrim = redistributeNode(node, newSibling, discrim, overcap);
                return {NULL, discrim, newSibling};
            }
        }

        return {NULL, 0, NULL};
    }

    #ifdef DEBUG
    assert(false);
    #endif

    return {NULL, 0, NULL};
}
