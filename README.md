# Sistema de Gerenciamento de Livros

**Objetivo:** O Sistema de Gerenciamento de Livros é uma aplicação em C que permite **adicionar, listar e remover** livros. Ele opera em **três modos**: **Terminal** (console), **Servidor TCP** (cliente interativo mantendo a conexão) e **Servidor HTTP** (interface web simples).  
Os dados são persistidos no arquivo `livros.txt` em **UTF-8** (com **BOM** no Windows, se necessário), garantindo suporte a acentos.

---

## Requisitos

- **Regras de Negócio**
  - Um livro possui: **Título**, **Autor**, **ISBN** e **Ano**.
  - Persistência no arquivo: `titulo;autor;isbn;ano`.
  - Suporte a acentos (UTF-8).  
  - **TCP**: cliente mantém a conexão e envia comandos (**READ**, **ADD**, **DELETE**, **EXIT**).
  - **HTTP**: listagem e remoção via navegador (GET).

- **Ambiente**
  - Compilador C: `gcc` (no Windows, MinGW).
  - **Windows**: Winsock (`-lws2_32`).
  - Terminal com suporte a UTF-8 (PowerShell/Windows Terminal no Windows; bash no Linux).
  - Navegador (Chrome/Firefox/etc.).
  - Permissão de escrita no diretório do projeto (para `livros.txt`).

---

## Modos de Operação

- **Modo Terminal (console)**
  - Menu: **Adicionar**, **Listar**, **Remover**, **Sair**.
  - Interação linha a linha.
  - Ao sair, salva em `livros.txt`.

- **Modo Servidor TCP (cliente interativo)**
  - Porta padrão: **9000** (parametrizável).
  - Protocolo **binário** com comandos:
    ```c
    enum Comando {
      LER_DADOS    = 100, // READ
      ENVIAR_DADOS = 101, // ADD
      APAGAR_DADOS = 102, // DELETE
      SAIR         = 103  // EXIT
    };

    typedef struct {
      char titulo[100];
      char autor[100];
      char isbn[20];
      int  ano;
    } LivroBinario;
    ```
  - Cliente mostra menu e mantém a conexão até escolher **Sair**.

- **Modo Servidor HTTP**
  - Porta padrão: **8080** (parametrizável).
  - **GET /** → página HTML com lista de livros.
  - **GET /delete/<isbn>** → remove e redireciona para `/`.

---

## Compilação

### Linux
```bash
# Servidor/multimodo
gcc sistema_livros.c -o sistema_livros

# Cliente TCP
gcc cliente_tcp.c -o cliente_tcp
```

### Windows (MinGW)
```bash
# Servidor/multimodo
gcc sistema_livros.c -o sistema_livros.exe -lws2_32

# Cliente TCP
gcc cliente_tcp.c -o cliente_tcp.exe -lws2_32
```

---

## Como Executar

### 1) Modo Terminal
```bash
./sistema_livros --modo-normal
```
**Funcionalidades:** Adicionar (título, autor, ISBN, ano) • Listar • Remover por ISBN • Sair (salva em `livros.txt`).

---

### 2) Modo Servidor TCP
**Servidor (porta padrão 9000 ou personalizada):**
```bash
./sistema_livros --modo-servidor-tcp
# ou
./sistema_livros --modo-servidor-tcp 9001
```

**Cliente (menu interativo, permanece conectado):**
```bash
./cliente_tcp <IP> [porta]
# Exemplo:
./cliente_tcp 127.0.0.1 9000
```

**Fluxo no cliente:**
```
1 - Listar livros
2 - Adicionar livro    # título/autor/ISBN/ano lidos com fgets (aceita espaços)
3 - Remover livro      # remove por ISBN
4 - Sair               # encerra a conexão
```

---

### 3) Modo Servidor HTTP
```bash
./sistema_livros --modo-http
# ou
./sistema_livros --modo-http 8081
```

**Acesso no navegador:**
```
http://localhost:8080
```
- Lista os livros e oferece links **Remover** (GET `/delete/<isbn>`).

---

## Documentação da API (HTTP)

#### Listar livros
```http
GET /
```

| Parâmetro | Tipo | Descrição |
| :-- | :-- | :-- |
| — | — | Retorna uma página HTML com a lista de livros |

**Resposta:** `200 OK` + HTML com **Título**, **Autor**, **ISBN**, **Ano**.

#### Remover livro
```http
GET /delete/<isbn>
```

| Parâmetro | Tipo     | Descrição                     |
| :-------- | :------- | :---------------------------- |
| `isbn`    | `string` | **Obrigatório**. ISBN alvo.   |

**Resposta:** `302 Found` (redirect para `/` após remover) • `400 Bad Request` (ISBN ausente/inválido)

---

## Formato do Arquivo `livros.txt`
Cada linha:
```
titulo;autor;isbn;ano
```

**Exemplos (UTF-8):**
```
João e o pé de feijão;Joãozinho;800;1235
Brás Cubas;Machado de Assis;7008;1950
Crônica de uma Morte Anunciada;Gabriel García Márquez;1234567890;1981
```

> Dica (Windows + Notepad++): **Encoding → Convert to UTF-8** (ou **UTF-8-BOM**).

---

## Solução de Problemas
- **Acentos estranhos no arquivo**: confirme UTF-8/UTF-8-BOM no editor.
- **Não salvou `livros.txt`**: verifique mensagens de erro e permissões de escrita.
- **Porta ocupada**: altere a porta (`--modo-servidor-tcp 9001`, `--modo-http 8081`).
- **Firewall (Windows)**: permita tráfego nas portas 9000/8080.
- **Cliente desconecta**: certifique-se de que o servidor está ativo e que os comandos foram enviados corretamente.

---

## Autor
- [@João Victor](https://github.com/dev-oliveira-jv)
