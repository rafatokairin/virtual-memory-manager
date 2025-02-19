#include "process_file.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

std::deque<int> deque; // Fila duplamente encadeada usada para FIFO|LRU
std::size_t NUM_FRAMES;
int page_faults = 0;      // Contador de page faults
int TLB_hits = 0;         // Contador de TLB hits
int total_references = 0; // Total de referências de endereço
char **PhysicalMemory;

// Extrai o número da página de um endereço lógico
int extract_page_number(int logical_address) {
  return (logical_address >> 8) & 0xFF;
}

// Extrai o offset de um endereço lógico
int extract_offset(int logical_address) { return logical_address & 0xFF; }

// Verifica se a página está na TLB
int check_TLB(int page_number) {
  for (const auto &entry : TLB)
    if (entry.page_number == page_number)
      return entry.frame_number;
  return -1; // TLB miss
}

// Verifica se a página está na tabela de páginas
int check_page_table(int page_number) {
  if (PageTable.find(page_number) != PageTable.end() &&
      PageTable[page_number].valid)
    return PageTable[page_number].frame_number;
  return -1; // Page fault
}

// Função para substituição de página
int handle_page_fault(int page_number) {
  int frame_number;

  if (deque.size() >= NUM_FRAMES) {
    // Memória física está cheia, substitui o frame mais antigo
    frame_number = deque.front();
    deque.pop_front();

    // Invalida a página substituída na tabela de páginas
    for (auto &entry : PageTable)
      if (entry.second.frame_number == frame_number)
        entry.second.valid = false;
  } else // Atribui um novo frame
    frame_number = deque.size();

  // Lê a página do arquivo BACKING_STORE.bin
  FILE *backing_store = fopen("BACKING_STORE.bin", "rb");
  fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
  fread(PhysicalMemory[frame_number], sizeof(char), PAGE_SIZE, backing_store);
  fclose(backing_store);

  // Atualiza a tabela de páginas
  PageTable[page_number] = {frame_number, true};

  // Adiciona o frame à fila
  deque.push_back(frame_number);
  return frame_number;
}

// Função para traduzir endereços lógicos em físicos
void translate_address(int logical_address,
                       const std::string &replacement_method,
                       std::ofstream &outfile) {
  total_references++; // Incrementa o total de referências

  int page_number = extract_page_number(logical_address);
  int offset = extract_offset(logical_address);

  int frame_number = check_TLB(page_number);
  if (frame_number == -1) {
    frame_number = check_page_table(page_number);
    if (frame_number == -1) {
      // Escolhe o método de substituição com base no argumento
      frame_number = handle_page_fault(page_number);
      page_faults++; // Incrementa o contador de page faults
    }
    // Atualiza a ordem no uso no FIFO, se a TLB já está na memória
    if (TLB.size() >= TLB_SIZE)
      TLB.pop_front();
    TLB.push_back({page_number, frame_number});
  } else {
    // Atualiza a ordem de uso no LRU, se a TLB já está na memória
    if (replacement_method == "LRU" && frame_number != -1) {
      auto it = std::find(TLB.begin(), TLB.end(), frame_number);
      if (it != TLB.end()) {
        TLB.erase(it); // Remove o frame da posição atual
        TLB.push_back(
            {page_number, frame_number}); // Adiciona ao final (mais recente)
      }
    }
    TLB_hits++; // Incrementa o contador de TLB hits
  }
  // Atualiza a ordem de uso no LRU, se a página já está na memória
  if (replacement_method == "LRU" && frame_number != -1) {
    auto it = std::find(deque.begin(), deque.end(), frame_number);
    if (it != deque.end()) {
      deque.erase(it);               // Remove o frame da posição atual
      deque.push_back(frame_number); // Adiciona ao final (mais recente)
    }
  }

  int physical_address = frame_number * PAGE_SIZE + offset;
  char value = PhysicalMemory[frame_number][offset];

  outfile << "Logical Address: " << logical_address
          << " Physical Address: " << physical_address
          << " Value: " << (int)value << std::endl;
}

void process_file(std::string filename, std::string replacement_method) {
  // Abre o arquivo addresses.txt
  std::ifstream address_file(filename);
  std::string line;
  // Abre o arquivo de saída
  std::ofstream outfile("correct.txt");
  while (std::getline(address_file, line)) {
    // Verifica se a linha é "PageTable" ou "TLB"
    if (line == "PageTable")
      print_page_table(outfile);
    else if (line == "TLB")
      print_TLB(outfile);
    else {
      // Caso contrário, processa como um endereço lógico
      int logical_address = std::stoi(line);
      translate_address(logical_address, replacement_method, outfile);
    }
  }
  // Fecha o arquivo addresses.txt
  address_file.close();
  // Exibe estatísticas ao final da execução
  display_statistics(outfile);
  // Fecha o arquivo de saída
  outfile.close();
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: ./vmm addresses.txt <frames> <FIFO|LRU>" << std::endl;
    return 1;
  }

  std::string filename = argv[1];
  NUM_FRAMES = std::atoi(argv[2]);
  std::string replacement_method = argv[3];
  // Verifica se o método de substituição é válido
  if (replacement_method != "FIFO" && replacement_method != "LRU") {
    std::cerr << "Invalid replacement method. Use 'FIFO' or 'LRU'."
              << std::endl;
    return 1;
  }

  /* Inicializa a PageTable e TLB com entradas inválidas (caso queira visualizar
   * em correct.txt todas as páginas de PageTable ou TLB) */
  // initialize_page_table();
  // initialize_TLB();

  // Aloca memória dinamicamente
  PhysicalMemory = new char *[NUM_FRAMES];
  for (std::size_t i = 0; i < NUM_FRAMES; i++)
    PhysicalMemory[i] = new char[PAGE_SIZE];

  // Processa a leitura e escrita dos arquivos .txt
  process_file(filename, replacement_method);

  // Libera a memória alocada
  for (std::size_t i = 0; i < NUM_FRAMES; i++)
    delete[] PhysicalMemory[i];
  delete[] PhysicalMemory;
  return 0;
}
