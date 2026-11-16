
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA 1024

int n_customers, n_resources;
int *available;
int **maximum, **allocation, **need;

void limpa_fim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ')) s[--len] = '\0';
}

int converte(const char *s, int *saida)
{
    char *fim;
    long v = strtol(s, &fim, 10);
    if (fim == s || *fim != '\0' || v < 0)
        return 0;
    *saida = (int)v;
    return 1;
}

int cabe(int *a, int *b)
{
    for (int j = 0; j < n_resources; j++)
        if (a[j] > b[j])
            return 0;
    return 1;
}

/* Retorna 1 se o estado atual e seguro, 0 se inseguro */
int estado_seguro(void)
{
    int *work = malloc(n_resources * sizeof(int));
    int *finish = malloc(n_customers * sizeof(int));

    for (int j = 0; j < n_resources; j++)
        work[j] = available[j];
    for (int i = 0; i < n_customers; i++)
        finish[i] = 0;

    int achou;
    do {
        achou = 0;
        for (int i = 0; i < n_customers; i++) {
            if (finish[i] == 0 && cabe(need[i], work)) {
                for (int j = 0; j < n_resources; j++)
                    work[j] += allocation[i][j];
                finish[i] = 1;
                achou = 1;
            }
        }
    } while (achou);

    for (int i = 0; i < n_customers; i++) {
        if (finish[i] == 0) {
            free(work);
            free(finish);
            return 0;
        }
    }
    free(work);
    free(finish);
    return 1;
}

/* Retorna 0 = alocou, 1 = excede o maximo, 2 = nao ha recursos, 3 = inseguro */
int requisita(int c, int *req)
{
    if (!cabe(req, need[c]))
        return 1;
    if (!cabe(req, available))
        return 2;

    /* passo 3: fingir que alocou */
    for (int j = 0; j < n_resources; j++) {
        available[j] -= req[j];
        allocation[c][j] += req[j];
        need[c][j] -= req[j];
    }

    if (estado_seguro())
        return 0;

    /* inseguro: desfaz exatamente o que fez */
    for (int j = 0; j < n_resources; j++) {
        available[j] += req[j];
        allocation[c][j] -= req[j];
        need[c][j] += req[j];
    }
    return 3;
}

/* Retorna 0 = ok, 1 = erro (devolvendo mais do que tem) */
int libera(int c, int *rel)
{
    if (!cabe(rel, allocation[c]))
        return 1;

    for (int j = 0; j < n_resources; j++) {
        allocation[c][j] -= rel[j];
        available[j] += rel[j];
        need[c][j] += rel[j];
    }
    return 0;
}

/* Escreve os m numeros de v separados por espaco (sem espaco no fim) */
void escreve_vetor(FILE *s, int *v)
{
    for (int j = 0; j < n_resources; j++) {
        if (j > 0)
            fprintf(s, " ");
        fprintf(s, "%d", v[j]);
    }
}

/* Monta o texto de um vetor no buffer: "7 5 3 " (cada numero seguido de espaco) */
void vetor_texto(char *buf, size_t tam, int *v)
{
    buf[0] = '\0';
    size_t usado = 0;
    for (int j = 0; j < n_resources && usado < tam; j++)
        usado += snprintf(buf + usado, tam - usado, "%d ", v[j]);
}

/* Comando "*": imprime a tabela MAXIMUM | ALLOCATION | NEED e o AVAILABLE */
void imprime_estado(FILE *s)
{
    char bufMax[256], bufAlloc[256], bufNeed[256];

    fprintf(s, "%-10s|   %-12s|   %s\n", "MAXIMUM", "ALLOCATION", "NEED");

    for (int i = 0; i < n_customers; i++) {
        vetor_texto(bufMax, sizeof bufMax, maximum[i]);
        vetor_texto(bufAlloc, sizeof bufAlloc, allocation[i]);
        vetor_texto(bufNeed, sizeof bufNeed, need[i]);
        fprintf(s, "%-10s|   %-12s|   %s\n", bufMax, bufAlloc, bufNeed);
    }

    fprintf(s, "AVAILABLE ");
    for (int j = 0; j < n_resources; j++)
        fprintf(s, "%d ", available[j]);
    fprintf(s, "\n");
}

