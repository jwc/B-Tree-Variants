#include <map>
#include <tuple>
#include <sys/stat.h>

#include "tree.h"

#define NUM_LEAF_VALS (PAGE_SIZE - (4 * sizeof(int))) / (VALUE_SIZE + sizeof(int))
#define NUM_BRANCH_VALS ((PAGE_SIZE - (3 * sizeof(int))) / (2 * sizeof(int)))
#define FREELIST_SIZE PAGE_SIZE - (3 * sizeof(int))
#define GAIN_THRESHOLD 4

enum NodeType {BRANCH, LEAF, OVFL};

struct Leaf {
    enum NodeType type;
    int index;
    int card;
    int overflow;
    int keys[NUM_LEAF_VALS];
    char values[NUM_LEAF_VALS][VALUE_SIZE];
};

struct Branch {
    enum NodeType type;
    int index;
    int card;
    int filler; 
    int discrim[NUM_BRANCH_VALS - 1];
    int offsets[NUM_BRANCH_VALS];
};

typedef union {
    struct Branch b;
    struct Leaf l;
} Node;

typedef struct {
    int root;
    int numPages;
    int numFreePages;
    int freelist[FREELIST_SIZE];
} TreeData;

class uBPlusTree : public Tree {
    public:
        explicit uBPlusTree(std::string filename) : Tree(filename) {
            open(filename);

            #ifdef DEBUG
            assert(sizeof(Node) <= PAGE_SIZE);
            assert(NUM_LEAF_VALS >= 3);
            #endif
        }

        ~uBPlusTree() {
            if (database) close();    
        }

        // ubpOpen.cpp
        void open(std::string filename);

        void close();

        // ubpWrite.cpp
        void write(int, char *);

        // ubpErase.cpp
        void erase(int);

        // ubpRead.cpp
        char * read(int);

        #ifdef DEBUG
        // ubpCard.cpp
        int getCardinality();

        void printTree();
        #endif

    private:
        TreeData metadata;
        std::map<int, int> gain;
    
        char * getPage(int pos) {
            #ifdef DEBUG
            assert(database);
            #endif

            char * page = new char[PAGE_SIZE]();
            lseek(database, (PAGE_SIZE * pos), 0);
            ::read(database, page, PAGE_SIZE);
            numReads++;

            int temp = gain[pos];
            if (temp < 0) temp = 0;
            gain[pos] = temp + 1;

            return page;
        }

        void setPage(int pos, void * page) {
            #ifdef DEBUG
            assert(database);
            #endif

            lseek(database, (PAGE_SIZE * pos), 0);
            ::write(database, page, PAGE_SIZE);
            numWrites++;

            int temp = gain[pos] - 1;
            if (temp < 0) temp = 0;
            gain[pos] = temp;
        }

        // ubpwrite.cpp
        std::tuple<int, Node *> insert(Node *, int, char *);

        // ubpread.cpp
        std::tuple<char *, int, Node *> search(Node *, int);

        Node * findChld(Node *, int);

        // ubpErase.cpp
        std::tuple<Node *, int, Node *> remove(Node *, int);

        // ubpmisc.cpp
        Node * allocateNode(enum NodeType type);

        void deallocateNode(Node *);

        void putNode(Node *, int, Node *);

        void putLeaf(Node *, int, char *);

        void removeNode(Node *, int);

        Node * removeLeaf(Node *, int);
        
        Node * merge(Node *, Node *, int);

        Node * mergeLeaf(Node *, Node *, int);

        // ubp redistribute.cpp
        int redistribute(Node *, Node *) ;

        int redistributeNode(Node *, Node *, int, Node *);

        int redistributeLeaf(Node *, Node *, int, char *);

        // Other
        int getMax(Node * node) { return isLeaf(node) ? node->l.keys[node->l.card - 1] : getMax(getNode(node->b.offsets[getCard(node) - 1])); }

        int getMin(Node * node) { return isLeaf(node) ? node->l.keys[0] : getMin(getNode(node->b.offsets[0])); }

        Node * getNode(int index) { return (Node *) getPage(index); }

        void setNode(Node * node) { setPage(node->l.index, node); }

        int getCard(Node * node) { return node->l.card; }

        int getGain(Node * node) { return gain.find(node->b.index) == gain.end() ? -1 : gain.find(node->b.index)->second; }

        void setGain(Node * node, int newGain) { gain[node->l.index] = newGain; }

        bool gainExpensed(Node * node) { return getGain(node) > GAIN_THRESHOLD; }

        int getIndex(Node * node) { return node->l.index; }

        Node * getOvfl(Node * node) { return getNode(node->l.overflow); }

        void setOvfl(Node * R, Node * R_ovfl) { R->l.overflow = R_ovfl->l.index; }

        void unsetOvfl(Node * R, Node * R_ovfl) {
            R->l.overflow = -1;
            R_ovfl->l.type = LEAF;
        }

        bool hasOvfl(Node * node) { return node->l.overflow > 0; }

        bool isLeaf(Node * n) { return n->l.type != BRANCH; }

        #ifdef DEBUG
        // ubpCard.cpp
        int getCardinalityRecursive(Node * node);

        int treeCard;
        void printPage(Node * node);
        #endif
};