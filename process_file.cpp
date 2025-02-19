#include "process_file.hpp"
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>

std::deque<TLBEntry> TLB;
std::unordered_map<int, PageTableEntry> PageTable;

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

// Exibe estatísticas ao final da execução
void display_statistics(std::ofstream &outfile) {
  float page_fault_rate = (float)page_faults / total_references * 100;
  float TLB_hit_rate = (float)TLB_hits / total_references * 100;

  outfile << "\nStatistics:\n";
  outfile << "Page-Fault Rate: " << page_fault_rate << "%\n";
  outfile << "TLB Hit Rate: " << TLB_hit_rate << "%\n";
}

// Inicializa a PageTable com entradas inválidas
void initialize_page_table() {
  for (int i = 0; i < PAGE_SIZE; ++i) {
    PageTable[i] = {-1, false}; // Frame número -1 e válido como false
  }
}

// Inicializa a TLB com entradas inválidas
void initialize_TLB() {
  for (int i = 0; i < TLB_SIZE; ++i) {
    TLB.push_back({-1, -1}); // Adiciona entradas inválidas
  }
}
