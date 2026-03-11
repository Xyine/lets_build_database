# include <stdbool.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>


#define USERNAME_SIZE 32
#define EMAIL_SIZE 255
#define INPUT_SIZE 100
#define TABLE_MAX_ROWS 100


typedef enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum PrepareResult {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef enum StatementType {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef enum ExecuteResult {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_TABLE_EMPTY
} ExecuteResult;

typedef struct Row {
    uint32_t id;
    char username[USERNAME_SIZE];
    char email[EMAIL_SIZE];
} Row;

typedef struct Statement {
    enum StatementType type;
    Row row_to_insert;
} Statement;

typedef struct Table {
    uint32_t num_rows; // // counter for the number of rows used because we can't get it easily otherwise
    Row *row[TABLE_MAX_ROWS];
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
        }
    }

    return 0;
}

void read_input(char *input){
    if (fgets(input, 100, stdin) == NULL){
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

PrepareResult prepare_statement(char *input, Statement *statement){
    if (strcmp(input, "select") == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input, "insert ", 7) == 0){
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(input, "insert %u %31s %254s", &(statement->row_to_insert.id),
        statement->row_to_insert.username, statement->row_to_insert.email);
        if (args_assigned < 3){
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

Table* new_table(void){
    Table *table = malloc(sizeof(Table));
    if (table == NULL) {
        fprintf(stderr, "Error: could not allocate memory for new table.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < TABLE_MAX_ROWS; i++){
        table->row[i] = NULL;
    }
    table->num_rows = 0;
    return table;
}

void free_table(Table *table){
    if (table == NULL){
        return;
    }
    for (int i = 0; i < table->num_rows; i++){
        free(table->row[i]);
    }
    free(table);
}

ExecuteResult insert_row(Statement *statement, Table *table){
    if (table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }
    Row *memory_block = malloc(sizeof(Row));
    if (memory_block == NULL) {
        fprintf(stderr, "Error: could not allocate memory for row.\n");
        exit(EXIT_FAILURE);
    }
    *memory_block = statement->row_to_insert;

    table->row[table->num_rows] = memory_block;
    table->num_rows ++;
    return EXECUTE_SUCCESS;
}

ExecuteResult select_row(Table *table){
    if (table->num_rows == 0){
        return EXECUTE_TABLE_EMPTY;
    }
    for (int i = 0; i < table->num_rows; i++){
        Row *row = table->row[i];
        printf("('%u', '%s', '%s')\n", row->id, row->username, row->email);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table){
    switch(statement->type){
        case STATEMENT_INSERT:
            return insert_row(statement, table);
        case STATEMENT_SELECT:
            return select_row(table);
    }
    return EXECUTE_SUCCESS;
}


// TODO 
// on a une table avec une collone voir ce qui est le plus simple entre 1 table 1 page et 1 collone ou 1 table et plusieurs colonne
