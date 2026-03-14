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

typedef enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum PrepareResult {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
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

typedef struct Table {
    uint32_t num_rows; // counter for the number of rows used because we can't get it easily otherwise
    Row *pages[TABLE_MAX_PAGES];
} Table;


Table* new_table();

void free_table(Table *table);

void read_input(char *input);

MetaCommandResult do_meta_command(char *input, Table *table);

PrepareResult prepare_statement(char *input, Statement *statement);

ExecuteResult execute_statement(Statement *statement, Table *table);

ExecuteResult insert_row(Statement *statement, Table *table);

ExecuteResult select_row(Table *table);


int main(void){
    Table *table = new_table();
    char input[INPUT_SIZE];
    while(1){
        printf("db >");
        fflush(stdout);

        read_input(input);

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

void read_input(char *input){
    if (fgets(input, INPUT_SIZE, stdin) == NULL){
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    
    size_t input_length = strlen(input);
    
    if (input_length > 0 && input[input_length - 1] == '\n') {
        input[input_length - 1] = '\0';
    }
}

MetaCommandResult do_meta_command(char *input, Table *table){
    if (strcmp(input, ".exit") == 0){
        free_table(table);
        exit(EXIT_SUCCESS);
    }  
    return META_COMMAND_UNRECOGNIZED_COMMAND;
}

PrepareResult prepare_insert(char *input, Statement *statement){
    statement->type = STATEMENT_INSERT;

    int args_assigned = sscanf(input, "insert %u %" STR(USERNAME_SIZE) "s %" STR(EMAIL_SIZE) "s", &(statement->row_to_insert.id),
    statement->row_to_insert.username, statement->row_to_insert.email);
    if (args_assigned < 3){
        return PREPARE_SYNTAX_ERROR;
    }

    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(char *input, Statement *statement){
    if (strcmp(input, "select") == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input, "insert ", 7) == 0){
        return prepare_insert(input, statement);
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

Table* new_table(void){
    Table *table = malloc(sizeof(Table));
    if (table == NULL) {
        fprintf(stderr, "Error: could not allocate memory for new table.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < TABLE_MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    table->num_rows = 0;
    return table;
}

void free_table(Table *table){
    if (table == NULL){
        return;
    }
    for (int i = 0; i < TABLE_MAX_PAGES; i++){
        free(table->pages[i]);
    }
    free(table);
}

ExecuteResult insert_row(Statement *statement, Table *table){
    if (table->num_rows >= ROWS_PER_PAGE * TABLE_MAX_PAGES){
        return EXECUTE_TABLE_FULL;
    }
    int page_num = table->num_rows / ROWS_PER_PAGE;
    if (table->pages[page_num] == NULL) {
        table->pages[page_num] = malloc(PAGE_SIZE);
        if (table->pages[page_num] == NULL) {
            fprintf(stderr, "Error: could not allocate memory for page.\n");
            exit(EXIT_FAILURE);
        }
    }
    int row_offset = table->num_rows % ROWS_PER_PAGE;
    table->pages[page_num][row_offset] = statement->row_to_insert;
    table->num_rows ++; 
    return EXECUTE_SUCCESS;
} 

ExecuteResult select_row(Table *table){
    if (table->num_rows == 0){
        return EXECUTE_TABLE_EMPTY;
    }
    for (uint32_t i = 0; i < table->num_rows; i++){
        int page_num = i / ROWS_PER_PAGE;
        int row_offset = i % ROWS_PER_PAGE;
        Row *row = &table->pages[page_num][row_offset];
        printf("(%u, %s, %s)\n", row->id, row->username, row->email);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table){
    switch(statement->type){
        case STATEMENT_INSERT:
            return insert_row(statement, table);
        case STATEMENT_SELECT:
            return select_row(table);
        default:
            return EXECUTE_FAILURE_UNKNOWN;
    }
}
