
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

    printf("Available:");
    for (int j = 0; j < n_resources; j++) printf(" %d", available[j]);
    printf("\nMax:\n");
    for (int c = 0; c < n_customers; c++) {
        for (int j = 0; j < n_resources; j++) printf("%d ", maximum[c][j]);
        printf("\n");
    }

    int r1[] = {0, 2, 1}, r2[] = {3, 0, 2}, r3[] = {2, 0, 0}, r4[] = {0, 0, 2};
    int l1[] = {0, 1, 1}, r5[] = {2, 1, 1}, r6[] = {4, 0, 0}, r7[] = {1, 0, 2};
    int r8[] = {0, 2, 0}, r9[] = {0, 1, 0};
    printf("%d ", requisita(0, r1));
    printf("%d ", requisita(2, r2));
    printf("%d ", requisita(1, r3));
    printf("%d ", requisita(4, r4));
    printf("%d ", libera(0, l1));
    printf("%d ", requisita(3, r5));
    printf("%d ", requisita(4, r6));
    printf("%d ", requisita(1, r7));
    printf("%d ", requisita(0, r8));
    printf("%d\n", requisita(2, r9));

    libera_tudo();
    return 0;
}