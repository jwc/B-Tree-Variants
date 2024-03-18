#include "ubptree.h"

using namespace std;

bool debug = false;

void uBPlusTree::write(int key, char * value) {
    Node * node = getNode(root);

    insert(key, value, node);
    
    #ifdef DEBUG
    if (debug) printTree();
    #endif
}


// Algorithm 1. insert(k, v, R)
// input : key k, associated value v, and current root R
// output: record <k', p'> where k' is a discriminator value and p' a pointer to a tree node; ⊥ 
//         indicates no need for further processing
// 1 if R is a leaf then
// 2    if R.ovfl = ⊥ then
// 3        if |R| + |<k, v>| ≤ C then R.put(<k, v>); return ⊥;
// 4        else R.ovfl = allocateNode(); redistribute(R, R.ovfl, <k, v>); return ⊥;
// 5    else if k < R.max and |R| + |<k, v>| ≤ C then R.put(<k, v>); return ⊥; else
// 6        success = |R.ovfl| + |<k, v>| ≤ C;
// 7        if success then R.ovfl.put(<k, v>);
// 8        else success = redistribute_l(R, R.ovfl, <k, v>);
// 9        if success then
// 10           if gain has been expensed then R.ovfl = ⊥; return <R.ovfl.min, R.ovfl>;
// 11           else return ⊥;
// 12       else R.ovfl = ⊥; return split(R, R.ovfl, <k, v>);
// 13 else Q = R.find(k); <l, p> = insert(k, v, Q); return localInsert(<l, p>, R);

std::pair<int, char *> uBPlusTree::insert(int key, char * val, Node * R) {
    if (isLeaf(R)) {
        Node * R_ovfl = NULL;
        if (R->l.overflow > -1) R_ovfl = getNode(R->l.overflow);
        // Node * R_ovfl = (Node *) uBPlusTree::getPage(R->l.overflow);
        if (debug) std::cout << "isLeaf";
        // 2 if R.ovfl = ⊥
        if (R_ovfl == NULL) {
            // 3 if |R| + |<k, v>| ≤ C 
            if (R->l.card + 1 <= NUM_LEAF_VALS) {
                // R.put(<k,v>); 
                put(R, {key, val});
                // return ⊥;
                if (debug) std::cout << "\tput\n";
                return {-1, NULL};
            } else {
                // R.ovfl = allocateNode(); 
                R_ovfl = allocateNode();
                    R_ovfl->l.type = OVERFLOW;
                    R_ovfl->l.overflow = -1;
                R->l.overflow = R_ovfl->l.index;
                // redistribute(R, R.ovfl, <k, v>); 
                // printTree();
                redistribute_b(R, R_ovfl, {key, val});
                // return ⊥;
                if (debug) std::cout << "\talloc,redist\n";
                // printTree();
                return {-1, NULL};
            }

        } else if (getCard(R) + getCard(R_ovfl) + 1 == 2 * NUM_LEAF_VALS && R->l.index == root) {
            if (debug) cout << "\tAt Capacity, root\n";
            R->l.overflow = -1;
            return split(R, R_ovfl, {key, val});
        
        } else if (getCard(R) + getCard(R_ovfl) + 1 == 2 * NUM_LEAF_VALS) {
            // Must split R and 
            if (debug) cout << "\tAt Capacity\n";
            assert(R->b.type != BRANCH);
            R->l.overflow = -1;
            R_ovfl->l.type = LEAF;
            redistribute_b(R, R_ovfl, {key, val});
            return {getMin(R_ovfl), (char *) R_ovfl};

        } else if (key < getMin(R_ovfl) && getCard(R) < NUM_LEAF_VALS && getCard(R_ovfl) < NUM_LEAF_VALS) {
            // place into R
            if (debug) cout << "\tinto R\n";
            put(R, {key, val});
            return {-1, NULL};

        } else if (key > getMax(R) && getCard(R_ovfl) < NUM_LEAF_VALS && getCard(R) < NUM_LEAF_VALS) {
            // place into R_ovfl
            if (debug) cout << "\tinto R_ovfl\n";
            put(R_ovfl, {key, val});
            return {-1, NULL};

        } else {
            // must rebalance
            if (debug) cout << "\trebalance\n";
            redistribute_b(R, R_ovfl, {key, val});
            return {-1, NULL};
        }

        if (gainExpensed(R)) {
            cout << "GAIN EXPENSED (I)" << endl;
            R->l.overflow = -1;
            R_ovfl->l.type = LEAF;
            return {getMin(R_ovfl), (char *) R_ovfl};
        } else {
            return {-1, NULL};
        }

        assert(false);
        return {-1, NULL};

        // // 5 else if k < R.max and |R| + |<k, v>| ≤ C
        // } else if (key < getMax(R) && R->l.card + 1 <= NUM_LEAF_VALS) {
        //     // R.put(<k, v>); 
        //     put(R, {key, val});
        //     std::cout << "\thasOvfl put\n";
        //     // return ⊥;
        //     return {-1, NULL};
        // } else {
        //     std::cout << "\thasOvfl else";

        //     // 6 success = |R.ovfl| + |<k, v>| ≤ C;
        //     bool success = R_ovfl->l.card + 1 <= NUM_LEAF_VALS;

        //     // 7 if success then R.ovfl.put(<k, v>);
        //     if (success) {
        //         std::cout << "\tsuccess=T";
        //         put(R_ovfl, {key, val});    
        //     } else {
        //         // 8 else success = redistribute_l(R, R.ovfl, <k, v>);
        //         success = redistribute_l(R, R_ovfl, {key, val});
        //         std::cout << "\tsuccess=F(" << success << ")";
        //         if (success) redistribute_b(R, R_ovfl, {key, val});
        //     }
        //     // 9 if success
        //     if (success) {
        //         // 10 if gain has been expensed then R.ovfl = ⊥; return <R.ovfl.min, R.ovfl>;
        //         //  **** gain
        //         std::cout << "\tsuccess=T\n";
        //             // return {-1, NULL};

        //         if (true) {
        //         // if (getCard(R) + getCard(R_ovfl) + 1 == 2 * NUM_LEAF_VALS) {
        //             // R_ovfl = NULL;
        //             R->l.overflow = -1;
        //             // setNode(R);
        //             return {getMin(R_ovfl), (char *) R_ovfl};
        //         } else {
        //             return {-1, NULL};
        //         }
        //         // 11 else return ⊥;
        //     } else {
        //         std::cout << "\tsuccess=F\n";
        //         // 12 R.ovfl = ⊥; 
        //         R->l.overflow = -1;
        //         // return split(R, R.ovfl, <k, v>);
        //         return split(R, R_ovfl, {key, val});
        //     }
        // }
    // 13
    } else { 
        if (debug) std::cout << "Branch\n";

        // Q = R.find(k);
        Node * Q = (Node *) find(R, key);
        
        // <l,p> = insert(k, v, Q);
        std::pair<int, char *> pair = insert(key, val, Q);
        // return localInsert(<l, p>, R);
        return localInsert(pair, R);
    }
    assert(false);
}

