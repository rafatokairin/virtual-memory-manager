#include <algorithm>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>

std::deque<int> deque;   // Usado para FIFO|LRU

const int PAGE_SIZE = 256;
int NUM_FRAMES;
const int TLB_SIZE = 16;

int page_faults = 0;      // Contador de page faults
int TLB_hits = 0;         // Contador de TLB hits
int total_references = 0; // Total de referências de endereço

struct TLBEntry {
  int page_number;
  int frame_number;
  // Operador para comparação de TLBEntry.frame_number com frame_number de std::find
  bool operator==(int frame) const { return frame_number == frame; }
};

struct PageTableEntry {
  int frame_number;
  bool valid;
};

std::deque<TLBEntry> TLB;
std::unordered_map<int, PageTableEntry> PageTable;
char **PhysicalMemory;

// Função para imprimir a tabela de páginas
void print_page_table(std::ofstream &outfile) {
  outfile << "########################\n";
  outfile << "Page - Frame - Valid Bit\n";
  for (const auto &entry : PageTable) {
    int page_number = entry.first;
    int frame_number = entry.second.frame_number;
    bool valid = entry.second.valid;

    outfile << std::setw(4) << page_number << " - " << std::setw(5)
            << frame_number << " - " << std::setw(9)
            << (valid ? "Valid" : "Invalid") << std::endl;
  }
  outfile << "########################\n";
}

// Função para imprimir a TLB
void print_TLB(std::ofstream &outfile) {
  outfile << "\n************\n";
  outfile << "Page - Frame\n";
  for (const auto &entry : TLB) {
    int page_number = entry.page_number;
    int frame_number = entry.frame_number;

    outfile << std::setw(4) << page_number << " - " << std::setw(5)
            << frame_number << std::endl;
  }
  outfile << "************\n";
}

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

// Função para substituição FIFO
int handle_page_fault_fifo(int page_number) {
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

// Função para substituição LRU
int handle_page_fault_lru(int page_number) {
  int frame_number;

  if (deque.size() >= NUM_FRAMES) {
    // Memória física está cheia, substitui o frame menos recentemente usado
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

  // Adiciona o frame ao final da fila LRU
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
      if (replacement_method == "FIFO")
        frame_number = handle_page_fault_fifo(page_number);
      else if (replacement_method == "LRU")
        frame_number = handle_page_fault_lru(page_number);
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

// Exibe estatísticas ao final da execução
void display_statistics(std::ofstream &outfile) {
  float page_fault_rate = (float)page_faults / total_references * 100;
  float TLB_hit_rate = (float)TLB_hits / total_references * 100;

  outfile << "\nStatistics:\n";
  outfile << "Page-Fault Rate: " << page_fault_rate << "%\n";
  outfile << "TLB Hit Rate: " << TLB_hit_rate << "%\n";
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: ./vmm addresses.txt <frames> <FIFO|LRU>" << std::endl;
    std::cerr << "Replacement methods: FIFO, LRU" << std::endl;
    return 1;
  }

  NUM_FRAMES = std::atoi(argv[2]);
  std::string replacement_method = argv[3];

  // Verifica se o método de substituição é válido
  if (replacement_method != "FIFO" && replacement_method != "LRU") {
    std::cerr << "Invalid replacement method. Use 'FIFO' or 'LRU'."
              << std::endl;
    return 1;
  }

  // Aloca memória dinamicamente
  PhysicalMemory = new char *[NUM_FRAMES];
  for (int i = 0; i < NUM_FRAMES; i++)
    PhysicalMemory[i] = new char[PAGE_SIZE];

  // Abre o arquivo addresses.txt
  std::ifstream address_file(argv[1]);
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

  address_file.close();

  // Exibe estatísticas ao final da execução
  display_statistics(outfile);

  // Fecha o arquivo de saída
  outfile.close();

  // Libera a memória alocada
  for (int i = 0; i < NUM_FRAMES; i++)
    delete[] PhysicalMemory[i];
  delete[] PhysicalMemory;
  return 0;
}
