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
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define PAGE_SIZE 4096
#define ROW_SIZE sizeof(Row)
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

typedef struct Row {
    uint32_t id;
    char username[USERNAME_SIZE + 1];
    char email[EMAIL_SIZE + 1];
} Row;

typedef struct Statement {
    enum StatementType type;
    Row row_to_insert;
} Statement;

typedef struct Pager {
    int file_descriptor;
    uint32_t file_length;
    Row *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct Table {
    uint32_t num_rows; // counter for the number of rows used because we can't get it easily otherwise
    Pager  *pager;
} Table;

typedef struct Cursor {
    Table *table;
    uint32_t row_num;
    bool end_of_table; // Indicates a position one past the last element
} Cursor;

Table* db_open(const char *filename);

void db_close(Table *table);

ReadResult read_input(char *input);

MetaCommandResult do_meta_command(char *input, Table *table);

Pager* pager_open(const char *filename);

void pager_flush(Pager *pager, uint32_t page_num, uint32_t size);

PrepareResult prepare_insert(char *input, Statement *statement);

PrepareResult prepare_statement(char *input, Statement *statement);

Row* get_page(Pager *pager, uint32_t page_num);

ExecuteResult execute_statement(Statement *statement, Table *table);

ExecuteResult insert_row(Statement *statement, Table *table);

ExecuteResult select_row(Table *table);

Cursor* table_start(Table *table) ;

Cursor* table_end(Table *table);

Row* cursor_value(Cursor* cursor);

void cursor_advance(Cursor *cursor);

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
    }  
    return META_COMMAND_UNRECOGNIZED_COMMAND;
}

PrepareResult prepare_insert(char *input, Statement *statement) {
    statement->type = STATEMENT_INSERT;
    char buffer[INPUT_SIZE];
    strcpy(buffer, input);

    char *keyword = strtok(buffer, " ");
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

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }
    return pager;
}

void pager_flush(Pager *pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == NULL) {
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }
    
    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);

    if (bytes_written == -1) {
        printf("Error writting: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

Table* db_open(const char *filename) {
    Pager *pager = pager_open(filename);
    uint32_t nums_rows = pager->file_length / ROW_SIZE;

    Table *table = malloc(sizeof(Table));
    if (table == NULL) {
        fprintf(stderr, "Error: could not allocate memory for new table.\n");
        exit(EXIT_FAILURE);
    }
    table->pager = pager;
    table->num_rows =nums_rows;
    return table;
}

void db_close(Table *table) {
    Pager *pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (uint32_t i = 0; i < num_full_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // There may be a partial page to write to the end of the file
    // This should not be needed after we switch to a B-tree
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != NULL) {
            pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
    }
    int result = close(pager->file_descriptor);
    if (result == -1) {
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        Row *page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}

Row* get_page(Pager *pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss. Allocate memory and load from file.
        Row *page = malloc(PAGE_SIZE);
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
    }

    return pager->pages[page_num];
}

ExecuteResult insert_row(Statement *statement, Table *table) {
    if (table->num_rows >= ROWS_PER_PAGE * TABLE_MAX_PAGES) {
        return EXECUTE_TABLE_FULL;
    }
    Cursor *cursor = table_end(table);
    Row *row = cursor_value(cursor);
    *row = statement->row_to_insert;

    table->num_rows ++; 
    free(cursor);
    return EXECUTE_SUCCESS;
} 

ExecuteResult select_row(Table *table) {
    if (table->num_rows == 0) {
        return EXECUTE_TABLE_EMPTY;
    }
    Cursor *cursor = table_start(table);
    while (!(cursor->end_of_table)) {
        int page_num = cursor->row_num / ROWS_PER_PAGE;
        int row_offset = cursor->row_num % ROWS_PER_PAGE;
        Row *page = get_page(table->pager, page_num);
        Row *row = &page[row_offset];
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
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);

    return cursor;
}

Cursor* table_end(Table *table) {
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;

    return cursor;
}

Row* cursor_value(Cursor* cursor) {
    uint32_t row_num = cursor->row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    Row *page = get_page(cursor->table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    return &page[row_offset];
}

void cursor_advance(Cursor *cursor) {
    cursor->row_num += 1;
    if (cursor->row_num >= cursor->table->num_rows) {
        cursor->end_of_table = true;
    }
}
