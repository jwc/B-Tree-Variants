#include "ubpTree.h"

void uBPlusTree::open(std::string filename) {
            #ifdef DEBUG
            assert(!database);
            #endif

            struct stat fileInfo;
            int statRet = stat(filename.c_str(), &fileInfo);
            int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
            database = ::open(filename.c_str(), O_RDWR | O_CREAT | O_SYNC, mode);

            if (statRet != 0 || fileInfo.st_size < 2 * PAGE_SIZE) {
                // New tree
                metadata.root = 1;
                metadata.numPages = 1;
                metadata.numFreePages = 0;
                setPage(0, (void *) &metadata);

                Node * node = allocateNode(LEAF);
                metadata.root = node->l.index;
                setPage(metadata.root, node);

            } else {
                // Existing tree
                char * page = getPage(0);
                memcpy(&metadata, page, sizeof(TreeData));
            }

            #ifdef DEBUG
            assert(database);
            assert(metadata.root > 0);
            assert(metadata.numPages > 1);
            #endif
        }

        void uBPlusTree::close() {
            #ifdef DEBUG
            assert(database);
            #endif

            setPage(0, &metadata);

            ::close(database);
            database = 0;
        }
