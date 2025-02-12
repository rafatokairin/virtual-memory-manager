# Simulador de Gerenciamento de Memória Virtual

Problema retirado do capítulo 9 (Designing a Virtual Memory Manager) do livro Operating System Concepts, de Abraham Silberschatz e James Peterson. 

- Este programa simula a tradução de endereços lógicos para físicos, utilizando uma TLB e tabela de páginas. 

## Compilação

Para compilar o programa, use:

```
make
```

## Execução

O programa deve ser executado com três argumentos:

```
./vmm addresses.txt <frames> <FIFO|LRU>
```

Exemplo:

```
./vmm addresses.txt 128 FIFO
```

## Limpeza

Para remover o executável gerado:

```
make clean
```
