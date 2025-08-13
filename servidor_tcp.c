/* sistema_livros.c
   Compilar:
     Linux: gcc sistema_livros.c -o sistema_livros
     Windows (MinGW): gcc sistema_livros.c -o sistema_livros.exe -lws2_32
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib,"ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSESOCK closesocket
    #define SOCKINIT() do { WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); } while(0)
    #define SOCKCLEAN() WSACleanup()
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    typedef int socket_t;
    #define CLOSESOCK close
    #define SOCKINIT()
    #define SOCKCLEAN()
#endif

#define PORT_DEFAULT 9000
#define PORT_HTTP 8080
#define BUFFER_SIZE 1024
#define TAM 100

/* Protos dos modos */
void modoTerminal();
void modoServidorTCP(int porta);
void modoHttp(int porta);

/* Estrutura e funções de lista */
typedef struct Livro {
    char titulo[TAM];
    char autor[TAM];
    char isbn[20];
    int ano;
    struct Livro *prox;
} Livro;

void adicionarLivroInterativo(Livro **lista) {
    Livro *novo = (Livro*)malloc(sizeof(Livro));
    if (!novo) { printf("Erro alocacao\n"); return; }

    printf("Digite o titulo: ");
    fgets(novo->titulo, sizeof(novo->titulo), stdin);
    novo->titulo[strcspn(novo->titulo, "\r\n")] = 0;

    printf("Digite o autor: ");
    fgets(novo->autor, sizeof(novo->autor), stdin);
    novo->autor[strcspn(novo->autor, "\r\n")] = 0;

    printf("Digite o ISBN: ");
    fgets(novo->isbn, sizeof(novo->isbn), stdin);
    novo->isbn[strcspn(novo->isbn, "\r\n")] = 0;

    printf("Digite o ano: ");
    if (scanf("%d", &novo->ano) != 1) novo->ano = 0;
    getchar(); /* limpa \n */

    novo->prox = *lista;
    *lista = novo;
    printf("Livro adicionado.\n");
}

void listarLivros(Livro *lista) {
    if (!lista) { printf("Nenhum livro cadastrado.\n"); return; }
    while (lista) {
        printf("Titulo: %s\nAutor: %s\nISBN: %s\nAno: %d\n------------------------\n",
            lista->titulo, lista->autor, lista->isbn, lista->ano);
        lista = lista->prox;
    }
}

/* recursiva: remove por ISBN e retorna nova cabeca */
Livro* removerLivroRec(Livro *lista, const char *isbn) {
    if (!lista) return NULL;
    if (strcmp(lista->isbn, isbn) == 0) {
        Livro *next = lista->prox;
        free(lista);
        return next;
    }
    lista->prox = removerLivroRec(lista->prox, isbn);
    return lista;
}

void removerLivro(Livro **lista, const char *isbn) {
    *lista = removerLivroRec(*lista, isbn);
}

Livro* carregarDoArquivo(const char *nomeArquivo) {
    FILE *arq = fopen(nomeArquivo, "r");
    if (!arq) {
        printf("Erro ao abrir arquivo %s para leitura: %s\n", nomeArquivo, strerror(errno));
        return NULL;
    }
    Livro *lista = NULL;
    char linha[512];

    while (fgets(linha, sizeof(linha), arq)) {
        Livro *novo = (Livro*)malloc(sizeof(Livro));
        if (!novo) break;
        /* formato esperado: titulo;autor;isbn;ano\n */
        int items = sscanf(linha, "%99[^;];%99[^;];%19[^;];%d",
                           novo->titulo, novo->autor, novo->isbn, &novo->ano);
        if (items < 4) { /* linha mal formada */
            free(novo);
            continue;
        }
        novo->prox = lista;
        lista = novo;
    }
    fclose(arq);
    return lista;
}

void salvarEmArquivo(FILE *arq, Livro *lista) {
    while (lista) {
        fprintf(arq, "%s;%s;%s;%d\n", lista->titulo, lista->autor, lista->isbn, lista->ano);
        lista = lista->prox;
    }
}

void salvarLista(Livro *lista, const char *nomeArquivo) {
    FILE *arq = fopen(nomeArquivo, "w");
    if (!arq) {
        printf("Erro ao abrir arquivo %s para escrita: %s\n", nomeArquivo, strerror(errno));
        return;
    }
#ifdef _WIN32
    /* Adiciona o BOM para UTF-8 no Windows */
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    fwrite(bom, sizeof(bom), 1, arq);
#endif
    salvarEmArquivo(arq, lista);
    fclose(arq);
    printf("Arquivo %s salvo com sucesso.\n", nomeArquivo);
}

