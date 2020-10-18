#include <stdlib.h>
#include <stdio.h>
#include "Bite_array.h"


int check (bitmap* bitarr, int n) {
    if (bitarr == NULL) exit(-1);
    int num = bitarr->num * 8;
    if (num < n) {
        printf("Out of array: change place or expand the array\n");
        exit(-1);
    }
    else if (n < 0) {
        printf("Negative value of place\n");
        exit(-1);
        }
    else return 0;
}

int findplace (int* plc, int n) {
    if (plc == NULL) return -1;
    plc[0] = n / 8; // байт
    plc[1] = n % 8; // бит в нем
    //printf("0:%d 1:%d\n", plc[0], plc[1]);
}

bitmap* inizialize(int n) {
    n = (n % 8 ? n/8+1 : n/8);
    bitmap* bitarr = (bitmap*)malloc(sizeof(bitmap));
    if (bitarr == NULL) exit(-1);
    bitarr->arr = (int*)calloc(n, sizeof(int)); // . or -> dnt rem
    bitarr->num = n;
    if (bitarr->arr == NULL) exit(-1);

    return bitarr;
}

int set(bitmap* bitarr, int n, int bit) {
    if (bitarr == NULL) return -1;
    check(bitarr, n);
    int plc[2];
    findplace(plc, n);
    if (bit > 0)
        bit = 1;
    else if (!bit)
        bit = 0;
    else return 1;
    if (bit)
        bitarr->arr[plc[0]] = bitarr->arr[plc[0]] | (01 << (7 - plc[1]));
    else
        bitarr->arr[plc[0]] = bitarr->arr[plc[0]] & ~(01 << (7 - plc[1]));
}

int get(bitmap* bitarr, int n, int* value) {
    if (bitarr == NULL || value == NULL) return -1;
    check(bitarr, n);
    int plc[2];
    findplace(plc, n);
    if (bitarr->arr[plc[0]] & (01 << (7 - plc[1])))
        *value = 1;
    else
        *value = 0;
}

int clear (bitmap* bitarr) {
    if (bitarr == NULL) return -1;
    free(bitarr->arr);
    free(bitarr);
}
