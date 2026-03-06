# include <stdio.h>

int main(void){
    char input[100];
    while(1){
        printf("db >");
        fflush(stdout);

        fgets(input, 100, stdin);
    }

    return 0;
}
