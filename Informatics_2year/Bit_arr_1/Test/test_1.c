#include <Bite_array.h>
#include <stdio.h>

#define N 64

int main() {
    bitmap* bit_arr;
    bit_arr = inizialize(N);

    bitmap* bit_arr_2;
    bit_arr_2 = inizialize(N);
    int i;
    printf(" 1ая структура \n");
    for (i = 0; i < N; i++) {
    //printf ("1");
        if (i % 2)
            set(bit_arr, i, 1);
        else
            set(bit_arr, i, 0);
    }

    int value = 0;
    for (i = 0; i < N; i ++) {
        get(bit_arr, i, &value);
        printf("%0d", value);
    }
    printf("\n 2ая структура \n");

    for (i = 0; i < N; i++) {
        set(bit_arr_2, i, 1);
    }

    int value_2 = 0;
    for (i = 1; i < N; i++) {
        get(bit_arr_2, i, &value_2);
        printf("%d", value_2);
    }
    printf("\n чистим 1ую и 2ую структуры\n");
    clear (bit_arr);
    clear (bit_arr_2);

    return 1;
}
