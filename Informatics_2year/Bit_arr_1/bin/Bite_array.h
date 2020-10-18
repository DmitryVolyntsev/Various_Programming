#ifndef BITARR
#define BITARR

typedef struct {
    int* arr; // битовый массив
    int num; // величина массива
} bitmap;

//int check (bitmap* bitarr, int n);
//int findplace (int* plc, int n);
bitmap* inizialize(int n); // Инициализируем битовый массив, передаем количество бит
int set(bitmap* bitarr, int n, int bit); // в n-тый бит устанавливаем значение bit
int get(bitmap* bitarr, int n, int* value); // получаем значение n-ого бита, оно передается в value
int clear(bitmap* bitarr); // Очищаем структуру
#endif
