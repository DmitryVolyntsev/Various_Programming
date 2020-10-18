#include "/home/kirilya/common.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	int r;

	if (argc != 2) {							//Проверка корректности аргументов
		die_ret("This program expects one argument.");
	}

	if (strempty(argv[1])) {						//Проверка корректности N
		die_ret("Empty number of tasks passed.");
	}

	errno = 0;
	char *endptr = NULL;
	unsigned long tasks = strtoul(argv[1], &endptr, 10);			//Распознаем N и проверим корректность
	if (!strempty(endptr) || errno != 0) {
		die_ret("Invalid number of tasks \"%s\": %m", argv[1]);
	}
	if (tasks > USHRT_MAX) {
		die_ret("Too high number of tasks \"%s\": %m.", argv[1]);
	}

	

	int sem = semget(IPC_PRIVATE, 1, 0600);					//Инициализируем семафор выделив новый сегмент разделяемой памяти
	if (sem < 0) {
		die_ret("Failed to semget() the control semaphore: %m");	
	}
	
	r = semctl(sem, 0, SETVAL, sem_arg_val((int)tasks));			//Передадим семафору N
	if (r < 0) {
		die_ret("Failed to call semctl(SETVAL) to initialize the control semaphore %d: %m", sem);
	}

	signal(SIGCHLD, SIG_IGN);

	
	FILE *in_file = stdin;							
										//Построчно считаем UNIX-команду
	for (;;) {
		_cleanup_free_ char *line = NULL;
		r = fscanf(in_file, " %m[^\n]", &line);
		if (r < 1) {				
			if (feof(in_file)) {					//Проверим конец файла
				break;
			} else if (ferror(in_file)) {
				die_ret("Failed to read line from input file: %m");
			} else {
				die_ret("Failed to parse input file.");
			}
		}


		_cleanup_free_ char **tokens = malloc(sizeof(char*));		//Токенизируем(распознаем) введеную строку
		size_t tokens_alloc = 1, tokens_nr = 0;
		_cleanup_free_ char *strtok_line = strdup(line);
		char *strtok_saveptr = NULL;

		assert(tokens);

		for (;;) {
			char *next_token = strtok_r(strtok_line, " ", &strtok_saveptr);
			strtok_line = NULL;

			if (tokens_nr >= tokens_alloc) {
				tokens_alloc *= 2;
				tokens = realloc(tokens, sizeof(char*) * tokens_alloc);
				assert(tokens);
			}

			tokens[tokens_nr++] = next_token;
			if (next_token == NULL) {
				break;
			}
		}

		if (tokens_nr == 1) {
			log("Invalid input line \"%s\".", line);
		}

		

		int child_pid = fork();						//Создадим процесс, проверим количество процессов, Р управляющий семафор
		if (child_pid == 0) {
			r = semop_many(sem, 1, semop_entry(0, -1, SEM_UNDO|IPC_NOWAIT));	
			if (r < 0) {
				if (errno == EAGAIN) {
					die_ret("Process limit exceeded, will not execute \"%s\".", line);
				} else {
					die_ret("Failed to P the control semaphore %d: %m", sem);
				}
			}

			r = execvp(tokens[0], tokens);				//Запустим команду
			die_ret("Failed to execvp() the file \"%s\": %m", tokens[0]);
		} else if (child_pid < 0) {
			die_ret("Failed to fork() off a child: %m");
		}
	}

	

	r = wait(NULL);
	if (r >= 0 || errno != ECHILD) {					//Дождемся выполнения команды
		log("Unexpected wait() result %d: %m", r);
	}

	return 0;
}
