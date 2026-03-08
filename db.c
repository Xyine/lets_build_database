# include <stdlib.h>
# include <string.h>
# include <stdio.h>


typedef enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum PrepareResult {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum StatementType {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_UNRECOGNIZED
} StatementType;

typedef struct Statement {
    enum StatementType type;
} Statement;

void read_input(char *input);

MetaCommandResult do_meta_command(char *input);

PrepareResult prepare_statement(char *input, Statement *statement);

void execute_statement(Statement *statement);

int main(void){
    char input[100];
    while(1){
        printf("db >");
        fflush(stdout);

        read_input(input);

        if (input[0] == '.'){
            switch (do_meta_command(input)) {
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
        }

        execute_statement(&statement);
        printf("Executed.\n");
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

MetaCommandResult do_meta_command(char *input){
    if (strcmp(input, ".exit") == 0){
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
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement *statement){
    switch(statement->type){
        case STATEMENT_SELECT:
            printf("here we do the select\n");
            break;
        case STATEMENT_INSERT:
            printf("here we do the insert\n");
            break;
    }
}
