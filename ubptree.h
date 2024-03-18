#include <utility>
#include <map>

#include "tree.h"

#define DEBUG

#define NUM_LEAF_VALS (PAGE_SIZE - (4 * sizeof(int))) / (VALUE_SIZE + sizeof(int))
#define NUM_BRANCH_VALS ((PAGE_SIZE - (3 * sizeof(int))) / (2 * sizeof(int)))
#define GAIN_THRESHOLD 4

enum NodeType {BRANCH, LEAF, OVERFLOW};

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

class uBPlusTree : public Tree {
    public:
        explicit uBPlusTree(std::string f) : Tree(f) {
            root = 0;
            Node * r = new Node;
            r->b.index = 0;
            r->b.type = LEAF;
            r->b.card = 0;
            r->l.overflow = -1;
            setPage(0, r);
            numPages = 1;

            assert(sizeof(Node) <= PAGE_SIZE);
            assert(NUM_LEAF_VALS >= 3);
        }

        // ubpwrite.cpp
        void write(int, char *);

        // ubperase.cpp
        // void erase(int);

        // ubpread.cpp
        // char * read(int);

        #ifdef DEBUG
        void printTree() {
            // std::cout << "root:" << root << "\n";
            Node * r = getNode(root);
            std::cout << "\tTREE:" << r << "\n";
            printPage(r);
            std::cout << "\twrites: " << numWrites << "\n";
            std::cout << "\treads: " << numReads << "\n";
        }
        #endif

    private:
        int root;
        int numPages;
        std::map<int, int> gain;
        
        // ubpwrite.cpp
        std::pair<int, char *> insert(int, char *, Node *);
        std::pair<int, char *> localInsert(std::pair<int, char *>, Node *);

        // ubpread.cpp
        // std::pair<char *, std::pair<int, char *>> search(int, Node *);

        // ubperase.cpp
        // std::pair<int, char *> delet(int, Node);

        // ubpmisc.cpp
        char * find(Node *, int);
        void put(Node *, std::pair<int, char*>);
        void remove(Node *, int);
        Node * allocateNode();
        bool redistribute_l(Node *, Node *, std::pair<int, char*>);
        int redistribute_b(Node *, Node *, std::pair<int, char*>);
        std::pair<int, char *> split(Node *, Node *, std::pair<int, char *>);

        int getMax(Node * node) {
            if (isLeaf(node)) return node->l.keys[node->l.card - 1];
            else return getMax(getNode(node->b.offsets[getCard(node) - 1]));
        }

        int getMin(Node * node) {
            if (isLeaf(node)) return node->l.keys[0];
            else return getMin(getNode(node->b.offsets[0]));
        }

        char * getPage(int pos) {
            fseek(database, (PAGE_SIZE * pos), 0);
            char * page = new char[PAGE_SIZE]();
            fread(page, PAGE_SIZE, 1, database);
            numReads++;

            int temp = gain[pos];
            if (temp < 0) temp = 0;
            gain[pos] = temp + 1;

            return page;
        }

        void setPage(int pos, void * page) {
            fseek(database, (PAGE_SIZE * pos), 0);
            fwrite(page, PAGE_SIZE, 1, database);
            numWrites++;

            int temp = gain[pos] - 1;
            if (temp < 0) temp = 0;
            gain[pos] = temp;
        }

        Node * getNode(int index) {
            return (Node *) getPage(index);
        }

        void setNode(Node * node) {
            setPage(node->l.index, node);
        }

        int getCard(Node * node) {
            return node->l.card;
        }

        void setGain(Node * node, int newGain) {
            gain[node->l.index] = newGain;
        }

        int getGain(Node * node) {
            return gain.find(node->b.index) == gain.end() ? -1 : gain.find(node->b.index)->second;
        }

        bool gainExpensed(Node * node) {
            return getGain(node) > GAIN_THRESHOLD;
        }

        bool isLeaf(Node * n) {
            return n->l.type != BRANCH;
        }

        #ifdef DEBUG
        void printPage(Node * node) {
            if (node->l.type != BRANCH) {
                std::cerr << "{id:" << node->l.index << " ty:" << node->l.type << " card:" <<
                            node->l.card << " ovfl:" << node->l.overflow << " min:" << getMin(node) << 
                            " max:" << getMax(node) << " keys:"; 
                
                for (int i = 0; i < node->l.card; i++) {
                    std::cerr << "(" << node->l.keys[i] << ")";
                }

                std::cerr << "}" << std::endl;

                if (node->l.type == LEAF && node->l.overflow >=0) 
                    printPage(getNode(node->l.overflow));
            } else {
                std::cerr << "{id:" << node->b.index << " ty:" << node->b.type << " card:" << 
                node->b.card << " ptrs:"; 
                for (int i = 0; i < node->b.card; i++) {
                    if (i == node->b.card - 1) 
                        std::cerr << "(" << node->b.offsets[i] << ")";
                    else
                        std::cerr << "(" << node->b.offsets[i] << ")" << node->b.discrim[i];
                }
                std::cerr << "}\n";
                for (int i = 0; i < node->b.card; i++)
                    printPage(getNode(node->b.offsets[i]));
            }
        }
        #endif
};