#include <utility>
#include <map>
#include <unordered_set>
#include <set>
#include <unistd.h>

#include "tree.h"

#define NUM_LEAF_VALS (PAGE_SIZE - (4 * sizeof(int))) / (VALUE_SIZE + sizeof(int))
#define NUM_BRANCH_VALS ((PAGE_SIZE - (3 * sizeof(int))) / (2 * sizeof(int)))
#define WRITE_BUFFER_SIZE 10
#define NODES_PER_FILE 16384
#define FILE_MIN_ACTIVE_NODES NODES_PER_FILE / 10

enum NodeType {BRANCH, LEAF};

struct Leaf {
    int keys[NUM_LEAF_VALS];
    char values[NUM_LEAF_VALS][VALUE_SIZE];
};

struct Branch {
    int discrim[NUM_BRANCH_VALS - 1];
    int nids[NUM_BRANCH_VALS];
};

struct Node {
    enum NodeType type;
    int id;
    int card;
    union {
        struct Branch b;
        struct Leaf l;
    };
};

struct NodeLocation {
    int file;
    int page;

    bool operator==(const NodeLocation& other) const {
        return file == other.file && page == other.page;
    }
};

struct NodeLocationHash {
    size_t operator()(const NodeLocation& loc) const {
        return std::hash<int>()(loc.file) ^ std::hash<int>()(loc.page);
    }
};

template<>
struct std::hash<NodeLocation> {
    size_t operator()(const NodeLocation &location) const {
        return std::hash<int>()(location.file) ^ (std::hash<int>()(location.page) << 1);
    }
};

class ASBTree : public Tree {
    public:
        explicit ASBTree(std::string f) : Tree(f) {
            baseFileName = f;
            currentFile = openFile(baseFileName + ".db");
            numValidNodesInFile[currentFile] = 0;
            files.insert(currentFile);
            fileNames[currentFile] = baseFileName + ".db";
            currentFileWriteCount = 0;

            root = 0;
            Node * r = new Node;
            r->id = 0;
            r->type = LEAF;
            r->card = 0;
            setNode(r);
            nextId = 1;
            nextFileNum = 1;
            writeBufferIndex = 0;

            #ifdef DEBUG
            assert(sizeof(Node) <= PAGE_SIZE);
            assert(NUM_LEAF_VALS >= 3);
            #endif
        }

        ~ASBTree() {
            auto it = files.begin();
            while (it != files.end()) {
                int file = *it;
                closeFile(file);
                it = files.begin();
            }
        }

        void open(std::string filename) {
            return;
        }
        
        void close() {
            flushWriteBuffer();
        }

        void write(int key, char * value) {
            insert(key, value);
        }

        char* read(int key) {
            return search(key);
        }

        // asbinsert.cpp
        void insert(int, char *);

        // asberase.cpp
        void erase(int);

        // asbsearch.cpp
        char * search(int);

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
        int nextId;
        std::string baseFileName;
        int nextFileNum;
        int currentFile;
        int currentFileWriteCount;
        std::map<int, int> numValidNodesInFile;
        std::map<int, NodeLocation> addressMap;
        std::unordered_set<NodeLocation> activeLocations;
        std::map<int, std::string> fileNames;
        std::set<int> files;
        Node* writeBuffer[WRITE_BUFFER_SIZE];
        int writeBufferIndex;
        
        // asbinsert.cpp
        void localInsert(int, char *, Node *);

        // asberase.cpp
        void localEraseLeaf(Node *, int);
        void localEraseBranch(Node *, int);
        void mergeLeaf(Node *, Node *, Node *, int);
        void mergeBranch(Node *, Node *, Node *, int);

        // asbmisc.cpp
        char * findInNode(Node *, int);
        Node* findNode(int, Node *);
        void put(Node *, int, char*);
        void remove(Node *, int);
        void removeDiscrim(Node *, int);
        Node * allocateNode(enum NodeType);
        void splitLeaf(Node *, int insKey, char* insValue);
        void splitBranch(Node *);
        int findParent(int, int);
        int getCardinality();
        int getCardinalityRecursive(Node *);

        int openFile(std::string filename) {
            int file = ::open(filename.c_str(), O_RDWR | O_CREAT);
            #ifdef DEBUG
            assert(file);
            #endif
            return file;
        }

        void closeFile(int file) {
            int fileNum = file;
            ::close(file);
            std::string filename = fileNames[fileNum];
            ::remove(filename.c_str());
            files.erase(fileNum);
            fileNames.erase(fileNum);
            numValidNodesInFile.erase(fileNum);
        }

        int getMax(Node * node) {
            if (isLeaf(node)) return node->l.keys[node->card - 1];
            else return getMax(getNode(node->b.nids[getCard(node) - 1]));
        }

        int getMin(Node * node) {
            if (isLeaf(node)) return node->l.keys[0];
            else return getMin(getNode(node->b.nids[0]));
        }

