#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void sed(char *find, char *replace, char *input) {
	int i = 0;
	int find_len = strlen(find);

	while (input[i]) {
		if (strstr(&input[i], find) == &input[i]) {
			printf("%s", replace);
			i += find_len;
		}
		else {
			printf("%c", input[i]);
			i++;
		}
	}
}

int main(int argc, char *argv[]) {
	int count = 3;

	if (argc < 3) {
		printf("my-sed: find_term replace_term [file ...]\n");
		exit(1);
	}
	char *input;
	size_t len = 0;

	if (argc == 3) {
		while (getline(&input, &len, stdin) != -1) {
			sed(argv[1], argv[2], input);
		}
		free(input);
	}
	else {
		while (argc != count) {
			FILE *fp = fopen((argv[count]), "r");
			count++;
			if (fp == NULL) {
				printf("my-sed: cannot open file\n");
				exit(1);
			}
			while (getline(&input, &len, fp) != -1) {
				sed(argv[1], argv[2], input);
			}
			free(input);
			fclose(fp);
		}
	}
	return 0;
}