/* cria um texto com a lista (para TCP) */
void listarLivrosEmBuffer(Livro *lista, char *buffer, size_t bufflen) {
    buffer[0] = '\0';
    while (lista) {
        char linha[256];
        snprintf(linha, sizeof(linha),
            "Titulo: %s\nAutor: %s\nISBN: %s\nAno: %d\n------------------------\n",
            lista->titulo, lista->autor, lista->isbn, lista->ano);
        if (strlen(buffer) + strlen(linha) + 1 < bufflen) strcat(buffer, linha);
        lista = lista->prox;
    }
}

/* utilitario: compara prefixo sem case */
int startsWithIgnoreCase(const char *s, const char *prefix) {
    while (*prefix) {
        if (!*s) return 0;
        if (toupper((unsigned char)*s) != toupper((unsigned char)*prefix)) return 0;
        s++; prefix++;
    }
    return 1;
}

/* remove \r \n leading/trailing */
void trim_crlf(char *s) {
    char *p;
    p = strchr(s, '\r'); if (p) *p = '\0';
    p = strchr(s, '\n'); if (p) *p = '\0';
}

/* ---------------- MODOS ---------------- */

void modoTerminal() {
    Livro *lista = carregarDoArquivo("livros.txt");
    char opcao[10], isbn[40];
    while (1) {
        printf("\n1-Adicionar\n2-Listar\n3-Remover\n4-Sair\nOpcao: ");
        if (!fgets(opcao, sizeof(opcao), stdin)) break;
        switch (opcao[0]) {
            case '1': adicionarLivroInterativo(&lista); break;
            case '2': listarLivros(lista); break;
            case '3':
                printf("ISBN a remover: ");
                if (!fgets(isbn, sizeof(isbn), stdin)) break;
                isbn[strcspn(isbn, "\r\n")] = 0;
                lista = removerLivroRec(lista, isbn);
                printf("Se existia, o livro foi removido.\n");
                break;
            case '4':
                salvarLista(lista, "livros.txt");
                printf("Saindo.\n");
                return;
            default:
                printf("Opcao invalida\n");
        }
    }
}

/* modo servidor TCP */
void modoServidorTCP(int porta) {
    SOCKINIT();
    socket_t servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor < 0) { perror("socket"); SOCKCLEAN(); return; }

    int opt = 1;
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(porta);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(servidor, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); CLOSESOCK(servidor); SOCKCLEAN(); return;
    }

    if (listen(servidor, 5) < 0) {
        perror("listen"); CLOSESOCK(servidor); SOCKCLEAN(); return;
    }

    printf("Servidor TCP escutando na porta %d...\n", porta);
    Livro *lista = carregarDoArquivo("livros.txt");

    for (;;) {
        struct sockaddr_in cliente_addr;
        socklen_t len = sizeof(cliente_addr);
        socket_t cliente = accept(servidor, (struct sockaddr*)&cliente_addr, &len);
        if (cliente < 0) { perror("accept"); continue; }
        printf("Cliente conectado!\n");

        while (1) {
            char buffer[BUFFER_SIZE];
            int n = recv(cliente, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) { printf("Cliente desconectado.\n"); break; }
            buffer[n] = '\0';
            trim_crlf(buffer);

            if (startsWithIgnoreCase(buffer, "READ")) {
                char resposta[BUFFER_SIZE * 40];
                listarLivrosEmBuffer(lista, resposta, sizeof(resposta));
                if (strlen(resposta) == 0) strcpy(resposta, "Nenhum registro.\n");
                /* Envia o tamanho da resposta primeiro */
                unsigned int resp_len = htonl((unsigned int)strlen(resposta));
                send(cliente, (char*)&resp_len, sizeof(resp_len), 0);
                /* Envia a resposta */
                send(cliente, resposta, (int)strlen(resposta), 0);

            } else if (startsWithIgnoreCase(buffer, "DELETE ")) {
                char *sp = strchr(buffer, ' ');
                if (sp) {
                    char isbn[64];
                    strcpy(isbn, sp + 1);
                    trim_crlf(isbn);
                    lista = removerLivroRec(lista, isbn);
                    salvarLista(lista, "livros.txt");
                    const char *msg = "Livro removido com sucesso.\n";
                    send(cliente, msg, (int)strlen(msg), 0);
                } else {
                    const char *msg = "DELETE sem parametro\n";
                    send(cliente, msg, (int)strlen(msg), 0);
                }

            } else if (startsWithIgnoreCase(buffer, "ADD")) {
                Livro novo;
                n = recv(cliente, (char*)&novo, sizeof(Livro), 0);
                if (n <= 0) { printf("Erro ao receber dados do livro.\n"); break; }
                Livro *novo_livro = (Livro*)malloc(sizeof(Livro));
                if (!novo_livro) {
                    const char *msg = "Erro de alocação no servidor.\n";
                    send(cliente, msg, strlen(msg), 0);
                    continue;
                }
                memcpy(novo_livro, &novo, sizeof(Livro));
                novo_livro->prox = lista;
                lista = novo_livro;
                salvarLista(lista, "livros.txt");
                const char *msg = "Livro adicionado com sucesso.\n";
                send(cliente, msg, strlen(msg), 0);
                printf("Adicionado: %s - %s (%d) ISBN: %s\n", novo.titulo, novo.autor, novo.ano, novo.isbn);

            } else {
                const char *msg = "Comando invalido\n";
                send(cliente, msg, (int)strlen(msg), 0);
            }
        }

        CLOSESOCK(cliente);
    }

    salvarLista(lista, "livros.txt");
    CLOSESOCK(servidor);
    SOCKCLEAN();
}