void libera_tudo(void)
{
    for (int i = 0; i < n_customers; i++) {
        free(maximum[i]);
        free(allocation[i]);
        free(need[i]);
    }
    free(maximum);
    free(allocation);
    free(need);
    free(available);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "uso: ./banker r1 r2 ...\n");
        return 1;
    }

    n_resources = argc - 1;
    available = malloc(n_resources * sizeof(int));

    for (int j = 0; j < n_resources; j++) {
        if (!converte(argv[j + 1], &available[j])) {
            fprintf(stderr, "Erro: \"%s\" nao e um inteiro >= 0\n", argv[j + 1]);
            free(available);
            return 1;
        }
    }

    FILE *f = fopen("customer.txt", "r");
    if (f == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir customer.txt\n");
        free(available);
        return 1;
    }

    char linha[TAM_LINHA];

    n_customers = 0;
    while (fgets(linha, sizeof linha, f) != NULL) {
        limpa_fim(linha);
        if (linha[0] != '\0')
            n_customers++;
    }

    if (n_customers == 0) {
        fprintf(stderr, "Erro: customer.txt esta vazio\n");
        fclose(f);
        free(available);
        return 1;
    }
    rewind(f);

    maximum = malloc(n_customers * sizeof(int *));
    allocation = malloc(n_customers * sizeof(int *));
    need = malloc(n_customers * sizeof(int *));
    for (int i = 0; i < n_customers; i++) {
        maximum[i] = malloc(n_resources * sizeof(int));
        allocation[i] = malloc(n_resources * sizeof(int));
        need[i] = malloc(n_resources * sizeof(int));
    }

    int i = 0;
    while (fgets(linha, sizeof linha, f) != NULL) {
        limpa_fim(linha);
        if (linha[0] == '\0')
            continue;

        size_t tam = strlen(linha);
        if (linha[0] == ',' || linha[tam - 1] == ',' || strstr(linha, ",,") != NULL) {
            fprintf(stderr, "Erro: formato invalido em customer.txt: \"%s\"\n", linha);
            fclose(f);
            libera_tudo();
            return 1;
        }

        int j = 0;
        char *tok = strtok(linha, ",");
        while (tok != NULL) {
            int v;
            if (!converte(tok, &v)) {
                fprintf(stderr, "Erro: valor invalido \"%s\" em customer.txt\n", tok);
                fclose(f);
                libera_tudo();
                return 1;
            }
            if (j < n_resources)
                maximum[i][j] = v;
            j++;
            tok = strtok(NULL, ",");
        }

        if (j != n_resources) {
            fprintf(stderr, "Erro: customer.txt tem %d recursos, mas foram passados %d\n", j, n_resources);
            fclose(f);
            libera_tudo();
            return 1;
        }

        for (j = 0; j < n_resources; j++) {
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
        i++;
    }
    fclose(f);

    f = fopen("commands.txt", "r");
    if (f == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir commands.txt\n");
        libera_tudo();
        return 1;
    }

    /* 1a passada: so valida */
    int num_linha = 0;
    while (fgets(linha, sizeof linha, f) != NULL) {
        num_linha++;
        limpa_fim(linha);
        if (linha[0] == '\0')
            continue;

        char *tok = strtok(linha, " ");
        if (strcmp(tok, "*") == 0) {
            if (strtok(NULL, " ") != NULL) {
                fprintf(stderr, "Erro: formato invalido na linha %d de commands.txt\n", num_linha);
                fclose(f);
                libera_tudo();
                return 1;
            }
        } else if (strcmp(tok, "RQ") == 0 || strcmp(tok, "RL") == 0) {
            int c;
            tok = strtok(NULL, " ");
            if (tok == NULL || !converte(tok, &c) || c >= n_customers) {
                fprintf(stderr, "Erro: cliente invalido na linha %d de commands.txt\n", num_linha);
                fclose(f);
                libera_tudo();
                return 1;
            }

            int qtd = 0;
            tok = strtok(NULL, " ");
            while (tok != NULL) {
                int v;
                if (!converte(tok, &v)) {
                    fprintf(stderr, "Erro: formato invalido na linha %d de commands.txt\n", num_linha);
                    fclose(f);
                    libera_tudo();
                    return 1;
                }
                qtd++;
                tok = strtok(NULL, " ");
            }

            if (qtd != n_resources) {
                fprintf(stderr, "Erro: commands.txt tem %d recursos, mas foram passados %d\n", qtd, n_resources);
                fclose(f);
                libera_tudo();
                return 1;
            }
        } else {
            fprintf(stderr, "Erro: comando invalido \"%s\" na linha %d de commands.txt\n", tok, num_linha);
            fclose(f);
            libera_tudo();
            return 1;
        }
    }

    rewind(f);
    FILE *saida = fopen("result.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar result.txt\n");
        fclose(f);
        libera_tudo();
        return 1;
    }

    /* 2a passada: executa (ja foi tudo validado) */
    int vetor[n_resources];
    while (fgets(linha, sizeof linha, f) != NULL) {
        limpa_fim(linha);
        if (linha[0] == '\0')
            continue;

        char *cmd = strtok(linha, " ");
        if (strcmp(cmd, "*") == 0) {
            imprime_estado(saida);
            continue;
        }

        int c;
        converte(strtok(NULL, " "), &c);
        for (int j = 0; j < n_resources; j++)
            converte(strtok(NULL, " "), &vetor[j]);

        if (strcmp(cmd, "RQ") == 0) {
            int r = requisita(c, vetor);
            if (r == 0) {
                fprintf(saida, "Allocate to customer %d the resources ", c);
                escreve_vetor(saida, vetor);
                fprintf(saida, "\n");
            } else if (r == 1) {
                fprintf(saida, "The customer %d request ", c);
                escreve_vetor(saida, vetor);
                fprintf(saida, " was denied because exceed its maximum allocation\n");
            } else if (r == 2) {
                fprintf(saida, "The resources ");
                escreve_vetor(saida, available);
                fprintf(saida, " are not enough to customer %d request ", c);
                escreve_vetor(saida, vetor);
                fprintf(saida, " \n");
            } else {
                fprintf(saida, "The customer %d request ", c);
                escreve_vetor(saida, vetor);
                fprintf(saida, " was denied because result in an unsafe state\n");
            }
        } else { /* RL */
            if (libera(c, vetor) == 0) {
                fprintf(saida, "Release from customer %d the resources ", c);
                escreve_vetor(saida, vetor);
                fprintf(saida, "\n");
            } else {
                fprintf(saida, "The customer %d released ", c);
                escreve_vetor(saida, vetor);
                fprintf(saida, " was denied because exceed its allocation\n");
            }
        }
    }

    fclose(f);
    fclose(saida);

    libera_tudo();
    return 0;
}