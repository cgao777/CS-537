#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include <fcntl.h>
#include<sys/wait.h>
#include<syscall.h>
#include<math.h>

char exit_string[5] = "exit";
char cd_string[3] = "cd";
char history_string[8] = "history";
char path_string[5] = "path";
char error_message[30] = "An error has occurred\n";


char *sed(char *find, char *input) {
	int i = 0;
	int j = 0;
	int find_len = strlen(find);
	char *result = malloc(strlen(input) + 1);
	while (input[i]) {
		if (strstr(&input[i], find) == &input[i]) {
			i += find_len;
		}
		else {
			result[j] = input[i];
			i++;
			j++;
		}
	}
	result[j] = '\0';
	return result;
}

struct node {
	char* data;
	struct node *next;
};


int main(int argc, char *argv[]) {
	char **path_array = NULL;
	path_array = (char **)malloc(1);
	path_array[0] = (char *)malloc(6);
	strcpy(path_array[0], "/bin/");

	struct node *head = NULL;
	struct node *last = NULL;

	int history_cnt = 0;

	int path_num = 1;

	if (argc > 2) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	char *input;
	size_t len = 0;

	if (argc > 2) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	FILE *my_file;
	if (argc == 2) {
		my_file = fopen((argv[1]), "r");
		if (my_file == NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
	}

	while (1) {


		if (argc == 1) {
			printf("wish> ");
			fflush(stdout);
			if (getline(&input, &len, stdin) == -1) {
				exit(0);
			}
		}
		if (argc == 2) {
			if (getline(&input, &len, my_file) == -1) {
				fclose(my_file);
				exit(0);
			}
		}

		char *temp_array = malloc(strlen(input) + 1);
		strcpy(temp_array, input);


		if (head == NULL) {
			head = (struct node*)malloc(sizeof(struct node));
			head->data = temp_array;
			head->next = NULL;
			last = head;
			history_cnt++;
		}

		else {
			struct node *next_node = (struct node*)malloc(sizeof(struct node));
			last->next = next_node;
			next_node->data = temp_array;
			next_node->next = NULL;
			last = next_node;
			history_cnt++;
		}


		char *s = sed("\n", input);
		s = sed("\t", s);
		char p[sizeof(s)];
		strcpy(p, s);
		int pipe_cnt = 0;
		int redirection_cnt = 0;


		for (int i = 0; i < strlen(s); i++) {
			if (s[i] == '|') {
				pipe_cnt++;
			}
			if (s[i] == '>') {
				redirection_cnt++;
			}
		}

		if (pipe_cnt + redirection_cnt > 1) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			continue;
		}

		
		
		if (redirection_cnt == 1) {

			int cmd_count1 = 0;
			int cmd_count2 = 0;
			char *first_half = strtok(s, ">");
			if (first_half == NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			char first_cpy[strlen(first_half) + 1];
			strcpy(first_cpy, first_half);

			char *second_half = strtok(NULL, ">");
			if (second_half == NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			char second_cpy[strlen(second_half) + 1];
			strcpy(second_cpy, second_half);

			char *token = strtok(first_half, " ");
			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_count1++;
					token = strtok(NULL, " ");
				}
			}
			if (cmd_count1 == 0) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			char *cmd_argv1[cmd_count1];

			token = strtok(second_half, " ");
			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_count2++;
					token = strtok(NULL, " ");
				}
			}
			if (cmd_count2 != 1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			char *cmd_argv2[cmd_count2];


			cmd_count1 = 0;
			token = strtok(first_cpy, " ");


			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_argv1[cmd_count1] = token;
					token = strtok(NULL, " ");
					cmd_count1++;
				}
			}

			cmd_count2 = 0;
			token = strtok(second_cpy, " ");


			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_argv2[cmd_count2] = token;
					token = strtok(NULL, " ");
					cmd_count2++;
				}
			}



			int fp = open(cmd_argv2[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			if (fp == -1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			int rc = fork();
			if (rc < 0) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			if (rc == 0) {



				dup2(fp, STDOUT_FILENO);
				dup2(fp, STDERR_FILENO);


				if (path_num == 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					return 0;
				}


				for (int i = 0; i < path_num; i++)
				{

					char *my_argv[cmd_count1 + 1];
					char *x = NULL;

					for (int i = 0; i < cmd_count1; i++) {
						x = (char *)malloc(strlen(cmd_argv1[i]) + 1);
						strcpy(x, cmd_argv1[i]);
						my_argv[i] = x;
					}

					my_argv[cmd_count1] = NULL;

					char my_path[strlen(path_array[i]) + strlen(my_argv[0]) + 1];
					strcpy(my_path, path_array[i]);
					strcat(my_path, my_argv[0]);
					my_argv[0] = my_path;

					char ls_path[3] = "ls";
					if (strcmp(my_argv[0], "/bin/ls") == 0) {
						my_argv[0] = ls_path;
					}

					execv(my_path, my_argv);
				}
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(0);
			}
			else {
				wait(NULL);
				close(fp);
			}
			continue;
		}




		//pipe///////
		if (pipe_cnt == 1) {

			int cmd_count1 = 0;
			int cmd_count2 = 0;
			char *first_half = strtok(s, "|");
			if (first_half == NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			char first_cpy[strlen(first_half) + 1];
			strcpy(first_cpy, first_half);

			char *second_half = strtok(NULL, "|");
			if (second_half == NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			char second_cpy[strlen(second_half) + 1];
			strcpy(second_cpy, second_half);

			char *token = strtok(first_half, " ");
			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_count1++;
					token = strtok(NULL, " ");
				}
			}
			if (cmd_count1 < 1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			char *cmd_argv1[cmd_count1];

			token = strtok(second_half, " ");
			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_count2++;
					token = strtok(NULL, " ");
				}
			}
			if (cmd_count2 < 1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			char *cmd_argv2[cmd_count2];


			cmd_count1 = 0;
			token = strtok(first_cpy, " ");


			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_argv1[cmd_count1] = token;
					token = strtok(NULL, " ");
					cmd_count1++;
				}
			}

			cmd_count2 = 0;
			token = strtok(second_cpy, " ");


			while (token != NULL) {
				if (strcmp(token, "") == 0) {
					token = strtok(NULL, " ");
				}
				else {
					cmd_argv2[cmd_count2] = token;
					token = strtok(NULL, " ");
					cmd_count2++;
				}
			}


			int fd[2];
			if (pipe(fd) == -1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}

			int rc = fork();
			if (rc == -1) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			if (rc == 0) {


				close(fd[0]);
				dup2(fd[1], STDOUT_FILENO);
				dup2(fd[1], STDERR_FILENO);


				if (path_num == 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					return 0;
				}


				for (int i = 0; i < path_num; i++)
				{

					char *my_argv[cmd_count1 + 1];
					char *x = NULL;

					for (int i = 0; i < cmd_count1; i++) {
						x = (char *)malloc(strlen(cmd_argv1[i]) + 1);
						strcpy(x, cmd_argv1[i]);
						my_argv[i] = x;
					}


					my_argv[cmd_count1] = NULL;


					char my_path[strlen(path_array[i]) + strlen(my_argv[0]) + 1];
					strcpy(my_path, path_array[i]);
					strcat(my_path, my_argv[0]);
					my_argv[0] = my_path;

					char ls_path[3] = "ls";
					if (strcmp(my_argv[0], "/bin/ls") == 0) {
						my_argv[0] = ls_path;
					}

					execv(my_path, my_argv);

				}
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(0);
			}
			else {
				close(fd[1]);
				waitpid(rc, NULL, 0);

				int rc2 = fork();
				if (rc2 == -1) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					continue;
				}
				if (rc2 == 0) {
						
					close(fd[1]);
					dup2(fd[0], STDIN_FILENO);
					
//					close(fd[0]);	

//					char buffer[1024];
//					int n;
//
//					if ((n = read(fd[0], buffer, sizeof(buffer) - 1) > 0)) {
//					}
//					else {
//						write(STDERR_FILENO, error_message, strlen(error_message));
//						exit(0);
//					}
//					buffer[strlen(buffer)] = '\0';
//					printf("%s\n", buffer);


					if (path_num == 0) {
						write(STDERR_FILENO, error_message, strlen(error_message));
						return 0;
					}


					for (int i = 0; i < path_num; i++)
					{

						char *my_argv[cmd_count2 + 1];
						char *x = NULL;

						for (int i = 0; i < cmd_count2; i++) {
							x = (char *)malloc(strlen(cmd_argv2[i]) + 1);
							strcpy(x, cmd_argv2[i]);
							my_argv[i] = x;
						}

						my_argv[cmd_count2] = NULL;

						char my_path[strlen(path_array[i]) + strlen(my_argv[0]) + 1];
						strcpy(my_path, path_array[i]);
						strcat(my_path, my_argv[0]);
						my_argv[0] = my_path;

						char ls_path[3] = "ls";
						if (strcmp(my_argv[0], "/bin/ls") == 0) {
							my_argv[0] = ls_path;
						}

						execv(my_path, my_argv);

					}
					write(STDERR_FILENO, error_message, strlen(error_message));
					exit(0);
				}
				else {
					close(fd[0]);
					wait(NULL);
					continue;
				}
			}

		}

		int cmd_count = 0;
		char *token = strtok(s, " ");
		while (token != NULL) {
			if (strcmp(token, "") == 0) {
				token = strtok(NULL, " ");
			}
			else {
				cmd_count++;
				token = strtok(NULL, " ");
			}
		}


		char *cmd_argv[cmd_count];
		cmd_count = 0;
		token = strtok(p, " ");

		char *my_tmp;

		while (token != NULL) {
			if (strcmp(token, "") == 0) {
				token = strtok(NULL, " ");
			}
			else {
				my_tmp = (char *)malloc(strlen(token) + 1);
				strcpy(my_tmp, token);

				cmd_argv[cmd_count] = my_tmp;

				token = strtok(NULL, " ");
				cmd_count++;
			}
		}

		if (cmd_count == 1) {
			if (strcmp(cmd_argv[0], exit_string) == 0) {
				exit(0);
			}
		}
		if (cmd_count == 0) {
			continue;
		}

		//cd///////////////////////////////////////////////////////////
		if (strcmp(cmd_argv[0], cd_string) == 0) {
			if (cmd_count != 2) {
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			else {
				if (chdir(cmd_argv[1]) == -1) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}
			continue;
		}

		//history/////////////////////////
		if (strcmp(cmd_argv[0], history_string) == 0) {
			if (cmd_count > 2) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			if (cmd_count == 1) {
				struct node *history_ptr = head;
				for (int i = 0; i < history_cnt; i++)
				{
					printf("%s", history_ptr->data);
					fflush(stdout);
					history_ptr = history_ptr->next;
				}
				continue;
			}
			else {
				double history_num = atof(cmd_argv[1]);
				if (history_num == 0 && cmd_argv[1][0] != '0') {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
				else {
					int new_num = (int)ceil(history_num);
					if (new_num <= 0) {
						continue;
					}
					if (new_num > history_cnt) {
						new_num = history_cnt;
					}
					struct node *history_ptr = head;
					for (int i = 0; i < history_cnt - new_num; i++)
					{
						history_ptr = history_ptr->next;
					}
					for (int i = 0; i < new_num; i++)
					{
						printf("%s", history_ptr->data);
						fflush(stdout);
						history_ptr = history_ptr->next;
					}
				}
				continue;
			}

		}

		//path///////////////////////////////////////////
		if (strcmp(cmd_argv[0], path_string) == 0)
		{
			path_num = cmd_count - 1;
			char **new_arr = NULL;
			char *p;
			new_arr = (char **)malloc(cmd_count - 1);
			for (int i = 1; i < cmd_count; i++)
			{
				if (cmd_argv[i][strlen(cmd_argv[i]) - 1] != '/')
				{
					p = malloc(strlen(cmd_argv[i]) + 2);
					strcpy(p, cmd_argv[i]);
					strcat(p, "/");
					new_arr[i - 1] = p;
				}
				else {
					new_arr[i - 1] = cmd_argv[i];
				}
			}

			path_array = new_arr;

			continue;
		}


		int rc = fork();
		if (rc == -1) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			continue;
		}
		if (rc == 0) {

			if (path_num == 0) {
				write(STDERR_FILENO, error_message, strlen(error_message));
				return 0;
			}


			for (int i = 0; i < path_num; i++)
			{

				char *my_argv[cmd_count + 1];
				char *x = NULL;

				for (int i = 0; i < cmd_count; i++) {
					x = (char *)malloc(strlen(cmd_argv[i]) + 1);
					strcpy(x, cmd_argv[i]);
					my_argv[i] = x;
				}


				my_argv[cmd_count] = NULL;

				char my_path[strlen(path_array[i]) + strlen(my_argv[0]) + 1];

				strcpy(my_path, path_array[i]);
				strcat(my_path, my_argv[0]);
				my_argv[0] = my_path;

				char ls_path[3] = "ls";
				if (strcmp(my_argv[0], "/bin/ls") == 0) {
					my_argv[0] = ls_path;
				}

				execv(my_path, my_argv);

			}

			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(0);
		}
		else {
			wait(NULL);
		}
	}
	return 0;
}


