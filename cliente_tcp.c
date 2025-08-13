/* cliente_tcp.c
   Compilar:
     Linux: gcc cliente_tcp.c -o cliente_tcp
     Windows: gcc cliente_tcp.c -o cliente_tcp.exe -lws2_32
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib,"ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSESOCK closesocket
    #define SOCKINIT() do { WSADATA w; WSAStartup(MAKEWORD(2,2), &w); } while(0)
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

#define BUFFER_SIZE 4096
#define PORT_DEFAULT 9000
#define TAM 100

/* Estrutura do livro, deve ser igual à do servidor */
typedef struct {
    char titulo[TAM];
    char autor[TAM];
    char isbn[20];
    int ano;
} Livro;

int is_number(const char *s) {
    if (!s || !*s) return 0;
    while (*s) { if (!isdigit((unsigned char)*s)) return 0; s++; }
    return 1;
}

void trim_crlf(char *s) {
    char *p;
    p = strchr(s, '\r'); if (p) *p = '\0';
    p = strchr(s, '\n'); if (p) *p = '\0';
}

int main(int argc, char *argv[]) {
    /* Configura locale para UTF-8 */
    setlocale(LC_ALL, "");
#ifdef _WIN32
    SetConsoleOutputCP(65001); /* UTF-8 no console do Windows */
#endif

    if (argc < 2) {
        printf("Uso: %s <IP> [porta]\n", argv[0]);
        printf("Ex: %s 127.0.0.1\n", argv[0]);
        printf("Ex: %s 127.0.0.1 9000\n", argv[0]);
        return 1;
    }

    int porta = PORT_DEFAULT;
    if (argc >= 3 && is_number(argv[2])) {
        porta = atoi(argv[2]);
    }

    SOCKINIT();

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); SOCKCLEAN(); return 1; }

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(porta);
    servidor.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&servidor, sizeof(servidor)) < 0) {
        perror("connect");
        CLOSESOCK(sock);
        SOCKCLEAN();
        return 1;
    }

    printf("Conectado ao servidor %s:%d\n", argv[1], porta);

    char opcao[10];
    char buffer[BUFFER_SIZE];
    int bytes;
    while (1) {
        printf("\nMenu:\n");
        printf("1 - Adicionar livro\n");
        printf("2 - Listar livros\n");
        printf("3 - Remover livro\n");
        printf("4 - Sair\n");
        printf("Opcao: ");
        if (!fgets(opcao, sizeof(opcao), stdin)) break;
        opcao[strcspn(opcao, "\r\n")] = 0;

        if (opcao[0] == '1') {
            Livro novo;
            printf("Digite o titulo: ");
            if (!fgets(novo.titulo, sizeof(novo.titulo), stdin)) continue;
            novo.titulo[strcspn(novo.titulo, "\r\n")] = 0;

            printf("Digite o autor: ");
            if (!fgets(novo.autor, sizeof(novo.autor), stdin)) continue;
            novo.autor[strcspn(novo.autor, "\r\n")] = 0;

            printf("Digite o ISBN: ");
            if (!fgets(novo.isbn, sizeof(novo.isbn), stdin)) continue;
            novo.isbn[strcspn(novo.isbn, "\r\n")] = 0;

            printf("Digite o ano: ");
            if (scanf("%d", &novo.ano) != 1) novo.ano = 0;
            getchar(); /* limpa \n */

            /* Envia comando ADD seguido da estrutura */
            strcpy(buffer, "ADD");
            send(sock, buffer, strlen(buffer), 0);
            send(sock, (char*)&novo, sizeof(Livro), 0);

            /* Recebe resposta */
            bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                printf("Conexão perdida.\n");
                break;
            }
            buffer[bytes] = '\0';
            printf("%s", buffer);

        } else if (opcao[0] == '2') {
            strcpy(buffer, "READ");
            send(sock, buffer, strlen(buffer), 0);

            /* Recebe o tamanho da resposta */
            unsigned int resp_len;
            bytes = recv(sock, (char*)&resp_len, sizeof(resp_len), 0);
            if (bytes <= 0) {
                printf("Conexão perdida.\n");
                break;
            }
            resp_len = ntohl(resp_len); /* Converte de network para host byte order */

            /* Recebe a resposta */
            size_t total_received = 0;
            while (total_received < resp_len) {
                bytes = recv(sock, buffer + total_received, resp_len - total_received, 0);
                if (bytes <= 0) {
                    printf("Erro ao receber dados ou conexão perdida.\n");
                    break;
                }
                total_received += bytes;
            }
            if (total_received > 0) {
                buffer[total_received] = '\0';
                printf("%s", buffer);
            } else if (bytes == 0) {
                printf("Conexão perdida.\n");
                break;
            }

        } else if (opcao[0] == '3') {
            printf("ISBN a remover: ");
            char isbn[20];
            if (!fgets(isbn, sizeof(isbn), stdin)) continue;
            isbn[strcspn(isbn, "\r\n")] = 0;
            snprintf(buffer, sizeof(buffer), "DELETE %s", isbn);
            send(sock, buffer, strlen(buffer), 0);

            /* Recebe resposta */
            bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                printf("Conexão perdida.\n");
                break;
            }
            buffer[bytes] = '\0';
            printf("%s", buffer);

        } else if (opcao[0] == '4') {
            printf("Desconectando.\n");
            break;
        } else {
            printf("Opção inválida.\n");
        }
    }

    CLOSESOCK(sock);
    SOCKCLEAN();
    return 0;
}