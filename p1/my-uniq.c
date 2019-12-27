#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void uniq(char *input_prev, char *input_next){
    if (input_prev == NULL){
        printf("%s", input_next);
    }else if (strcmp(input_prev, input_next) != 0){
        printf("%s", input_next);
    }
}

int main(int argc, char *argv[]){
    int count = 1;

    char *input_prev = NULL;
    char *input_next = NULL;
    size_t len = 0;

    if (argc == 1){
        while (getline(&input_next, &len, stdin)!= -1){
	    uniq(input_prev, input_next);
	    free(input_prev);
	    input_prev = strdup(input_next);
	}
	free(input_prev);
	free(input_next);
    }
    else{
        while (argc != count){
            FILE *fp = fopen((argv[count]), "r");
	    count++;
            if (fp == NULL) {
                printf("my-uniq: cannot open file\n");
	        exit(1);
            }
    	    while (getline(&input_next, &len, fp) != -1){
                uniq(input_prev, input_next);
                free(input_prev);
                input_prev = strdup(input_next);
	    }
	    free(input_prev);
	    free(input_next);
            fclose(fp) ;
        }
    }
    return 0;
}

