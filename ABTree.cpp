#include "ABTree.h"

int debug = 0;  // change to 0 to disable debugging
char g_search_buff[ABT_PAGE_SIZE];
bool insertIsReplace = true;

// table header is stored on page# 0, mainly to remember root page id
typedef struct TableHeader {
    uint32_t m_version;
    uint32_t m_pageSize;
    uint32_t m_pageCount;
    uint32_t m_rootPageId;
    uint32_t m_cardinality;

    TableHeader(uint32_t pageSize, uint32_t pageCount) : m_version(PAGE0_VERSION),
                                                         m_pageSize(pageSize),
                                                         m_pageCount(pageCount),
                                                         m_rootPageId(DEFAULT_ROOT_PAGE_ID),
                                                         m_cardinality(0) {}

    TableHeader() {}

} TableHeader;

typedef struct TableFile {
    char m_schemaName[SCHEMA_NAME_MAX_LEN + 1];
    char m_tableName[TABLE_NAME_MAX_LEN + 1];
    char m_fileName[SCHEMA_NAME_MAX_LEN + 1 + TABLE_NAME_MAX_LEN + 1 + TABLE_SUFFIX_LEN + 1];
    int m_fd;
    uint32_t m_pageSize;
    uint32_t m_pageCount;
    uint32_t m_rootPageId;
    uint32_t m_cardinality;
    uint32_t m_readCount;
    uint32_t m_writeCount;

    TableFile() {
        m_fd = -1;
        m_pageSize = ABT_PAGE_SIZE;
        m_pageCount = -1;
        m_rootPageId = DEFAULT_ROOT_PAGE_ID;
        m_cardinality = 0;
        m_readCount = 0;
        m_writeCount = 0;
    }

    // creates new table, and closes it.
    int createTableFile(const char *schema, const char *table, uint32_t numPages);

    // open for insert/update/delete/search
    int openTableFile(const char *schema, const char *table);

    // close after insert/update/delete/search
    int closeTableFile();

    // read specified page from disk to caller's buffer
    // caller needs to provide a buffer for the page
    int readPage(uint32_t pageNumber, char *page) {
        int rc = 0;
        ssize_t ssize = 0;

        if (m_fd == -1) {
            fprintf(stderr, "%s:%d: readPage without init \n", __FILE__, __LINE__);
            rc = 1;
            goto error_exit;
        }

        ssize = pread(m_fd, page, ABT_PAGE_SIZE, pageNumber * ABT_PAGE_SIZE);
        if (ssize != ABT_PAGE_SIZE) {
            fprintf(stderr, "%s:%d: error during pread() %lu:%d\n", __FILE__, __LINE__, ssize, errno);
            perror("pread");
            rc = 1;
            goto error_exit;
        }
        m_readCount++;
    exit:
        return rc;

    error_exit:
        // error handlign if any
        goto exit;
    }

    // write a page to the disk
    int writePage(uint32_t pageNumber, char *page) {
        int rc = 0;
        ssize_t ssize = 0;

        if (m_fd == -1) {
            fprintf(stderr, "%s:%d: writePage without init \n", __FILE__, __LINE__);
            rc = 1;
            goto error_exit;
        }

        ssize = pwrite(m_fd, page, ABT_PAGE_SIZE, pageNumber * ABT_PAGE_SIZE);
        if (ssize != ABT_PAGE_SIZE) {
            fprintf(stderr, "%s:%d: error during pwrite() %lu\n", __FILE__, __LINE__, ssize);
            rc = 1;
            goto error_exit;
        }
        m_writeCount++;
    exit:
        return rc;

    error_exit:
        // error handlign if any
        goto exit;
    }

} TableFile;

