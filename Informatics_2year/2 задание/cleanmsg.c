#include <stdlib.h>                 
#include <sys/ipc.h>               
#include <sys/msg.h>                
#include <stdio.h>                   
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define TABLE_LIMIT 6
#define FAIL(a) {\
printf (a);\
exit(-1);\
}

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

struct mymsg {
    long mtype;
    int info;
};

int main () {
    key_t key; //key of shared memory
    pid_t chpid;
    int msgid;
    struct mymsg mybuf;
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
    if ((msgid = msgget(key, IPC_CREAT|0666)) < 0) {
        printf("Can't create message queue\n");
        exit(-1);
    }

    //here we create two processes:
    chpid = fork();
    if (chpid < 0) {
        printf ("Couldn't fork\n");
        exit(-1);
    } else if (chpid == 0) {
        temp3 = 0;
        for (i = 0; i < dirtys_num; i++) {
            while((*(pl_dirty+i))->num > 0) {
                if (temp3 < TABLE_LIMIT) {
                    mybuf.mtype = 1;
                    mybuf.info = find_id(pl_wash, (*(pl_dirty+i))->name, kinds_num); 
                    sleep(((*pl_wash+i))->num);
                    printf("%s washed, id=%d\n", (*(pl_dirty+i))->name, (*(pl_dirty+i))->id);
                    if (msgsnd(msgid, (struct msgbuf*)&mybuf, sizeof(int), 0) < 0) {
                        msgctl(msgid, IPC_RMID, NULL);
                        FAIL("msgsnd\n")
                    }
                    temp3++;
                    (*(pl_dirty+i))->num--;
                }
                if (msgrcv(msgid, (struct msgbuf*)&mybuf, sizeof(int), 2, 0) < 0) {
                    msgctl(msgid, IPC_RMID, NULL);
                    FAIL("msgsnd\n")
                }
                temp3--;
            }
        }
        printf("Everything is washed\n");
        mybuf.mtype = 1;
        mybuf.info = -1;
        if (msgsnd(msgid, (struct msgbuf*)&mybuf, sizeof(int), 0) < 0) {
            msgctl(msgid, IPC_RMID, NULL);
            FAIL("msgsnd\n");
        }
        wait(&i);
        return 0;
    } else {
        while(1) {
            if (msgrcv(msgid, (struct msgbuf*)&mybuf, sizeof(int), 1, 0) < 0){
                msgctl(msgid, IPC_RMID, NULL);
                FAIL("msgrcv, child\n")
            }
            if (mybuf.info == -1) {
                break;
            } else {
                if ((temp = find_num(pl_wipe, mybuf.info, kinds_num_2)) < 0) FAIL("ID NOT FOUND")
                sleep(temp);
                printf ("Item with id=%d wiped\n", mybuf.info);
                mybuf.mtype = 2;
                mybuf.info = 0;
                if (msgsnd(msgid, (struct msgbuf*)&mybuf, sizeof(int), 0) < 0) {
                    msgctl(msgid, IPC_RMID, NULL);
                    FAIL("msgsnd, child\n")
                }
            }
        }
        printf ("Wiping complete\n");
        return 0;
    }           
    return 0;
}
