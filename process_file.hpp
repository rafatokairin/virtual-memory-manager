#ifndef PROCESS_FILE_HPP
#define PROCESS_FILE_HPP

#include <cstdlib>
#include <deque>
#include <fstream>
#include <unordered_map>

struct TLBEntry {
  int page_number;
  int frame_number;
  bool operator==(int frame) const { return frame_number == frame; }
};

struct PageTableEntry {
  int frame_number;
  bool valid;
};

extern std::deque<TLBEntry> TLB;
extern std::unordered_map<int, PageTableEntry> PageTable;
extern int page_faults;
extern int TLB_hits;
extern int total_references;
const int PAGE_SIZE = 256;
const int TLB_SIZE = 16;

void print_page_table(std::ofstream &outfile);
void print_TLB(std::ofstream &outfile);
void display_statistics(std::ofstream &outfile);
void initialize_page_table();
void initialize_TLB();

#endif // PROCESS_FILE_HPP