void copyValue(char *dst, const char *src, int len) {
    int i = 0;
    for (; i < len && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    for (; i < len; i++) {
        dst[i] = '\0';
    }
    dst[len-1] = '\0';
}

struct KeyValue {
    int key; // changed from uint32_t
    char value[MAX_VAL_LEN];
};

// Custom comparator function to compare KeyValue objects by key
int compare(const void *a, const void *b) {
    const KeyValue *pa = reinterpret_cast<const KeyValue *>(a);
    const KeyValue *pb = reinterpret_cast<const KeyValue *>(b);
    if (pa->key < pb->key) return -1;
    if (pa->key > pb->key) return 1;
    return 0;
}

typedef enum OpType {
    INSERT,
    UPDATE, 
    DELETE
} OpType;

typedef struct Entry {
    int indexKey;
    OpType type;
    char value[MAX_VAL_LEN];
} Entry;

typedef struct Bucket {
    Entry       entry[MAX_BUCKET_SIZE];
    uint32_t    bucketCard;
} Bucket;

typedef struct Node {
    uint32_t    m_eyeCatcher;  // for debugging purpose
    uint32_t    m_pageNumber;  // for debugging purpose
    char        isLeaf;
    uint32_t    child[MAX_NUM_BUCKETS]; // offset for children
    Bucket      bucket[MAX_NUM_BUCKETS];
    int         key[MAX_NUM_KEYS];
    char        value[MAX_NUM_KEYS][MAX_VAL_LEN];
    uint32_t    nodeCard;

    Node(uint32_t pageNumber) {
        m_eyeCatcher = 0xdeadbeef;
        m_pageNumber = pageNumber;
        nodeCard = 0;
        for (auto &c : child) {
            c = 0;
        }
        for (auto &b : bucket) {
            b.bucketCard = 0;
            for (auto &e: b.entry) {
                e.indexKey = 0;
                e.value[0] = '\0';
            }
        }
    }

    void print() {
        printf("Node Index: %d\n", m_pageNumber);
        printf("Node card: %d\n", nodeCard);
        printf("Node [key:value]: ");
        for (int i = 0; i < nodeCard; i++) {
            printf("[%d:%s] ", key[i], value[i]);
        }
        printf("\n");
        printf("\tNode Children: ");
        for (int i = 0; i < MAX_BUCKET_SIZE; i++) {
            printf("%d ", child[i]);
        }
        printf("\n");
        for (int i = 0; i < MAX_NUM_BUCKETS; i++) {
            printf("\tNode Bucket: %d\n", i);
            printf("\t\tBucket Card: %d\n", bucket[i].bucketCard);
            printf("\t\tBucket keys: ");
            for (int j = 0; j < MAX_BUCKET_SIZE; j++) {
                printf("%d ", bucket[i].entry[j].indexKey);
            }
            printf("\n");
            printf("\t\tBucket Values: ");
            for (int j = 0; j < MAX_BUCKET_SIZE; j++) {
                printf("%s ", bucket[i].entry[j].value);
            }
            printf("\n");
        }
        printf("\n");
    }
} Node;

static_assert(sizeof(Node) < ABT_PAGE_SIZE);

typedef struct Table {

    TableFile tableFile;

    static int reorganize(int key[MAX_BUCKET_SIZE], char value[MAX_BUCKET_SIZE], Node * node) {
        return 0;
    }

    // returns 0 on success, RC_DUPLICATE_KEY on duplicate-key, or other error codes
    // todo: implement this
    int insertHelper(int key, const char *value, Node *node, char *page) {
        //  if node has space in keylist
        //      insert into node's keylist
        //  else if proper bucket in node has empty space
        //      insert into proper bucket (remember to increase bucket's card)
        //  else
        //    split bucket into childNode
        //    insert_helper(childNode)


        // first check if keylist contains key already
        for (int k = 0; k < node->nodeCard; k++) {
            if (node->key[k] == key) {
                if (insertIsReplace) {
                    // memset(node->value[k], '\0', MAX_VAL_LEN);
                    // memcpy(node->value[k], value, MAX_VAL_LEN);
                    copyValue(node->value[k], value, MAX_VAL_LEN);
                    return tableFile.writePage(node->m_pageNumber, (char *) node);
                } else {
                    return RC_DUPLICATE_KEY;
                }
            }
        }
        // Now you can try to insert in keylist
        //      (No need to check if buckets contain key already yet since 
        //      buckets are guaranteed to be empty if nodeCard < MAX_NUM_KEYS)
        if (node->nodeCard < MAX_NUM_KEYS) {
            int i = node->nodeCard - 1;
            while (i >= 0 && key < node->key[i]) {
                node->key[i+1] = node->key[i];
                memset(node->value[i+1], '\0', MAX_VAL_LEN);
                memcpy(node->value[i+1], node->value[i], MAX_VAL_LEN);
                i--;
            }
            node->key[i+1] = key;
            memset(node->value[i+1], '\0', MAX_VAL_LEN);
            memcpy(node->value[i+1], value, MAX_VAL_LEN);
            node->nodeCard++;
            return tableFile.writePage(node->m_pageNumber, (char *) node);
        }
        // find proper bucket
        int bucketIndex = 0;
        while (bucketIndex < MAX_NUM_KEYS && key > node->key[bucketIndex]) {
            bucketIndex++;
        }
        //  if bucket has child node
        if (node->child[bucketIndex]) {
            // Node * newNode = nullptr;
            char p[ABT_PAGE_SIZE];
            int rc = getNode(p, node->child[bucketIndex], &node);
            return insertHelper(key, value, node, page);
        } else {
            // Check if bucket contains key already
            for (int k = 0; k < node->bucket[bucketIndex].bucketCard; k++) {
                if (node->bucket[bucketIndex].entry[k].indexKey == key) {
                    if (insertIsReplace) {
                        copyValue(node->bucket[bucketIndex].entry[k].value, value, MAX_VAL_LEN);
                        return tableFile.writePage(node->m_pageNumber, (char *) node);
                    } else {
                        return RC_DUPLICATE_KEY;
                    }
                }
            }

            // if bucket has space
            if (node->bucket[bucketIndex].bucketCard < MAX_BUCKET_SIZE) {
                uint32_t card = node->bucket[bucketIndex].bucketCard;
                node->bucket[bucketIndex].entry[card].indexKey = key;
                memset(node->bucket[bucketIndex].entry[card].value, '\0', MAX_VAL_LEN);
                memcpy(node->bucket[bucketIndex].entry[card].value, value, MAX_VAL_LEN);
                node->bucket[bucketIndex].bucketCard++;
                return tableFile.writePage(node->m_pageNumber, (char *) node);
            } else {
                // splitting here
                
                // store contents of buckets into keyvaluepairs
                KeyValue keyValuePairs[MAX_BUCKET_SIZE + 1];
                for (int j = 0; j < MAX_BUCKET_SIZE; j++) {
                    Entry e = node->bucket[bucketIndex].entry[j];
                    keyValuePairs[j].key = e.indexKey;
                    strcpy(keyValuePairs[j].value, e.value);
                }
                // storing new value to be inserted
                keyValuePairs[MAX_BUCKET_SIZE].key = key;
                strcpy(keyValuePairs[MAX_BUCKET_SIZE].value, value);

                // sort keyvaluepairs
                qsort(keyValuePairs, MAX_BUCKET_SIZE+1, sizeof(KeyValue), compare);
                
                // allocate new node
                uint32_t index;
                char p[ABT_PAGE_SIZE];
                int rc = allocatePage(p, &index);
                Node *child = (Node *) p;
                child->m_eyeCatcher = 0xdeadbeef;
                child->m_pageNumber = index;
                child->nodeCard = 0;
                for (auto &c : child->child) {
                    c = 0;
                }
                for (auto &b : child->bucket) {
                    b.bucketCard = 0;
                    for (auto &e : b.entry) {
                        e.indexKey = 0;
                        e.value[0] = '\0';
                    }
                }
                float temp = (MAX_BUCKET_SIZE+1 - MAX_NUM_KEYS) / (MAX_NUM_BUCKETS*1.0);
                // making sure this value is never below 0 (though it shouldn't be anyways)
                temp = temp < 0 ? 0 : temp;
                int n = ceil(temp);
                int cnt = 0;
                int keyIdx = 0;
                int numInBuckets = 0;

                // Putting key value pairs back into n
                //  for each bucket b
                //      while i < n && cnt < MAX_BUCKET_SIZE 
                //          b.entry[i].key = key[cnt]
                //          b.entry[i++].value = value[cnt++]
                //          if keyIDX < max_num_keys && cnt < maxbucketsize:
                //              child->key[keyIdx] = key[cnt]
                //              child->value[keyIdx++] = value[cnt++]
                for (auto &b : child->bucket) {
                    int i = 0;
                    // while bucket has less values than it should have && all values have not been inserted yet && inserting into key list is prioritized
                    while (i < n && cnt < MAX_BUCKET_SIZE+1 && numInBuckets + MAX_NUM_KEYS < MAX_BUCKET_SIZE+1) {
                        b.entry[i].indexKey = keyValuePairs[cnt].key;
                        strcpy(b.entry[i++].value, keyValuePairs[cnt++].value);
                        b.bucketCard++;
                        numInBuckets++;
                    }
                    if (keyIdx < MAX_NUM_KEYS && cnt < MAX_BUCKET_SIZE+1) {
                        child->key[keyIdx] = keyValuePairs[cnt].key;
                        strcpy(child->value[keyIdx++], keyValuePairs[cnt++].value);
                        child->nodeCard++;
                    }
                }
                assert(cnt == MAX_BUCKET_SIZE+1);
                node->child[bucketIndex] = child->m_pageNumber;

                // write child node to disk
                rc = tableFile.writePage(child->m_pageNumber, (char *) child);
                if (rc) return rc;
                // write parent node to disk
                rc = tableFile.writePage(node->m_pageNumber, (char *) node);
                return rc;
            }
        }
        return 1;
    }

    int insert(int key, const char *value) {
        Node * root = nullptr;
        char page[ABT_PAGE_SIZE];
        int rc = getRootNode(page, &root);
        if (rc) goto error_exit;
        rc = insertHelper(key, value, root, &page[0]);
        if (!rc) tableFile.m_cardinality++;
        if (rc) goto error_exit;
exit:
        return rc;
error_exit:
        // error handling if any
        if (rc == RC_DUPLICATE_KEY) {
            if (debug) printf("Duplicate Key: %d not inserted\n", key);
        }
        goto exit;
    }

    int updateHelper(int key, const char *value, Node * node, char *page) {
        int i = 0;
        // loop through node's keys
        while (i < node->nodeCard && key > node->key[i]) {
            i++;
        }
        // if key is found in keylist
        if (i < node->nodeCard && key == node->key[i]) {
            memset(node->value[i], '\0', MAX_VAL_LEN);
            memcpy(node->value[i], value, MAX_VAL_LEN);
            tableFile.writePage(node->m_pageNumber, (char *) node);
            return 0;
        }
        //  if node->child[i] > 0:
            // return updateHelper(key, childNode)
        // else
            // search bucket & update
        if (node->child[i]) {
            int rc = getNode(page, node->child[i], &node);
            return updateHelper(key, value, node, page);
        } else {
            for (int j = 0; j < node->bucket[i].bucketCard; j++) {
                if (node->bucket[i].entry[j].indexKey == key) {
                    memset(node->bucket[i].entry[j].value, '\0', MAX_VAL_LEN);
                    memcpy(node->bucket[i].entry[j].value, value, MAX_VAL_LEN);
                    tableFile.writePage(node->m_pageNumber, (char *) node);
                    return 0;
                }
            }
        }
        return RC_NOT_FOUND;
    }
    // returns 0 on success, non-zero on error
    // todo: implement this
    int update(int key, const char *newValue) {
        Node * root = nullptr;
        char page[ABT_PAGE_SIZE];
        int rc = getRootNode(page, &root);
        if (rc) goto error_exit;
        rc = updateHelper(key, newValue, root, &page[0]);
        if (rc) goto error_exit;
        // if (rc == RC_NOT_FOUND) {
        //     printf("Key %d not found", key);
        //     rc = 0;
        //     goto exit;
        // }
exit:
        return rc;
error_exit:
        // error handling if any
        if (rc == RC_NOT_FOUND) {
            printf("Key %d not found\n", key);
            rc = 0;
        }
        goto exit;
    }

    // returns 0 on success, non-zero on error
    // todo: implement this
    int deleteRow(int key) {
        fprintf(stderr, "%s not implemented\n", __func__);
        return 1;
    }

    // returns 0 on found, RC_NOT_FOUND if not found, or other error codes
    // todo: implement this
    int searchHelper(int key, Node * node, char * outValue, char * page){
        int i = 0;
        // loop through node's keys
        while (i < node->nodeCard && key > node->key[i]) {
            i++;
        }
        // if key is found in keylist
        if (i < node->nodeCard && key == node->key[i]) {
            memset(outValue, '\0', MAX_VAL_LEN);
            memcpy(outValue, node->value[i], MAX_VAL_LEN);
            return 0;
        }
        //  if node->child[i] > 0:
            // return searchHelper(key, childNode)
        // else
            // search bucket
        if (node->child[i]) {
            int rc = getNode(page, node->child[i], &node);
            return searchHelper(key, node, outValue, page);
        } else {
            for (int j = 0; j < node->bucket[i].bucketCard; j++) {
                if (node->bucket[i].entry[j].indexKey == key) {
                    memset(outValue, '\0', MAX_VAL_LEN);
                    memcpy(outValue, node->bucket[i].entry[j].value, MAX_VAL_LEN);
                    return 0;
                }
            }
        }
        return RC_NOT_FOUND;
    }
    int search(int key, char *outValue) {
        Node * root = nullptr;
        char page[ABT_PAGE_SIZE];
        int rc = getRootNode(page, &root);
        if (rc) goto error_exit;
        rc = searchHelper(key, root, outValue, &page[0]);
        if (rc) goto error_exit;
exit:
        return rc;
error_exit:
        // error handling if any
        if (rc == RC_NOT_FOUND) {
            if (debug) printf("Key %d not found\n", key);
        }
        goto exit;
    }

    static int openTable(const char *schemaName, const char *tableName, Table **table);

    static int closeTable(Table *table);

    // given the page number, find the node
    // caller should provide a buffer for the page
    int getNode(char *page, uint32_t pageNumber, Node **outNode) {
        int rc = 0;
        rc = tableFile.readPage(pageNumber, page);
        if (!rc) *outNode = (Node *)page;
        return rc;
    }

    // find the root node
    // caller should provide a buffer for the page
    int getRootNode(char *page, Node **rootNode) {
        return getNode(page, tableFile.m_rootPageId, rootNode);
    }
    // set root node number in memory and on disk
    int setRootNode(uint32_t rootPageNum) {
        int rc = 0;
        char page[ABT_PAGE_SIZE];
        TableHeader tableHeader;  // for reading and then writing

        // update on disk
        rc = tableFile.readPage(0, page);
        if (rc) goto error_exit;
        memcpy(&tableHeader, &page[0], sizeof(tableHeader));
        tableHeader.m_rootPageId = rootPageNum;
        memcpy(&page[0], &tableHeader, sizeof(tableHeader));
        rc = tableFile.writePage(0, page);
        if (rc) goto error_exit;

        // update in memory
        tableFile.m_rootPageId = rootPageNum;

    exit:
        return rc;

    error_exit:
        // error handling if any
        goto exit;
    }

    // caller should provide a buffer for the page
    int allocatePage(char *page, uint32_t *outPageNum) {
        int rc = 0;
        uint32_t targetPageNumber = 0;
        int byte = 0;
        int bit = 0;
        char full = 0xFF;

        // read bitmap from page 1
        rc = tableFile.readPage(1, page);
        if (rc) goto error_exit;

        // find empty page
        for (byte = 0; byte < ABT_PAGE_SIZE; byte++) {
            if (page[byte] != full) {  // not full
                break;
            }
        }
        if (byte == ABT_PAGE_SIZE) {
            // not found
            fprintf(stderr, "%s:%d: allocatePage did not find a page \n", __FILE__, __LINE__);
            rc = 1;
            goto error_exit;
        }

        for (bit = 0; bit < 8; bit++) {
            if ((page[byte] & (((unsigned char)1) << bit)) == 0) {
                // found the bit
                break;
            }
        }

        targetPageNumber = byte * 8 + bit;
        if (debug) {
            printf("allocatePage found a page %d\n", targetPageNumber);
        }

        // mark the page allocated
        page[byte] |= (((unsigned char)1) << bit);

        // write the bitmap
        rc = tableFile.writePage(1, page);
        if (rc) goto error_exit;

        // return newly allocated page
        rc = tableFile.readPage(targetPageNumber, page);
        if (rc) goto error_exit;

        *outPageNum = targetPageNumber;

    exit:
        return rc;

    error_exit:
        // error handling if any
        goto exit;
    }

} Table;

int Table::openTable(const char *schemaName, const char *tableName, Table **table) {
    int rc = 0;
    Table *table1 = new Table();
    // todo: error handle: if unable to allocate memory
    rc = table1->tableFile.openTableFile(schemaName, tableName);
    *table = table1;
    return rc;
}

int Table::closeTable(Table *table) {
    int rc = table->tableFile.closeTableFile();
    delete (table);
    return rc;
}

/*
typedef struct TableList {
        // future_todo: doubly link list
        // for now: only one table:
        Table *theTable;
        TableList() { theTable = nullptr; }
}TableList;

TableList g_tableList;
*/

int TableFile::createTableFile(const char *schemaName, const char *tableName, uint32_t pageCount) {
    int rc = 0;
    char page[ABT_PAGE_SIZE];
    TableHeader tableHeader(ABT_PAGE_SIZE, pageCount);
    Node node(DEFAULT_ROOT_PAGE_ID);

    // check schema name length
    if (strlen(schemaName) > SCHEMA_NAME_MAX_LEN) {
        fprintf(stderr, "%s:%d: Schema name is larger than max allowed %d\n", __FILE__, __LINE__, SCHEMA_NAME_MAX_LEN);
        rc = 1;
        goto error_exit;
    }

    // check table name length
    if (strlen(tableName) > TABLE_NAME_MAX_LEN) {
        fprintf(stderr, "%s:%d: Table name is larger than max allowed %d\n", __FILE__, __LINE__, TABLE_NAME_MAX_LEN);
        rc = 1;
        goto error_exit;
    }

    snprintf(m_schemaName, sizeof(m_schemaName), "%s", schemaName);
    snprintf(m_tableName, sizeof(m_tableName), "%s", tableName);

    // construct table file name
    snprintf(m_fileName, sizeof(m_fileName), "%s.%s.dat", schemaName, tableName);
    if (debug) {
        fprintf(stderr, "creating table-file %s\n", m_fileName);
    }

    // open table file for create
    m_fd = open(m_fileName, O_RDWR | O_CREAT, 0644);
    if (m_fd == -1) {
        perror("open");
        rc = 1;
        goto error_exit;
    }
    m_rootPageId = DEFAULT_ROOT_PAGE_ID;

    // pre-allocate file
    if (ftruncate(m_fd, pageCount * ABT_PAGE_SIZE) == -1) {
        perror("ftruncate");
        rc = 1;
        goto error_exit;
    }
    /*
    if (fallocate(m_fd, FALLOC_FL_ZERO_RANGE, 0, pageCount * ABT_PAGE_SIZE) == -1) {
        perror("fallocate");
        rc = 1;
        goto error_exit;
    }
    */

    // init page-0: read, copy header, write
    rc = readPage(0, page);
    if (rc) goto error_exit;
    memcpy(&page[0], &tableHeader, sizeof(tableHeader));
    rc = writePage(0, page);
    if (rc) goto error_exit;

    // init page-1: bitmap:	mark page 0, 1, 2 as allocated
    rc = readPage(1, page);
    if (rc) goto error_exit;
    page[0] = (1 << 0) | (1 << 1) | (1 << 2);
    rc = writePage(1, page);
    if (rc) goto error_exit;

    // init page-2: root node on page#2
    rc = readPage(2, page);
    if (rc) goto error_exit;
    memcpy(&page[0], &node, sizeof(Node));
    rc = writePage(2, page);
    if (rc) goto error_exit;

exit:
    if (m_fd != -1) {
        // Close the file.
        int rc2 = 0;
        rc2 = fsync(m_fd);
        if (rc2) {
            perror("fsync");
            if (!rc) rc = rc2;
        }
        rc2 = close(m_fd);
        if (rc2) {
            perror("close");
            // if there had been error before then preserve it, else set it.
            if (!rc) rc = rc2;
        }
        m_fd = -1;
    }
    return rc;

error_exit:
    // error handling if any
    goto exit;
}

int createTable(const char *schemaName, const char *tableName, uint32_t numPages) {
    TableFile tableFile;
    return tableFile.createTableFile(schemaName, tableName, numPages);
}

int TableFile::openTableFile(const char *schemaName, const char *tableName) {
    int rc = 0;

    // check schema name length
    if (strlen(schemaName) > SCHEMA_NAME_MAX_LEN) {
        fprintf(stderr, "%s:%d: Schema name is larger than max allowed %d\n", __FILE__, __LINE__, SCHEMA_NAME_MAX_LEN);
        rc = 1;
        goto error_exit;
    }

    // check table name length
    if (strlen(tableName) > TABLE_NAME_MAX_LEN) {
        fprintf(stderr, "%s:%d: Table name is larger than max allowed %d\n", __FILE__, __LINE__, TABLE_NAME_MAX_LEN);
        rc = 1;
        goto error_exit;
    }

    snprintf(m_schemaName, sizeof(m_schemaName), "%s", schemaName);
    snprintf(m_tableName, sizeof(m_tableName), "%s", tableName);

    // construct table file name
    snprintf(m_fileName, sizeof(m_fileName), "%s.%s.dat", schemaName, tableName);
    if (debug) {
        fprintf(stderr, "opening table-file %s\n", m_fileName);
    }

    // open table file for read/write
    m_fd = open(m_fileName, O_RDWR);
    if (m_fd == -1) {
        perror("open");
        rc = 1;
        goto error_exit;
    }

    // init page-0: read, copy header, write
    {
        TableHeader tableHeader;  // for reading
        char page[ABT_PAGE_SIZE];

        rc = readPage(0, page);
        if (rc) goto error_exit;
        memcpy(&tableHeader, &page[0], sizeof(tableHeader));

        if (tableHeader.m_version > PAGE0_VERSION) {
            fprintf(stderr, "%s:%d: wronge page0 header version %d\n", __FILE__, __LINE__, tableHeader.m_version);
            rc = 1;
            goto error_exit;
        }
        if (tableHeader.m_pageSize != ABT_PAGE_SIZE) {
            fprintf(stderr, "%s:%d: wronge page0 header ABT_PAGE_SIZE %d\n", __FILE__, __LINE__, tableHeader.m_pageSize);
            rc = 1;
            goto error_exit;
        }

        m_pageSize = tableHeader.m_pageSize;
        m_pageCount = tableHeader.m_pageCount;
        m_rootPageId = tableHeader.m_rootPageId;
        m_cardinality = tableHeader.m_cardinality;
    }

exit:
    return rc;

error_exit:
    // error handling if any
    goto exit;
}

int TableFile::closeTableFile() {
    int rc = 0;
    int rc2 = 0;
    TableHeader tableHeader;
    char page[ABT_PAGE_SIZE];
    if (m_fd == -1) {
        fprintf(stderr, "%s:%d: close table file without init \n", __FILE__, __LINE__);
        rc = 1;
        goto error_exit;
    }
    rc = readPage(0, page);
    if (rc) goto error_exit;
    memcpy(&page[0], &tableHeader, sizeof(tableHeader));
    tableHeader.m_cardinality = m_cardinality;
    memcpy(&tableHeader, &page[0], sizeof(tableHeader));
    rc = writePage(0, page);
    if (rc) goto error_exit;
exit:
    if (m_fd != -1) {
        rc2 = fsync(m_fd);
        if (rc2) {
            perror("fsync");
            // if there had been error before then preserve it, else set it.
            if (!rc) rc = rc2;
        }
        rc2 = close(m_fd);
        if (rc2) {
            perror("close");
            // if there had been error before then preserve it, else set it.
            if (!rc) rc = rc2;
        }
    }
    m_fd = -1;
    return rc;

error_exit:
    // error handling if any
    goto exit;
}


ABTree::ABTree(const char * schema, const char * tableName, bool doCreateTable) : Tree("") {
    int rc = 0;
    table = nullptr;
    tableOpened = false;
    if (doCreateTable) {
        rc = createTable(schema, tableName, 4096);
        if (rc) goto error_exit;
    }
    rc = Table::openTable(schema, tableName, &table);
    if (rc) goto error_exit;
    tableOpened = true;
exit:
    return;
error_exit:
    // error handling if any
    printf("ERROR: %d\n", rc);
    goto exit;
}

ABTree::~ABTree() {
    if (tableOpened) {
        int rc2 = Table::closeTable(table);
    }
}

void ABTree::write(int key, char value[MAX_VAL_LEN]) {
    if (debug) printf("Inserting %d\n", key);
    int rc = table->insert(key, value);
    numReads = table->tableFile.m_readCount;
    numWrites = table->tableFile.m_writeCount;
    if (debug) {
        Node * r;
        char page[ABT_PAGE_SIZE];
        rc = table->getRootNode(page, &r);
        r->print();
    }
}

char * ABTree::read(int key) {
    int rc = 0;
    g_search_buff[0] = '\0';
    rc = table->search(key, g_search_buff);
    numReads = table->tableFile.m_readCount;
    numWrites = table->tableFile.m_writeCount;
    if (rc != RC_NOT_FOUND) return g_search_buff;
    return nullptr;
}

#ifdef DEBUG
int ABTree::getCardinality() {
    return table->tableFile.m_cardinality;
}
#endif

int abtTest2() {
    Tree *t = new ABTree("mydb", "table1");

    int x = 0; // Lower bound (inclusive)
    int y = 10; // Upper bound (inclusive)
    int size = 13; // Size of the vector

    // Seed the random number generator for reproducibility (optional)
    std::mt19937 gen(12345);
    std::vector<int> arr(size);
    std::uniform_int_distribution<> dis(x, y);

    // Generate random integers and insert them into the vector
    for (int i = 0; i < size; ++i) {
        arr[i] = dis(gen);
    }

    // bulk insert
    for (int key : arr) {
        if (debug) printf("Inserting %d\n", key);
        char val[MAX_VAL_LEN];
        sprintf(val, "%d", key);
        t->write(key, val);
    }
    
    #ifdef DEBUG
    printf("Cardinality: %d\n", t->getCardinality());
    #endif

    // char insertValue[MAX_VAL_LEN];
    // sprintf(insertValue, "%d", 1);
    // t->write(1, insertValue);
    // char * searchValue = t->read(1);
    // printf("Value is %s\n", searchValue);
    return 0;
    // delete tree;
}

int abtTest3() {
    Tree *t = new ABTree("mydb", "table1");
    t->write(1, (char *) "one");
    t->write(5, (char *) "five");
    t->write(13, (char *) "thirteen");
    const char *value1 = t->read(5);
    printf("Before Update value is %s\n", value1);
    t->write(5, (char *) "fifty");
    const char *value2 = t->read(5);
    printf("After Update value is %s\n", value2);

    return 0;

}
int abtTest1() {
    int rc = 0;
    char outValue[MAX_VAL_LEN];
    bool tableOpened = false;
    Table *table = nullptr;
    Node * r;
    char page[ABT_PAGE_SIZE];
    char updatedVal[MAX_VAL_LEN] = "hello\0";

    int x = -5; // Lower bound (inclusive)
    int y = 15; // Upper bound (inclusive)
    int size = 5; // Size of the vector

    // Seed the random number generator for reproducibility (optional)
    std::mt19937 gen(12345);
    std::vector<int> arr(size);
    std::uniform_int_distribution<> dis(x, y);

    // Generate random integers and insert them into the vector
    for (int i = 0; i < size; ++i) {
        arr[i] = dis(gen);
    }

    rc = createTable("mydb", "table1", 4096);
    if (rc) goto error_exit;

    rc = Table::openTable("mydb", "table1", &table);
    if (rc) goto error_exit;
    tableOpened = true;

/*
    // bulk insert
    for (int key : arr) {
        if (debug) printf("Inserting %d\n", key);
        char val[MAX_VAL_LEN];
        sprintf(val, "%d", key);
        rc = table->insert(key, val);
        if (rc) goto error_exit;
    }

    // bulk update
    for (int key : arr) {
        if (debug) printf("Updating %d\n", key);
        char val[MAX_VAL_LEN];
        sprintf(val, "%d_n", key);
        rc = table->update(key, val);
        if (rc) goto error_exit;
    }

    // bulk search
    for (int key : arr) {
        printf("Searching %d ", key);
        char val[MAX_VAL_LEN];
        char outVal[MAX_VAL_LEN];
        sprintf(val, "%d", key);
        rc = table->search(key, &outVal[0]);
        if (outVal) printf("[%d:%s]\n", key, outVal);
        if (rc) goto error_exit;
    }
    // searching non existing keys
*/


    // bulk random
    for (int i = 0; i < arr.size(); i++) {
        std::random_device rd;  // Obtain a random seed from the OS entropy device
        std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(0, 2); // Distribution from 0 to 2 (inclusive)
        int num = distrib(gen);
        int key = arr[i];
        if (num == 0) {
            printf("Updating %d\n", key);
            char val[MAX_VAL_LEN];
            sprintf(val, "%d_n", key);
            rc = table->update(key, val);
            if (rc) goto error_exit;
        } else if (num == 1) {
            printf("Inserting %d\n", key);
            char val[MAX_VAL_LEN];
            sprintf(val, "%d", key);
            rc = table->insert(key, val);
            if (rc) goto error_exit;
        } else if (num == 2) {
            printf("Searching %d ", key);
            char outVal[MAX_VAL_LEN];
            outVal[0] = '\0';
            rc = table->search(key, &outVal[0]);
            if (outVal[0] != '\0') printf("[%d:%s]\n", key, outVal);
            if (rc) goto error_exit;
        }
    }

    for (int i = 2; i < 13; i ++) {
        rc = table->getNode(page, i, &r);
        if (!rc) r->print();
        printf("\n");
    }
/*
// root node
    rc = table->insert(5, "5");
    if (rc) goto error_exit;
    rc = table->insert(15, "15");
    if (rc) goto error_exit;
    rc = table->insert(10, "10");
    if (rc) goto error_exit;
    rc = table->insert(7, "7");
    if (rc) goto error_exit;
    rc = table->insert(8, "8");
    if (rc) goto error_exit;

// child node (split)
    rc = table->insert(17, "17");
    if (rc) goto error_exit;
    rc = table->insert(18, "18");
    if (rc) goto error_exit;
    rc = table->insert(19, "19");
    if (rc) goto error_exit;
    rc = table->insert(20, "20");
    if (rc) goto error_exit;
    rc = table->insert(21, "21");
    if (rc) goto error_exit;
// child of child (split)
    rc = table->insert(22, "22");
    if (rc) goto error_exit;
    rc = table->insert(23, "23");
    if (rc) goto error_exit;
    rc = table->insert(24, "24");
    if (rc) goto error_exit;
    rc = table->insert(25, "25");
    if (rc) goto error_exit;
    rc = table->insert(26, "26");
    if (rc) goto error_exit;
// duplicate in child of child
    rc = table->insert(26, "26");
    if (rc) goto error_exit;

// root node
    rc = table->search(8, &outValue[0]);
    if (!rc) {
        printf("output value for key %d is: %s \n", 8, &outValue[0]);
    }
// child of child
    rc = table->update(26, &updatedVal[0]);
    if (!rc) {
        printf("Updated value for key %d is: %s \n", 26, &updatedVal[0]);
    }
// child of child
    rc = table->search(18, &outValue[0]);
    if (!rc) {
        printf("output value for key %d is: %s \n", 18, &outValue[0]);
    }

    printf("\n");

    rc = table->getRootNode(page, &r);
    r->print();

    printf("\n");

    rc = table->getNode(page, 3, &r);
    r->print();

    printf("\n");

    rc = table->getNode(page, 4, &r);
    r->print();
*/
    

exit:
    if (tableOpened) {
        int rc2 = Table::closeTable(table);
        // if there had been error before then preserve it, else set it.
        if (!rc) rc = rc2;
    }
    return rc;

error_exit:
    // error handling if any
    printf("ERROR: %d\n", rc);
    goto exit;
}