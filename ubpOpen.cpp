#include "ubpTree.h"

void uBPlusTree::open(std::string filename) {
            #ifdef DEBUG
            assert(!database);
            #endif

            struct stat fileInfo;
            int statRet = stat(filename.c_str(), &fileInfo);
            int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
            database = ::open(filename.c_str(), O_RDWR | O_CREAT, mode);

            if (statRet != 0 || fileInfo.st_size < 2 * PAGE_SIZE) {
                // New tree
                int metadata[PAGE_SIZE / sizeof(int)] = {0};
                metadata[0] = 1;
                metadata[1] = 1;
                numPages = 1;
                setPage(0, (void *) &metadata);

                Node * node = allocateNode(LEAF);
                root = node->l.index;
                setPage(root, node);

            } else {
                // Existing tree
                int * metadata = (int *) getPage(0);
                root = metadata[0];
                numPages = metadata[1];
            }

            #ifdef DEBUG
            assert(database);
            assert(root > 0);
            assert(numPages > 1);
            #endif
        }

        void uBPlusTree::close() {
            #ifdef DEBUG
            assert(database);
            #endif

            int metadata[PAGE_SIZE / sizeof(int)] = {0};
            metadata[0] = root;
            metadata[1] = numPages;
            setPage(0, (void *) &metadata);

            ::close(database);
            database = 0;
        }
