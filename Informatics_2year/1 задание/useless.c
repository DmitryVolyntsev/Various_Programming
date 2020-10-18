#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define crsh(fmt, ...) do { fprintf (stderr, fmt "\n", ## __VA_ARGS__); exit(EXIT_FAILURE); } while (0)		//Выход в случае ошибки
#define chk(expr, fmt, ...) do { if (!(expr)) crsh(fmt, ## __VA_ARGS__); } while (0)		//Проверка корректности с выходом в случае ошибки		


int main(int argc, char **argv)
{
	int r;

	if (argc != 2) 							//Проверка корректности аргументов
		{
		crsh("This program expects one argument.");
		}


	const char *in_path = argv[1];					//Открытие файла
	FILE *in_file;
	if ((in_file = fopen(in_path, "r")) == NULL) 
		{ 
		fprintf(stderr, "ERROR: Can't read from file.\n");
	        return 0;
		}							
	signal(SIGCHLD, SIG_IGN);					//Родитель ожидает оканчания процесса-ребенка
	while(1) 							//Построчное считывание данных и порождение	
		{

		int in_delay;
		char *in_path = NULL;

		r = fscanf(in_file, "%d %m[^\n] ", &in_delay, &in_path);			//Считывание строки
		if (r == EOF) 
		
			{
			chk(!ferror(in_file), "Failed to read next input line: %m"); 		//Проверка возможности считать новую строку
			break;
			}
		chk(r == 2, "Failed to parse next input line: parsed %d entries of 2", r);	//Проверка корректности строки

		int child_pid = fork();								//Порождение детского процесса
		if (child_pid == 0) 
			{
			sleep(in_delay);							//Задержка
			execlp(in_path, in_path, NULL);						//Запуск программы 
			crsh("Failed to exec() the file \"%s\": %m", in_path);			//Вывод в случае ошибки программы
			} 
		else 
			if (child_pid < 0) 
				{
				crsh("Failed to fork() off a child: %m");			//Вывод в случае порождения ребенка
				}
		free(in_path);									//Очистка строки
		}
	//r = wait(NULL);
	assert(wait(NULL) < 0 && errno == ECHILD);
	return 0;


}
