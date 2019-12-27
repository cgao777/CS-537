#include<stdio.h>
#include<stdlib.h>
int main(int argc, char *argv[]){
    int count = 1;
    while(argc != count){


        FILE *fp = fopen((argv[count]), "r");
	count++;
        if (fp == NULL) {
            printf("my-cat: cannot open file\n");
	    exit(1);
        }
        char buffer[100];
        while (fgets(buffer, 100, fp) != NULL){
            printf("%s", buffer);
        }
        fclose(fp) ;
    }
    return 0;
}

