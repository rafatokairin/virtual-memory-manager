#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>

std::queue<int> frame_queue; // Tracks the order of frame allocation

const int PAGE_SIZE = 256;
const int NUM_FRAMES = 128;
const int TLB_SIZE = 16;

int page_faults = 0;      // Count of page faults
int TLB_hits = 0;         // Count of TLB hits
int total_references = 0; // Total number of address references

struct TLBEntry {
  int page_number;
  int frame_number;
};

struct PageTableEntry {
  int frame_number;
  bool valid;
};

std::deque<TLBEntry> TLB;
std::unordered_map<int, PageTableEntry> PageTable;
char PhysicalMemory[NUM_FRAMES][PAGE_SIZE];

int extract_page_number(int logical_address) {
  return (logical_address >> 8) & 0xFF;
}

int extract_offset(int logical_address) { return logical_address & 0xFF; }

int check_TLB(int page_number) {
  for (const auto &entry : TLB) {
    if (entry.page_number == page_number) {
      return entry.frame_number;
    }
  }
  return -1; // TLB miss
}

int check_page_table(int page_number) {
  if (PageTable.find(page_number) != PageTable.end() &&
      PageTable[page_number].valid) {
    return PageTable[page_number].frame_number;
  }
  return -1; // Page fault
}

int handle_page_fault(int page_number) {
  int frame_number;

  if (frame_queue.size() >= NUM_FRAMES) {
    // Physical memory is full, replace the oldest frame
    frame_number = frame_queue.front();
    frame_queue.pop();

    // Invalidate the replaced page in the page table
    for (auto &entry : PageTable) {
      if (entry.second.frame_number == frame_number) {
        entry.second.valid = false;
      }
    }
  } else {
    // Assign a new frame
    frame_number = frame_queue.size();
  }

  // Read the page from BACKING_STORE.bin
  FILE *backing_store = fopen("BACKING_STORE.bin", "rb");
  fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
  fread(PhysicalMemory[frame_number], sizeof(char), PAGE_SIZE, backing_store);
  fclose(backing_store);

  // Update the page table
  PageTable[page_number] = {frame_number, true};

  // Add the frame to the queue
  frame_queue.push(frame_number);

  return frame_number;
}

void translate_address(int logical_address) {
  total_references++; // Increment total address references

  int page_number = extract_page_number(logical_address);
  int offset = extract_offset(logical_address);

  int frame_number = check_TLB(page_number);
  if (frame_number == -1) {
    frame_number = check_page_table(page_number);
    if (frame_number == -1) {
      frame_number = handle_page_fault(page_number);
      page_faults++; // Increment page fault count
    }
    if (TLB.size() >= TLB_SIZE) {
      TLB.pop_front();
    }
    TLB.push_back({page_number, frame_number});
  } else {
    TLB_hits++; // Increment TLB hit count
  }

  int physical_address = frame_number * PAGE_SIZE + offset;
  char value = PhysicalMemory[frame_number][offset];

  std::cout << "Logical Address: " << logical_address
            << " Physical Address: " << physical_address
            << " Value: " << (int)value << std::endl;
}

void display_statistics() {
  float page_fault_rate = (float)page_faults / total_references * 100;
  float TLB_hit_rate = (float)TLB_hits / total_references * 100;

  std::cout << "\nStatistics:\n";
  std::cout << "Page-Fault Rate: " << page_fault_rate << "%\n";
  std::cout << "TLB Hit Rate: " << TLB_hit_rate << "%\n";
  std::cout << "Page Fault: " << page_faults << "\n";
  std::cout << "TLB Hits: " << TLB_hits << "\n";
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: ./vmm addresses.txt" << std::endl;
    return 1;
  }

  std::ifstream address_file(argv[1]);
  int logical_address;
  while (address_file >> logical_address) {
    translate_address(logical_address);
  }

  address_file.close();

  display_statistics();

  return 0;
}
