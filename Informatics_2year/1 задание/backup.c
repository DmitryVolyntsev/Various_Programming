#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#define crsh(fmt, ...) do { fprintf (stderr, fmt "\n", ## __VA_ARGS__); exit(EXIT_FAILURE); } while (0)		//Выход в случае ошибки
#define chk(expr, fmt, ...) do { if (!(expr)) crsh(fmt, ## __VA_ARGS__); } while (0)		//Проверка корректности с выходом в случае ошибки	



struct backup_parameters						//Структура параметров бэкапа
{
	const char **archiver_argv; 					//шаблон параметров архиватора без путей
	int archiver_argc; 						//количество параметров в шаблоне без пути
	const char *archiver_out_template; 				//шаблон имени архивированного файла		
	
};

struct backup_state							//Структура содержащая подаваемые директории
{
	int in_fd;
	int out_fd;
};

const char *dbg_st_mode_to_string(int st_mode) 				//Расшифровка типа файла, который не удалось обработать
{
	if(S_ISBLK(st_mode)) return "block special";
	if(S_ISCHR(st_mode)) return "character special";
	if(S_ISFIFO(st_mode)) return "FIFO special";
	if(S_ISREG(st_mode)) return "regular file";
	if(S_ISDIR(st_mode)) return "directory";
	if(S_ISLNK(st_mode)) return "symbolic link (broken)";	
	if(S_ISSOCK(st_mode)) return "socket";
	return "<unknown st_mode value>";
}




int backup_file(const struct backup_parameters *parameters, const struct backup_state *parent_state,		//Архивация файла
                const char *in_file, const char *out_file)
{
	int r;

	int in_fd = openat(parent_state->in_fd, in_file, O_RDONLY|O_CLOEXEC);					//Открытие обрабатываемой директории
	chk(in_fd > 0, "Could not openat() input file \"%s\": %m", in_file);

	int out_fd = openat(parent_state->out_fd, out_file, O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC, 0666);		//Открытие выходной директории
	chk(out_fd > 0, "Could not openat() output file \"%s\": %m", out_file);


        char buffer[1024];
        ssize_t read_bytes, written_bytes;

        while((read_bytes = read(in_fd, buffer, 1024)) > 0) 							//Считывание файла в буфер
		{
                written_bytes = write(out_fd, buffer, read_bytes);						//Запись файла из буфера
		if (written_bytes < 0) 
			{
	               	fprintf(stderr, "Could not write() %zd bytes to the destination file descriptor: %m", read_bytes); 	//Провека успешности записи
                        r = -1;
	                }
	        }
	if (read_bytes < 0) 
		{
		fprintf(stderr, "Could not read() %d bytes to the destination file descriptor: %m", 1024);			//Проверка успешности считывания
		r = -1;
        	}
	r = 0;
	close(in_fd);												//Закрытие файлов
	close(out_fd);

	if (r < 0)												
		{
		return -1;
		}

	const char **archiver_argv = parameters->archiver_argv;							//Передача параметров ариватора
	int archiver_argc = parameters->archiver_argc;

	archiver_argv[archiver_argc++] = out_file;

	int child_pid = fork();											//Создание детского процесса
	if (child_pid == 0) 
		{
		r = fchdir(parent_state->out_fd);								//Переход в выходной каталог
		chk(r == 0, "Could not chdir() to the directory of the destination file \"%s\": %m", out_file);
		execvp(archiver_argv[0], (char**) archiver_argv);						//Запуск архиватора
		crsh("Could not execvp() archiver '%s': %m", archiver_argv[0]);
		}
	chk(child_pid > 0, "Could not fork() off the archiver process: %m");					//Проверка успешности запуска детского процесса
	int child_exitcode;
	r = waitpid(child_pid, &child_exitcode, 0);								//Ожидание изменения состояния ребенка
	chk(r > 0, "Could not waitpid() for child %d: %m", child_pid);
	chk(r == child_pid, "Unexpected return from waitpid() for child %d: %d", child_pid, r);

	if (WIFEXITED(child_exitcode) && WEXITSTATUS(child_exitcode) != 0)					//Проверка статуса выхода архиватора 
		{
		fprintf(stderr, "Archiver for destination file \"%s\" exited with non-zero value %d", out_file, WEXITSTATUS(child_exitcode));
		return -1;
		}

	if (WIFSIGNALED(child_exitcode)) 									//Проверка если архиватор был остановлен сигналом
		{
		fprintf(stderr, "Archiver for destination file \"%s\" was terminated with signal %d", out_file, WTERMSIG(child_exitcode));
		return -1;
		}

	return 0;
}

