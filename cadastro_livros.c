#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define TAM  100
typedef struct Livro{
    char titulo[TAM];
    char autor[TAM];
    char isbn[20];
    int ano;
    struct Livro *prox;
} Livro;

//adiciona o livro a lista 
void adiconarLivro(Livro **lista){
    Livro *novo = (Livro* )malloc(sizeof(Livro));
    if(!novo){
        printf("Erro ao alocar memória\n");
        return;
    }

    printf("Digite o livro: \n");
    fgets(novo->titulo, sizeof(novo->titulo), stdin);
    novo->titulo [strcspn(novo->titulo, "\n")] = 0;

    printf("Digite o Autor: \n");
    fgets(novo->autor, sizeof(novo->autor), stdin);
    novo->autor [strspn(novo->autor, "\n")] = 0;

    printf("Digite o ISBN: \n");
    fgets(novo->isbn, sizeof(novo->isbn), stdin);
    novo->isbn [strspn(novo->isbn, "\n")] = 0;

    printf("Digite o ano de publicação: \n");
    scanf("%d", &novo->ano);
    getchar(); //limpar o '\n' do buffer

    novo->prox = *lista;
    *lista = novo;
    printf("Livro adicionado com sucesso! \n");
};

// Listar todos os livros 
void listarLivros(Livro *lista){
    if(!lista){
        printf("Nenhum livro cadastrado. \n");
        return;
    }

    printf("\n---- Lista de Livros ----\n");
    while (lista){
        printf("Título : %s\n", lista->titulo);
        printf("Autor: %s\n", lista->autor);
        printf("ISBN: %s\n", lista->isbn);
        printf("Ano: %d\n", lista->ano);
        printf("------------------------\n");
        lista = lista->prox;
    }
};

// Remover o livro pelo ISBN
void removerLivro(Livro **lista, const char *isbn) {
    Livro *atual = *lista;
    Livro *anterior = NULL;

    while (atual) {
        if (strcmp(atual->isbn, isbn) == 0) {
            if (anterior == NULL) {
                *lista = atual->prox;
            } else {
                anterior->prox = atual->prox;
            }
            free(atual);
            printf("Livro removido com sucesso!\n");
            return;
        }
        anterior = atual;
        atual = atual->prox;
    }

    printf("Livro com ISBN %s não encontrado.\n", isbn);
}
Livro* removerLivroRec(Livro *lista, const char *isbn){
    if(lista == NULL){
        printf("Livro com ISBN %s não encontrado. \n", isbn);
        return NULL;
    }

    if(strcmp(lista->isbn, isbn) == 0){
        Livro *temp = lista->prox;
        free(lista);
        printf("Livro removido com sucesso! \n");
        return temp;
    }
    lista->prox = removerLivroRec(lista->prox, isbn);
    return lista;
}

Livro* adicionarLivroRec(Livro *lista){
    if(lista == NULL){
        Livro *novo = (Livro *)malloc(sizeof(Livro));
        if(!novo){
            printf("Erro ao alocar memoria!\n");
            return NULL;
        }
        printf("Digite o título: ");
        fgets(novo->titulo, sizeof(novo->titulo), stdin);
        novo->titulo[strcspn(novo->titulo, "\n")] = 0;
        
        printf("Digite o autor: ");
        fgets(novo->autor, sizeof(novo->autor), stdin);
        novo->autor[strcspn(novo->autor, "\n")] = 0;

        printf("Digite o ISBN: ");
        fgets(novo->isbn, sizeof(novo->isbn), stdin);
        novo->isbn[strcspn(novo->isbn, "\n")] = 0;

        printf("Digite o ano de publicação: ");
        scanf("%d", &novo->ano);
        getchar(); // Limpa o \n do buffer

        novo->prox = NULL;
        printf("Livro adicionado com sucesso!\n");
        return novo;
    }

    lista->prox = adicionarLivroRec(lista->prox);
    return lista;
}

void salvarEmArquivosRec(FILE *arquivo, Livro *lista){
    if(lista == NULL) return;
    fprintf(arquivo, "%s;%s;%S;%d\n",
    lista->titulo, lista->autor, lista->isbn, lista->ano);
    salvarEmArquivosRec(arquivo, lista->prox);
}

void salvarLivros(Livro *lista, const char *nomeArquivo){

};

// Salva lista em arquivo texto
void salvarEmArquivo(Livro *lista, const char *nomeArquivo) {
    FILE *arquivo = fopen(nomeArquivo, "w");
    if (!arquivo) {
        printf("Erro ao abrir arquivo para escrita!\n");
        return;
    }

    while (lista) {
        fprintf(arquivo, "%s;%s;%s;%d\n", lista->titulo, lista->autor, lista->isbn, lista->ano);
        lista = lista->prox;
    }

    fclose(arquivo);
};

Livro* carregarDeArquivo(const char *nomeArquivo){
    FILE *arquivo  = fopen(nomeArquivo, "r");
    if(!arquivo) return NULL;

    Livro *lista = NULL;
    char linha[256];

    while(fgets(linha, sizeof(linha), arquivo)) {
        Livro *novo = (Livro *)malloc(sizeof(Livro));
        if(!novo) continue;

        sscanf(linha, "%99[^;];%99[^;];%19[^;];%d", novo->titulo,novo->autor,novo->isbn, &novo->ano);

        novo->prox = lista;
        lista = novo;
    }
    fclose(arquivo);
    return lista;
};

int main(){
    Livro *lista = NULL;
    char opcao[10];
    char isbn[20];
    const char *arquivo = "Livros.txt";

    lista = carregarDeArquivo(arquivo);

    while(1){
        printf("\nMenu:\n");
        printf("1 - Adicionar Livros\n");
        printf("2 - Listar Livros");
        printf("3 - Remover Livros por ISBN\n");
        printf("4 - Sair\n");
        printf("Escolha uma opção: ");
        fgets(opcao,sizeof(opcao), stdin);

        switch (opcao[0]){
            case'1':
            adiconarLivro(&lista);
            break;
            case '2':
            listarLivros(lista);
            break;
            case'3':
            printf("Digite o ISBN do livro que deseja remover");
            fgets(isbn,sizeof(isbn),stdin);
            isbn[strcspn(isbn, "\n")] = 0;
            removerLivro(&lista, isbn);
            break;
            case'4':
            salvarEmArquivo(lista,arquivo);
            printf("Saindo...\n");
            return 0;
            default:
            printf("Opção inválida. Tente novamente.\n");
            
        }
    }
}