#include <stdlib.h>                  //TODO: Вставить бит конца работы, чтобы после этого ребенок
#include <sys/ipc.h>                 //шел до концаи останавливался
#include <sys/shm.h>                 //
#include <stdio.h>                   //
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define TABLE_LIMIT 6

struct Plate {
    int num;
    char* name;
    int id;
};

typedef struct Plate plate;

plate* parse_and_construct(char* name, int num, int cur_id) { //parses string from file and creates structure, that contains type of dish and associated number
    if (!name) {
        printf("name = NULL\n");
        return NULL;
    }
    plate* res = malloc(sizeof(plate));
    res->num = num;
    res->id = cur_id;
    res->name = malloc(sizeof(char)*(strlen(name)+1));
    strcpy(res->name, name);
    return res;
}

int find_id(plate** plates, char* name, int len) { //finds id of a plate by its name
    int k = 0;
    for (k = 0; k < len; k++) {
        if (strcmp((*(plates+k))->name, name) == 0) {
            return (*(plates+k))->id;
        }
    }
    printf("Nothing found\n");
    return -1;
}

int find_num(plate** plates, int id, int len) {//finds certain number of a plate by its id
    int k = 0;
    for (k = 0; k < len; k++) {
        if ((*(plates+k))->id == id) {
            return (*(plates+k))->num;
        }
    }
    printf ("Nothing found\n");
    return -1;
}

int main () {
    key_t key; //key of shared memory
    pid_t chpid;
    int shmid;
    int* mem = malloc(sizeof(int)*(TABLE_LIMIT+4));
    int temp;
    int temp2 = 0;
    int temp3;
    int i = 0; //counter
    int kinds_num = 0; //number of different types of dishes = len(info)
    int dirtys_num = 0; //number of records in dirty.txt = len(plates)
    int kinds_num_2 = 0;
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
        *(pl_wash+i) = parse_and_construct(buf2, temp2, kinds_num+1);
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
        if ((temp = find_id(pl_wash, buf2, kinds_num)) == -1) {
            printf ("Problem with id\n");
            exit(-1);
        }
        *(pl_dirty+i) = parse_and_construct(buf2, temp2, temp);
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
        if ((temp = find_id(pl_wash, buf2, kinds_num)) == -1) {
            printf ("Problem with id\n");
            exit(-1);
        }
        *(pl_wipe+i) = parse_and_construct(buf2, temp2, temp);
        (kinds_num_2)++;
        i++;
    }
    printf("Scanning complete\n");
    fclose(washing_time);
    fclose(dirty_time);
    fclose(wiping_time);
    
    if (kinds_num != kinds_num_2) {
        printf("Not enough info\n");
    }
    //creating and allocating shared memory:
    if ((key = ftok("./dirty.txt", 0)) < 0) {
        printf("Can't generate key\n");
        exit(-1);
    }
    if ((shmid = shmget(key, sizeof(int)*(TABLE_LIMIT+4), IPC_CREAT|0666)) < 0) {
        printf("Can't create shared memory\n");
        exit(-1);
    }
    if ((mem = shmat(shmid, NULL, 0)) < 0) {
        printf ("Can't attach shred memory\n");
        exit(-1);
    }
    /* 
    mem[0,1] and mem[2] is for Peterson's synchronising algorithm;
    mem[3] means number of plates on a table;
    remaining place is ids of plates, 0 if nothing and -1 for END.*/
    for (i = 0; i < TABLE_LIMIT+4; i++) mem[i] = 0;
    //here we create two processes:
    chpid = fork();
    if (chpid < 0) {
        printf ("Couldn't fork\n");
        exit(-1);
    } else if (chpid == 0) {
        temp3 = 0;
        for (i = 0; i < dirtys_num; i++) {
            while ((*(pl_dirty+i))->num > 0) {
                /*critical section*/
                mem[0] = 1;
                mem[2] = 1;
                while (mem[1] && mem[2] == 1);
                if (mem[3] < TABLE_LIMIT) { //if there's free spaceon the table
                    mem[mem[3]+4] = (*(pl_dirty+i))->id;
                    mem[3]++;
                    printf("Parent, %d plates on a table, stored in %d\n", mem[3], mem[3]+3);
                    (*(pl_dirty+i))->num--;
                    mem[0] = 0;
                /*end of critical section*/
                    if ((temp3 = find_num(pl_wash, (*(pl_dirty+i))->id, kinds_num)) < 0) {
                        printf ("Couldn't find by id\n");
                        exit(-1);
                    }
                    sleep(temp3);
                    printf ("%s, id=%d washed\n", (*(pl_dirty+i))->name, (*(pl_dirty+i))->id);
                }
                mem[0] = 0;
            }
        }
        printf("Everything is washed\n");
        /*critical section*/
        while (1) {
            mem[0] = 1;
            mem[2] = 1;
            while (mem[1] && mem[2] == 1);
            if (mem[3] < TABLE_LIMIT) {
                mem[mem[3]+4] = -1;//END
                mem[3]++;
                printf("END written, cell %d\n", mem[3]+4);
                mem[0] = 0;
                break;
            }
            mem[0] = 0;
        }
        /*end of critical section*/
        wait(&i);
    } else {
        temp2 = 0;
        int cond = 1;
        while(1) {
                /*critical section*/
            mem[1]=1;
            mem[2]=0;
            while (mem[0] && mem[2] == 0);
            /*for (temp2 = 0; temp2 < TABLE_LIMIT+4; temp2++) printf("%d, ", mem[temp2]);
            printf("\n");*/
            if (mem[3] == 0) {
                if (cond == 0) {
                    printf ("Everything is wiped\n");
                    break;
                }
                mem[1] = 0;
                continue; //if nothing on the table
            } else {
                temp = mem[3]+3;
                printf ("Cell #%d, child\n", temp);
                if (mem[temp] == -1) {
                    cond = 0;
                    mem[temp] = 0;
                    mem[3]--;
                    mem[1] = 0;
                    continue;
                } else if (mem[temp] == 0){
                    printf ("Error, mem[%d] is empty\n", temp);
                    mem[1] = 0;
                    exit(-1);
                } else {
                    temp2 = mem[temp];
                    mem[temp] = 0;
                    mem[3]--;
                    mem[1] = 0;
                }
                /*end of critical section*/
                if ((temp == find_num(pl_wipe, temp2, kinds_num_2)) < 0) {
                    printf("Id not found\n");
                    exit(-1);
                } else {
                    sleep(temp);
                    printf("id %d wiped\n", temp2);
                }
            }
        }
    }           
    if (shmdt(mem) < 0) {
        printf ("Couldn't detach memory\n");
        exit(-1);
    }
    return 0;
}