int backup_dir(const struct backup_parameters *parameters, const struct backup_state *parent_state,		//Обработка директории
               const char *in_subdir, const char *out_subdir)
{
	int r, errors = 0;

	r = mkdirat(parent_state->out_fd, out_subdir, 0777); 							//Создание итоговой директории
	chk(r == 0 || errno == EEXIST, "Could not mkdirat() output subdirectory \"%s\": %m", out_subdir);	//Проверка успешности создания директории

	struct backup_state state = 										//Создание структуры, содержащей пути
	{
		.in_fd = -1,
		.out_fd = -1
	};

	state.in_fd = openat(parent_state->in_fd, in_subdir, O_RDONLY|O_DIRECTORY|O_CLOEXEC);			//Открытие обрабатываемой директории
	chk(state.in_fd > 0, "Could not openat() input subdirectory \"%s\": %m", in_subdir);			//Проверка успешности открытия директории

	state.out_fd = openat(parent_state->out_fd, out_subdir, O_RDONLY|O_DIRECTORY|O_CLOEXEC);		//Открытие выходной директории
	chk(state.out_fd > 0, "Could not openat() output subdirectory \"%s\": %m", out_subdir);			//Проверка успешности открытия директории

	DIR *in_dir = fdopendir(state.in_fd);									//Открытие потока ввода
	chk(in_dir, "Could not fdopendir() input subdirectory \"%s\": %m", in_subdir);				//Проверка успешности открытия потока


	struct dirent *dirent = alloca(sizeof(struct dirent) + NAME_MAX + 1);					//Выделение пямяти для записи
	struct dirent *readdir_result = NULL;

	while(1) 
	{
		r = readdir_r(in_dir, dirent, &readdir_result);						//Считывание указателя следующей директории в потоке каталога
		chk(r == 0, "Failed to readdir_r() input subdirectory \"%s\": %m", in_subdir);		//Проверка успешности считывания указателя директории
		if (!readdir_result) 									//Проверка корректности readdir_r (0 когда без ошибок)
		{
			break;
		}

		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..")) 		//Директория задана корректно (“.”-текущий каталог, “..”-родительский)
		{
			continue;
		}

		struct stat in_st;
		r = fstatat(state.in_fd, dirent->d_name, &in_st, 0);				//Считывание информации о директории
		chk(r == 0, "Failed to fstatat() file \"%s\" in the input subdirectory \"%s\": %m", dirent->d_name, in_subdir);

		if (S_ISDIR(in_st.st_mode)) 							//Обработка каталогов
			{
			r = backup_dir(parameters, &state, dirent->d_name, dirent->d_name);
			if (r < 0) 
				{
				++errors;
				}
			}
		 else 
			if (S_ISREG(in_st.st_mode)) 						//Обработка файлов
				{
				char *out_file = alloca(NAME_MAX + 1);				//Выделение памяти под архивированный файл
				snprintf(out_file, NAME_MAX + 1, parameters->archiver_out_template, dirent->d_name); //Создание файла

				struct stat out_st;
				r = fstatat(state.out_fd, out_file, &out_st, 0);		//Считывание информации о директории
				chk(r == 0 || errno == ENOENT, "Could not fstatat() file \"%s\" in the output subdirectory \"%s\": %m", out_file, out_subdir);
				const struct timespec *a=&out_st.st_mtim, *b=&in_st.st_mtim;
					//Проверка необходимости архивации по времени изменения
				if (r != 0 || (a->tv_sec < b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec < b->tv_nsec))) 	
					{
					r = backup_file(parameters, &state, dirent->d_name, dirent->d_name); //Вызов функции архивирования
					if (r < 0) 
						{
						++errors;
						}
					}
				} 
			else 
				{
				fprintf(stderr, "Don't know how to handle %s \"%s\" in the input subdirectory \"%s\"", //Ошибка, если не файл или каталог
						dbg_st_mode_to_string(in_st.st_mode), dirent->d_name, in_subdir);
				++errors;
				}
	}

	closedir(in_dir);
	close(state.in_fd);
	close(state.out_fd);

	return errors ? -1 : 0;
}

int main(int argc, char **argv)
{
	int r;

	if (argc != 3) 							//Проверка корректности аргументов
	{
		crsh("This program expects two arguments.");
	}

	const char *in_dir = argv[1], *out_dir = argv[2];		//Считывание директорий

	const char *archiver_argv_template[] = 				//Создание шаблона аргументов запуска архиватора
	{	
		"gzip",
		"-f",
		"-9",
		NULL, 							//Позиции под пути
		NULL
	};

	struct backup_parameters parameters = 
	{
		.archiver_argv = archiver_argv_template,
		.archiver_argc = (sizeof(archiver_argv_template) / sizeof(*archiver_argv_template)) - 2,
		.archiver_out_template = "%s.gz"
	};

	struct backup_state state = 					//Структура путей, иницилизированная относительно текущей директории
		{
		.in_fd = AT_FDCWD,
		.out_fd = AT_FDCWD
		};

	r = backup_dir(&parameters, &state, in_dir, out_dir);		//Запуск сканирования директорий

	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}	

  


