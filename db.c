# include <stdbool.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>


typedef enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum PrepareResult {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAXE_ERROR
} PrepareResult;

typedef enum StatementType {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_UNRECOGNIZED
} StatementType;

typedef enum Execute_result {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_TABLE_EMPTY
} Execute_result;

typedef struct Row {
    uint32_t id;
    char username[32];
    char email[255];
} Row;

typedef struct Statement {
    enum StatementType type;
    Row row_to_insert;
} Statement;

typedef struct Table {
    bool has_row;
    Row *row;
} Table;

Table* new_table();

void free_table(Table *table);

void read_input(char *input);

MetaCommandResult do_meta_command(char *input, Table *table);

PrepareResult prepare_statement(char *input, Statement *statement);

Execute_result execute_statement(Statement *statement, Table *table);


int main(void){
    Table *table = new_table();
    char input[100];
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
            case PREPARE_SYNTAXE_ERROR:
                printf("Syntax error in the insert statement, insert take 3 arguments int id, str username, str email '%s'\n", input);
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
        exit(1);
    }
    
    int input_length = strlen(input);
    
    if (input[input_length -1] == '\n'){
        input[input_length - 1] = '\0';
    }
}

MetaCommandResult do_meta_command(char *input, Table *table){
    if (strcmp(input, ".exit") == 0){
        free_table(table);
        exit(0);
    }  
    return META_COMMAND_UNRECOGNIZED_COMMAND;
}

PrepareResult prepare_statement(char *input, Statement *statement){
    if (strncmp(input, "select", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input, "insert", 6) == 0){
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(input, "insert %d %31s %254s", &(statement->row_to_insert.id),
        statement->row_to_insert.username, statement->row_to_insert.email);
        if (args_assigned < 3){
            return PREPARE_SYNTAXE_ERROR;
        }
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

Table* new_table(){
    Table *table = malloc(sizeof(Table));
    table->row = NULL;
    table->has_row = false;
    return table;
}

void free_table(Table *table){
    if (table == NULL){
        return;
    }
    free(table->row);
    free(table);
}

Execute_result insert_row(Statement *statement, Table *table){
    if (table->has_row){
        return EXECUTE_TABLE_FULL;
    }
    Row *memory_block = malloc(sizeof(Row));

    Row row = statement->row_to_insert;
    *memory_block = row;

    table->row = memory_block;
    table->has_row = true;
    return EXECUTE_SUCCESS;
}

Execute_result select_row(Table *table){
    if (!table->has_row || table->row == NULL){
        return EXECUTE_TABLE_EMPTY;
    }

    Row *row = table->row;

    printf("('%d', '%s', '%s')\n", row->id, row->username, row->email);
    return EXECUTE_SUCCESS;
}

Execute_result execute_statement(Statement *statement, Table *table){
    switch(statement->type){
        case STATEMENT_INSERT:
            return insert_row(statement, table);
        case STATEMENT_SELECT:
            return select_row(table);
    }
}
