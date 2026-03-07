# include <stdlib.h>
# include <string.h>
# include <stdio.h>


enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
};

enum MetaCommandResult do_meta_command(char *input);


int main(void){
    char input[100];
    while(1){
        printf("db >");
        fflush(stdout);

        if (fgets(input, 100, stdin) == NULL){
            printf("Error reading input\n");
            exit(1);
        }
        
        int input_length = strlen(input);
        
        if (input[input_length -1] == '\n'){
            input[input_length - 1] = '\0';
        }

        if (input[0] == '.'){
            switch (do_meta_command(input)) {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unrecognized command '%s'\n", input);
                    continue;
            }
        } else {

        }
    }

    return 0;
}

enum MetaCommandResult do_meta_command(char *input){
    if (strcmp(input, ".exit") == 0){
        exit(0);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}
