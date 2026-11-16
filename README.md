# Algoritmo do Banqueiro – Implementação 6 (Deadlock)

Infraestrutura de Software – CESAR School
Implementação do Algoritmo do Banqueiro (Silberschatz, *Operating System Concepts*, 10ª ed., Cap. 8, Seção 8.6.3), que só concede um pedido de recursos se o sistema continuar em estado seguro.

## Arquivos

- `banker.c` – código-fonte: leitura das entradas, algoritmo de segurança, requisição (RQ), liberação (RL) e impressão do estado (`*`).
- `Makefile` – compila o programa e limpa os arquivos gerados.
- `tests/` – arquivos de entrada para testar os casos de erro.
- `evidencias.log` – registro do terminal durante o desenvolvimento (`script -a`).

## Como compilar

```
make          # gera o executável banker
make clean    # apaga o executável e os .o
```

## Como executar

Cada argumento é o número de instâncias de um tipo de recurso:

```
./banker 10 5 7
```

O programa lê dois arquivos da pasta atual e gera um terceiro:

- `customer.txt` – uma linha por cliente com o máximo de cada recurso, separado por vírgula. O número de linhas define o número de clientes.
  ```
  7,5,3
  3,2,2
  ```
- `commands.txt` – um comando por linha:
  - `RQ c r0 r1 ...` – o cliente `c` pede recursos
  - `RL c r0 r1 ...` – o cliente `c` devolve recursos
  - `*` – imprime as matrizes MAXIMUM, ALLOCATION e NEED e o vetor AVAILABLE
- `result.txt` – o resultado de cada comando. Um pedido é negado quando excede o máximo do cliente, quando não há recursos disponíveis ou quando deixaria o sistema em estado inseguro.

Em caso de sucesso, nada é mostrado na tela.

## Erros tratados

O programa mostra uma mensagem e sai com código 1 quando:

- não há argumentos, ou um argumento não é um inteiro >= 0;
- o número de recursos em `customer.txt` ou `commands.txt` é diferente do número de argumentos;
- `customer.txt` ou `commands.txt` não existe ou não pode ser aberto;
- `customer.txt` está vazio ou tem formato inválido (ex.: `7,,5,3`);
- `commands.txt` tem um comando desconhecido, um cliente inexistente, um valor que não é número ou `*` seguido de outros caracteres.

Todo o `commands.txt` é validado antes de qualquer comando ser executado.

Um `RL` que devolve mais do que o cliente tem não encerra o programa: o pedido é negado com uma mensagem no `result.txt` e os demais comandos continuam.

## Testes

Para rodar um caso de erro, troque o `commands.txt` por um arquivo de `tests/`:

```
cp commands.txt commands.bak
cp tests/cmd_cliente.txt commands.txt
./banker 10 5 7; echo $?
cp commands.bak commands.txt
```