/* Modo HTTP simples (porta 8080) */
void gerarPaginaHTML(Livro *lista, char *html, size_t htmlsize) {
    snprintf(html, htmlsize, "<html><head><meta charset=\"utf-8\"><title>Livros</title></head><body><h1>Lista de Livros</h1><ul>");
    size_t used = strlen(html);
    while (lista) {
        char item[512];
        snprintf(item, sizeof(item),
            "<li><strong>%s</strong> - %s (%d) <br>ISBN: %s<br>"
            "<a href=\"/delete/%s\">Remover</a></li><hr>",
            lista->titulo, lista->autor, lista->ano, lista->isbn, lista->isbn);
        if (used + strlen(item) + 32 < htmlsize) {
            strcat(html, item);
            used = strlen(html);
        }
        lista = lista->prox;
    }
    strcat(html, "</ul></body></html>");
}

void modoHttp(int porta) {
    SOCKINIT();
    socket_t servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor < 0) { perror("socket"); SOCKCLEAN(); return; }

    int opt = 1;
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(porta);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(servidor, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); CLOSESOCK(servidor); SOCKCLEAN(); return;
    }
    if (listen(servidor, 5) < 0) {
        perror("listen"); CLOSESOCK(servidor); SOCKCLEAN(); return;
    }

    printf("Servidor HTTP em http://localhost:%d\n", porta);
    Livro *lista = carregarDoArquivo("livros.txt");

    for (;;) {
        struct sockaddr_in cliente;
        socklen_t tam = sizeof(cliente);
        socket_t sock = accept(servidor, (struct sockaddr*)&cliente, &tam);
        if (sock < 0) continue;

        char req[BUFFER_SIZE * 4];
        int n = recv(sock, req, sizeof(req) - 1, 0);
        if (n <= 0) { CLOSESOCK(sock); continue; }
        req[n] = '\0';

        /* pega a linha inicial: GET /path HTTP/1.1 */
        char method[16], path[256];
        if (sscanf(req, "%15s %255s", method, path) != 2) {
            CLOSESOCK(sock); continue;
        }

        if (startsWithIgnoreCase(method, "GET") && strcmp(path, "/") == 0) {
            char html[BUFFER_SIZE * 10];
            gerarPaginaHTML(lista, html, sizeof(html));
            char header[512];
            snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %zu\r\n\r\n",
                strlen(html));
            send(sock, header, (int)strlen(header), 0);
            send(sock, html, (int)strlen(html), 0);

        } else if (startsWithIgnoreCase(method, "GET") && startsWithIgnoreCase(path, "/delete/")) {
            /* extrai isbn */
            const char *isbn = path + strlen("/delete/");
            if (*isbn) {
                lista = removerLivroRec(lista, isbn);
                salvarLista(lista, "livros.txt");
                /* redireciona */
                const char *resp = "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
                send(sock, resp, (int)strlen(resp), 0);
            } else {
                const char *resp = "HTTP/1.1 400 Bad Request\r\n\r\n";
                send(sock, resp, (int)strlen(resp), 0);
            }
        } else {
            const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNot found";
            send(sock, resp, (int)strlen(resp), 0);
        }

        CLOSESOCK(sock);
    }

    salvarLista(lista, "livros.txt");
    CLOSESOCK(servidor);
    SOCKCLEAN();
}

/* Main */
int main(int argc, char *argv[]) {
    /* Configura locale para UTF-8 */
    setlocale(LC_ALL, "");
#ifdef _WIN32
    SetConsoleOutputCP(65001); /* UTF-8 no console do Windows */
#endif

    if (argc < 2) {
        printf("Uso: %s [--modo-normal | --modo-servidor-tcp [porta] | --modo-http [porta]]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--modo-normal") == 0) {
        modoTerminal();
    } else if (strcmp(argv[1], "--modo-servidor-tcp") == 0) {
        int porta = PORT_DEFAULT;
        if (argc >= 3) porta = atoi(argv[2]);
        modoServidorTCP(porta);
    } else if (strcmp(argv[1], "--modo-http") == 0) {
        int porta = PORT_HTTP;
        if (argc >= 3) porta = atoi(argv[2]);
        modoHttp(porta);
    } else {
        printf("Modo invalido: %s\n", argv[1]);
        return 1;
    }

    return 0;
}