# include <stdlib.h>
# include <string.h>
# include <stdio.h>

int main(void){
    char input[100];
    while(1){
        printf("db >");
        fflush(stdout);

        fgets(input, 100, stdin);
        int input_length = strlen(input);
        if (input[input_length -1] == '\n'){
            input[input_length - 1] = '\0';
        }
        if (strcmp(input, "exit") == 0){
            exit(0);
        }
    }

    return 0;
}
