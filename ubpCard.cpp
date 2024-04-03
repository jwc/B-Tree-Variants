#include "ubpTree.h"

#ifdef DEBUG
int uBPlusTree::getCardinality() { 
    return getCardinalityRecursive(getNode(metadata.root)); 
}

int uBPlusTree::getCardinalityRecursive(Node * node) {
    int temp = 0;

    #ifdef DEBUG
    assert(node->l.index > 0);
    #endif

    if (node->l.type != BRANCH) {
        temp = getCard(node);

        if (node->l.type == LEAF && hasOvfl(node)) {
            temp += getCard(getOvfl(node));
        }
    } else {
        for (int i = 0; i < node->b.card; i++)
            temp += getCardinalityRecursive(getNode(node->b.offsets[i]));
    }   
    
    return temp;
}

void uBPlusTree::printTree() {
    treeCard = 0;
    Node * r = getNode(metadata.root);
    std::cout << "\tTREE:" << "\n";
    printPage(r);
    std::cout << "\twrites: " << numWrites << "\n";
    std::cout << "\treads: " << numReads << "\n";
    std::cout << "\tReal Card: " << treeCard << "\n";
}

void uBPlusTree::printPage(Node * node) {
    assert(node->l.index > 0);
    if (node->l.type != BRANCH) {
        treeCard += getCard(node);
        std::cout << "{id:" << node->l.index << " ty:";
        if (node->l.type == LEAF) std::cout << "LEAF";
        else std::cout << "OVFL";
        std::cout << " card:" <<
                    node->l.card << " ovfl:" << node->l.overflow << " min:" << getMin(node) << 
                    " max:" << getMax(node) << " keys:"; 
        
        for (int i = 0; i < node->l.card; i++) {
            std::cout << "(" << node->l.keys[i] << ")";
        }

        std::cout << "}" << std::endl;

        if (node->l.type == LEAF && node->l.overflow >=0) {
            assert(node->l.overflow > 0);
            printPage(getNode(node->l.overflow));
        }
    } else {
        std::cout << "{id:" << node->b.index << " ty:" << "BRCH" << " card:" << 
        node->b.card << " ptrs:"; 
        for (int i = 0; i < node->b.card; i++) {
            if (i == node->b.card - 1) 
                std::cout << "(" << node->b.offsets[i] << ")";
            else
                std::cout << "(" << node->b.offsets[i] << ")" << node->b.discrim[i];
        }
        std::cout << "}\n";
        for (int i = 0; i < node->b.card; i++)
            printPage(getNode(node->b.offsets[i]));
    }
}
#endif