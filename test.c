#include<stdio.h>

int main(){
    int size;
    printf("size cheppu");
    scanf("%d", &size);
    int temp[size];
    for(int i=0; i<size; i++){
        scanf("%d", &temp[i]);
    }
    for(int i=0; i<size; i++){
        printf("%d ", temp[i]);
    }

}