#include <stdio.h>                   
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define TABLE_LIMIT 2
#define FAIL(a) {\
printf (a);\
exit(-1);\
}

struct Plate {
    int num;
    char* name;
};

typedef struct Plate plate;

plate* parse_and_construct(char* name, int num) { //parses string from file and creates structure, that contains type of dish and associated number
    if (!name) {
        printf("name = NULL\n");
        return NULL;
    }
    plate* res = malloc(sizeof(plate));
    res->num = num;
    res->name = malloc(sizeof(char)*(strlen(name)+1));
    strcpy(res->name, name);
    return res;
}

int find_num(plate** plates, char* name, int len) { //finds necassary value of a plate by its name
    int k = 0;
    for (k = 0; k < len; k++) {
        if (strcmp((*(plates+k))->name, name) == 0) {
            return (*(plates+k))->num;
        }
    }
    printf("Nothing found\n");
    return -1;
}

int callsem (int n, int semnum, int semid, struct sembuf* mybuf) { //calls necessary semaphore with given arguments
    if (!mybuf) return -1;
    mybuf->sem_op = n;
    mybuf->sem_num = semnum;
    return semop(semid, mybuf, 1);
}


int main () {
    int file_fd; //file descriptor of communication file
    pid_t chpid;
    int semid;
    key_t key;
    struct sembuf mybuf;
    mybuf.sem_flg = 0;
    int* mem = malloc(sizeof(int)*(TABLE_LIMIT+4));
    int temp;
    int temp2 = 0;
    int temp3;
    int i = 0; //counter
    int kinds_num = 0; //number of different types of dishes = len(info)
    int dirtys_num = 0; //number of records in dirty.txt = len(plates)
    int kinds_num_2 = 0;
    char* childbuf = malloc(sizeof(char)*200);
    char* buf = malloc(sizeof(char)*40);  //buffer
    char* buf2 = malloc(sizeof(char)*40); //buffer
    plate** pl_wash = malloc(sizeof(plate*)*100);
    plate** pl_dirty = malloc(sizeof(plate*)*100);
    plate** pl_wipe = malloc(sizeof(plate*)*100);
    

    FILE* washing_time = fopen("washing_time.txt", "r");
    while ((fgets(buf, 40, washing_time)) != NULL) { //filling array with information of washing nums of different kinds of plates
        if (sscanf(buf, "%s %d", buf2, &temp2) < 0) {
            printf ("Invalid sscanf\n");
            exit(-1);
        }
        *(pl_wash+i) = parse_and_construct(buf2, temp2);
        kinds_num++;
        i++;
    }
    printf ("Wash scanned\n");
    i = 0;
    
    FILE* dirty_time = fopen("dirty.txt", "r");
    while ((fgets(buf, 40, dirty_time)) != NULL) { //getting info about dirty plates
        //printf("%s read from dirty.txt\n", buf);
        if (sscanf(buf, "%s %d", buf2, &temp2) < 0) {
            printf ("Invalid sscanf\n");
            exit(-1);
        }
        *(pl_dirty+i) = parse_and_construct(buf2, temp2);
        dirtys_num++;
        i++;
    }
    printf("dirty scanned\n");
    i = 0;
    
    FILE* wiping_time = fopen("wiping_time.txt", "r"); //wiping info
    while ((fgets(buf, 40, wiping_time)) != NULL) {
        if (sscanf(buf, "%s %d", buf2, &temp2) < 0) {
            printf ("Invalid sscanf\n");
            exit(-1);
        }
        *(pl_wipe+i) = parse_and_construct(buf2, temp2);
        (kinds_num_2)++;
        i++;
    }
    printf("Scanning complete\n");
    fclose(washing_time);
    fclose(dirty_time);
    fclose(wiping_time);

    free(buf2);
    if (kinds_num != kinds_num_2) {
        printf("Not enough info\n");
    }
    
    if ((file_fd = open("table.dat", O_RDWR|O_CREAT|O_EXCL, 0666)) < 0) FAIL("communication file not created\n")
    if ((key = ftok("table.dat", 0)) < 0) FAIL("token not created\n")
    if ((semid = semget(key, 3, IPC_CREAT|0666)) < 0) FAIL("semid not found\n")//#1 for mutual exclusion, #0 and #2 for table limit control
    if (callsem(TABLE_LIMIT, 0, semid, &mybuf) < 0) FAIL("table semaphore initializing failed\n") 
    if (callsem(1, 1, semid, &mybuf) < 0) FAIL("sync semaphore initializing failed\n")
    //here we create two processes:
    chpid = fork();
    if (chpid < 0) {
        printf ("Couldn't fork\n");
        exit(-1);
    } else if (chpid == 0) {
        temp3 = 0;
        for (temp = 0; temp < dirtys_num; temp++) {
            while ((*(pl_dirty+temp))->num > 0) {
                if (callsem(-1, 0, semid, &mybuf) < 0) FAIL("semop\n")
                sleep(find_num(pl_wash, (*(pl_dirty+temp))->name, kinds_num));
                if (callsem(-1, 1, semid, &mybuf) < 0) FAIL("semop\n") //critical section
                (*(pl_dirty+temp))->num--;
                printf("%s washed\n", (*(pl_dirty+temp))->name);
                temp3 = strlen((*(pl_dirty+temp))->name);
                if (write(file_fd, (*(pl_dirty+temp))->name, temp3) < temp3) FAIL("write\n")
                if (write(file_fd, "\n", 1) < 1) FAIL("write\n")
                if (callsem(1, 1, semid, &mybuf) < 0) FAIL("semop\n") //end of critical section
                if (callsem(1, 2, semid, &mybuf) < 0) FAIL("semop\n")
            }
        }
        if (write(file_fd, "END\n", 4) < 4) FAIL("write\n")
        printf("Everything's washed\n");
    } else {
        char cond = 1;
        while(cond) {
            if (callsem(-1, 2, semid, &mybuf) < 0) FAIL("semop\n")
            if (callsem(-1, 1, semid, &mybuf) < 0) FAIL("semop\n")
            //critical section
            for (temp2 = 0; temp2 < 100; temp2++) *(childbuf+temp2)='\0';
            if ((temp2 = lseek(file_fd, 0, SEEK_CUR)) < 0) FAIL("lseek\n")
            if (lseek(file_fd, 0, SEEK_SET) < 0) FAIL("lseek\n")
            read(file_fd, childbuf, 100);
            if (lseek(file_fd, 0, SEEK_SET) < 0) FAIL("lseek\n")
            if (ftruncate(file_fd, 0) < 0) FAIL("trunk\n")
            if (callsem(1, 1, semid, &mybuf) < 0) FAIL("semop\n")
            //end of critical section
            if ((buf = strtok(childbuf, "\n")) != NULL) {
                if (strcmp(buf, "END") == 0) {
                    printf("END read\n");
                    break;
                }
                sleep(find_num(pl_wipe, buf, kinds_num_2));
                printf("%s wiped\n", buf);
                if (callsem(1, 0, semid, &mybuf) < 0) FAIL("semop\n")
            }
            while ((buf=strtok(NULL, "\n")) != NULL) {
                if (strcmp(buf, "END") == 0) {
                    printf("END read\n");
                    cond = 0;
                    break;
                }
                sleep(find_num(pl_wipe, buf, kinds_num_2));
                printf("%s wiped\n", buf);
                if (callsem(1, 0, semid, &mybuf) < 0) FAIL("semop\n")
                if (callsem(-1, 2, semid, &mybuf) < 0) FAIL("semop\n")
            }
            //printf("Iter\n");
        }
        printf("Everything is wiped\n");
        //child
    } 
    remove("table.dat");          
    return 0;
}
