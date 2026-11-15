
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


    libera_tudo();
    return 0;
}