// // Algorithm 2. localInsert(<l, p>, R)
// // input : record of key l, and pointer to node p, and node R in 
// //         which the record will be inserted
// // output: record <k', p'>, where k' is a discriminator value and p' a pointer to 
// //         a tree node; ⊥ indicates no need for further processing
// // 1 if <l, p> = ⊥ then return ⊥;
// // 2 else if |R| + |<l, p>| ≤ C then R.put(<l, p>); return ⊥;
// // 3 else
// // 4    S = allocateNode(); m = redistribute_b(R, S, <l, p>);
// // 5    if R is root then T = allocateNode(); connect T to R and S; return ⊥;
// // 6    else return <m, S>;

std::pair<int, char *> uBPlusTree::localInsert(std::pair<int, char *> pair, Node * R) {
    int l = pair.first;
    char * p = pair.second;
    Node * pp = (Node *) p;

    // cout << "pp type:" << pp->l.type << endl;
    // assert(pp->l.type == LEAF);
    assert(R->b.type == BRANCH);

    // cout << "l:" << l << " p:" << p << "\n";
    // if (debug) cout << "R:" << R->b.index << endl;
    if (l == -1 && p == NULL) {
        // if (debug) cout << "A\n";
        return {-1, NULL};
    } else if (R->b.card + 1 <= NUM_BRANCH_VALS) {
        if (debug) cout << "B\n";
        put(R, pair);
        return {-1, NULL};
    } else {
        // if (debug) cout << "C\n";

        Node * S = allocateNode();
        S->b.type = BRANCH;
        S->b.card = 0;
        int m = redistribute_b(R, S, pair);

        // setPage(S->b.index, S);
        setNode(S);
        // if (debug) cout << "S:" << endl;
        printPage(S);

        if (R->b.index == root) {
            Node * T = allocateNode();

            // cout << "T:" << endl;
            // setPage(T->b.index, T);
            // printPage(T);

            // if (debug) cout << "New Node\n";
            // *** connect T to R and S;
            T->b.type = BRANCH;
            T->b.card = 2;
            T->b.offsets[0] = R->b.index;
            T->b.offsets[1] = S->b.index;
            T->b.discrim[0] = getMin(S);
            root = T->b.index;

            // setPage(T->b.index, T);
            setNode(T);
            // printPage(T);

            return {-1, NULL};
        } else {
            return {m, (char *) S};
        }
    }

    return {-1, NULL};
}