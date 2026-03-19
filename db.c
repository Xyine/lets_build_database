# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/stat.h>
# include <stdbool.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>


#define USERNAME_SIZE 32
#define EMAIL_SIZE 255
#define INPUT_SIZE 300
#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

typedef enum ReadResult {
    READ_SUCCESS,
    READ_TOO_LONG
} ReadResult;

typedef enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum PrepareResult {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
} PrepareResult;

typedef enum StatementType {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef enum ExecuteResult {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_TABLE_EMPTY,
    EXECUTE_FAILURE_UNKNOWN
} ExecuteResult;

typedef enum NodeType {
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

typedef struct Row {
    uint32_t id;
    char username[USERNAME_SIZE + 1];
    char email[EMAIL_SIZE + 1];
} Row;

#define ROW_SIZE sizeof(Row)
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)

typedef struct Statement {
    StatementType type;
    Row row_to_insert;
} Statement;

typedef struct Pager {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct Table {
    uint32_t root_page_num;
    Pager *pager;
} Table;

typedef struct Cursor {
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
} Cursor;

const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_SIZE + LEAF_NODE_KEY_OFFSET;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;


Table* db_open(const char *filename);

void db_close(Table *table);

ReadResult read_input(char *input);

MetaCommandResult do_meta_command(char *input, Table *table);

Pager* pager_open(const char *filename);

void pager_flush(Pager *pager, uint32_t page_num);

PrepareResult prepare_insert(char *input, Statement *statement);

PrepareResult prepare_statement(char *input, Statement *statement);

void* get_page(Pager *pager, uint32_t page_num);

ExecuteResult execute_statement(Statement *statement, Table *table);

ExecuteResult insert_row(Statement *statement, Table *table);

ExecuteResult select_row(Table *table);

Cursor* table_start(Table *table) ;

Cursor* table_end(Table *table);

Row* cursor_value(Cursor* cursor);

void cursor_advance(Cursor *cursor);

void initialize_leaf_node(void *node);

void* leaf_node_cell(void *node, uint32_t cell_num);

void* leaf_node_value(void *node, uint32_t cell_num);

uint32_t* leaf_node_num_cells(void *node);

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value);

void print_constants();

int main(int argc, char* argv[]){
    if (argc < 2) {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }
    char *filename =  argv[1];
    Table *table = db_open(filename);

    char input[INPUT_SIZE];
    while(1){
        printf("db >");
        fflush(stdout);

        switch (read_input(input)) {
            case READ_SUCCESS:
                break;
            case READ_TOO_LONG:
                printf("Input too long.\n");
                continue;
        }

        if (input[0] == '.'){
            switch (do_meta_command(input, table)) {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unrecognized command '%s'\n", input);
                    continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input, &statement)) {
            case PREPARE_SUCCESS:
                break;
            case PREPARE_STRING_TOO_LONG:
                printf("String is too long.\n");
                continue;
            case PREPARE_NEGATIVE_ID:
                printf("ID must be positive.\n");
                continue;
            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized keyword at the start of statement '%s'\n", input);
                continue;
            case PREPARE_SYNTAX_ERROR:
                printf("Syntax error. Usage: insert <id> <username> <email>\n");
                continue;
        }

        switch (execute_statement(&statement, table)){
            case EXECUTE_SUCCESS:
                printf("Executed.\n");
                break;  
            case EXECUTE_TABLE_FULL:
                printf("Error: Table full.\n");
                break;
            case EXECUTE_TABLE_EMPTY:
                printf("Table is empty.\n");
                break;
            case EXECUTE_FAILURE_UNKNOWN:
                printf("Error: unknown statement type.\n");
                break;
        }
    }

    return 0;
}

ReadResult read_input(char *input){
    if (fgets(input, INPUT_SIZE, stdin) == NULL){
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    size_t input_length = strlen(input);

    if (input[input_length - 1] != '\n'){
        int c;
        while ((c = getchar()) != '\n' && c != EOF); // vider stdin

        return READ_TOO_LONG;
    }

    if (input_length > 0 && input[input_length - 1] == '\n') {
        input[input_length - 1] = '\0';
    }
    return READ_SUCCESS;
}

MetaCommandResult do_meta_command(char *input, Table *table){
    if (strcmp(input, ".exit") == 0){
        db_close(table);
        exit(EXIT_SUCCESS);
    } else if (strcmp(input, ".constants") == 0) {
        printf("Constants:\n");
        print_constants();
        return META_COMMAND_SUCCESS;
    }
    return META_COMMAND_UNRECOGNIZED_COMMAND;
}

PrepareResult prepare_insert(char *input, Statement *statement) {
    statement->type = STATEMENT_INSERT;
    char buffer[INPUT_SIZE];
    strcpy(buffer, input);

    strtok(buffer, " "); // skip "insert"
    char *id_str = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");

    if (id_str == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }
    
    int id = atoi(id_str);
    if (id < 0) {
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(username) > USERNAME_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(char *input, Statement *statement) {
    if (strcmp(input, "select") == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input, "insert ", 7) == 0) {
        return prepare_insert(input, statement);
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

Pager* pager_open(const char *filename) {
    int fd = open(
        filename,
        O_RDWR | // Read/Write mode
        O_CREAT, // Create file if it does not exist
        S_IWUSR | // User write permission
        S_IRUSR // User read permission
    );

    if (fd == -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager *pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length / PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0) {
        printf("Db file is not a whole number of pages. Corrupt file.\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }
    return pager;
}

void pager_flush(Pager *pager, uint32_t page_num) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }
    
    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

    if (bytes_written == -1) {
        printf("Error writting: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

Table* db_open(const char *filename) {
    Pager *pager = pager_open(filename);

    Table *table = malloc(sizeof(Table));
    if (table == NULL) {
        fprintf(stderr, "Error: could not allocate memory for new table.\n");
        exit(EXIT_FAILURE);
    }

    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0) {
        // New database file. Initialize page 0 as leaf node.
        void *root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
    }

    return table;
}

void db_close(Table *table) {
    Pager *pager = table->pager;
    for (uint32_t i = 0; i < pager->num_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void *page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }

    free(pager);
    free(table);
}

void* get_page(Pager *pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss. Allocate memory and load from file.
        void *page = malloc(PAGE_SIZE);
        if (page == NULL) {
            fprintf(stderr, "Error: could not allocate memory for page.\n");
            exit(EXIT_FAILURE);
        }
        uint32_t num_pages = pager->file_length / PAGE_SIZE;
    
        // We might save a partial page at the end of the file 
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num < num_pages) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}

ExecuteResult insert_row(Statement *statement, Table *table) {
    void *node = get_page(table->pager, table->root_page_num);
    if ((*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS)) {
        return EXECUTE_TABLE_FULL;
    }

    Cursor *cursor = table_end(table);
    Row *row_to_insert = &(statement->row_to_insert);
    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

    free(cursor);
    return EXECUTE_SUCCESS;
} 

ExecuteResult select_row(Table *table) {
    Cursor *cursor = table_start(table);

    while (!(cursor->end_of_table)) {
        Row *row = cursor_value(cursor);
        printf("(%u, %s, %s)\n", row->id, row->username, row->email);
        cursor_advance(cursor);
    }
    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table) {
    switch(statement->type) {
        case STATEMENT_INSERT:
            return insert_row(statement, table);
        case STATEMENT_SELECT:
            return select_row(table);
        default:
            return EXECUTE_FAILURE_UNKNOWN;
    }
}

Cursor* table_start(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

Cursor* table_end(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;

    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->cell_num = num_cells;
    cursor->end_of_table = true;

    return cursor;
}

Row* cursor_value(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void *page = get_page(cursor->table->pager, page_num);
    return (Row*)leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor *cursor) {
    uint32_t page_num = cursor->page_num;
    void *node = get_page(cursor->table->pager, page_num);

    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        cursor->end_of_table = true;
    }
}

uint32_t* leaf_node_num_cells(void *node) {
    return (uint32_t*)((char*)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* leaf_node_cell(void *node, uint32_t cell_num) {
    return (char*)node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void *node, uint32_t cell_num) {
    return (uint32_t*)leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void *node, uint32_t cell_num) {
    return (char*)leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void *node) {
    *leaf_node_num_cells(node) = 0;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
    void *node = get_page(cursor->table->pager, cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        // Node full
        printf("Need to implement splitting a leaf node.\n");
        exit(EXIT_FAILURE);
    }

    if (cursor->cell_num < num_cells) {
        // Make room for new cell
        for (uint32_t i = num_cells; i >cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i-1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    memcpy(leaf_node_value(node, cursor->cell_num), value, ROW_SIZE);
}

void print_constants() {
    printf("ROW_SIZE: %ld\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}