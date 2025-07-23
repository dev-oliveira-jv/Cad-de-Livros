#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define TAM  100;
typedef struct Livro{
    char titulo[100];
    char autor[100];
    char isbn[20];
    int ano;
    struct Livro *prox;
} Livro;