        char * getPage(int file, int page) {
            lseek(file, (PAGE_SIZE * page), 0);
            char * data = new char[PAGE_SIZE]();
            ::read(file, data, PAGE_SIZE);
            numReads++;

            return data;
        }

        void setPage(int file, int page, void * data) {
            lseek(file, (PAGE_SIZE * page), 0);
            ::write(file, data, PAGE_SIZE);
            numWrites++;

        }

        void deleteNode(int nid) {
            numValidNodesInFile[addressMap[nid].file]--;
            activeLocations.erase(addressMap[nid]);
            addressMap.erase(nid);
            for (int i = 0; i < writeBufferIndex; i++) {
                if (writeBuffer[i]->id == nid) {
                    writeBuffer[i]->id = -1;
                    return;
                }
            }
            recycleOldFiles();
        }

        void flushWriteBuffer() {
            for (int i = 0; i < writeBufferIndex; i++) {
                if (writeBuffer[i]->id == -1) {
                    continue;
                }
                setPage(currentFile, currentFileWriteCount, writeBuffer[i]);
                numValidNodesInFile[currentFile]++;
                numValidNodesInFile[addressMap[writeBuffer[i]->id].file]--;
                activeLocations.erase(addressMap[writeBuffer[i]->id]);
                addressMap[writeBuffer[i]->id] = NodeLocation{currentFile, currentFileWriteCount};
                activeLocations.insert(addressMap[writeBuffer[i]->id]);
                currentFileWriteCount++;
                if (currentFileWriteCount >= NODES_PER_FILE) {
                    currentFile = addNewFile();
                    currentFileWriteCount = 0;
                    recycleOldFiles();
                }
            }
            writeBufferIndex = 0;
        }

        void recycleOldFiles() {
            auto it = files.begin();
            while (it != files.end()) {
                int file = *it;
                if (numValidNodesInFile[file] < FILE_MIN_ACTIVE_NODES && file != currentFile) {
                    for (int i = 0; i < NODES_PER_FILE; i++) {
                        NodeLocation location = {file, i};
                        if (activeLocations.find(location) == activeLocations.end()) {
                            continue;
                        }
                        Node * node = (Node *) getPage(file, i);
                        setPage(currentFile, currentFileWriteCount, node);
                        numValidNodesInFile[currentFile]++;
                        activeLocations.erase(addressMap[node->id]);
                        addressMap[node->id] = NodeLocation{currentFile, currentFileWriteCount};
                        activeLocations.insert(addressMap[node->id]);
                        currentFileWriteCount++;
                        if (currentFileWriteCount >= NODES_PER_FILE) {
                            currentFile = addNewFile();
                            currentFileWriteCount = 0;
                        }
                    }
                    closeFile(file);
                    it = files.begin();
                } else {
                    it++;
                }
            }
        }

        Node * getNode(int nid) {
            for (int i = 0; i < WRITE_BUFFER_SIZE; ++i) {
                if (writeBuffer[i]->id == nid) {
                    return writeBuffer[i];
                }
            }
            NodeLocation location = addressMap[nid];
            return (Node *) getPage(location.file, location.page);
        }

        void setNode(Node * node) {
            int id = node->id;
            for (int i = 0; i < writeBufferIndex; i++) {
                if (writeBuffer[i]->id == id) {
                    writeBuffer[i] = node;
                    return;
                }
            }
            if (writeBufferIndex == WRITE_BUFFER_SIZE) {
                flushWriteBuffer();
            }
            writeBuffer[writeBufferIndex++] = node;
        }

        int addNewFile() {
            std::string filename = baseFileName + std::to_string(nextFileNum) + ".db";
            int file = openFile(filename);
            files.insert(file);
            fileNames[file] = filename;
            numValidNodesInFile[file] = 0;
            nextFileNum++;
            return file;
        }

        int getCard(Node * node) {
            return node->card;
        }

        bool isLeaf(Node * n) {
            return n->type != BRANCH;
        }

        #ifdef DEBUG
        void printPage(Node * node, bool recurse = true) {
            if (node->type != BRANCH) {
                std::cerr << "{id:" << node->id << " ty:" << node->type << " card:" <<
                            node->card << " min:" << getMin(node) << 
                            " max:" << getMax(node) << " keys:"; 
                
                for (int i = 0; i < node->card; i++) {
                    std::cerr << "(" << node->l.keys[i] << ", " << node->l.values[i] << ")";
                }

                std::cerr << "}" << std::endl;
            } else {
                std::cerr << "{id:" << node->id << " ty:" << node->type << " card:" << 
                node->card << " ptrs:"; 
                for (int i = 0; i < node->card; i++) {
                    if (i == node->card - 1) {
                        std::cerr << "(" << node->b.nids[i] << ")";
                    } else {
                        std::cerr << "(" << node->b.nids[i] << ")" << node->b.discrim[i];
                    }
                }
                std::cerr << "}\n";
                if (recurse) {
                    for (int i = 0; i < node->card; i++) {
                        printPage(getNode(node->b.nids[i]));
                    }
                }
            }
        }
        #endif
};