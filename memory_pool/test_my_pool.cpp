#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

using namespace std;

class MemoryPool{
  public:
    void* ptr_;
    void* sp_;
    size_t size_;
    char* start_;

    MemoryPool(size_t bytes){
      size_ = bytes;

      ptr_ = mmap(nullptr,size_,PROT_READ | PROT_WRITE,
                  MAP_ANONYMOUS | MAP_PRIVATE,-1, 0);

      if(ptr_ == MAP_FAILED){
        throw std::runtime_error("mmap failed");
      }

      sp_ = ptr_;
      start_ = static_cast<char*>(ptr_) + size_;
    }

    char* allocate(size_t bytes){
      if(start_ - bytes < static_cast<char*>(ptr_)){
        throw std::runtime_error("bad allocate");
      }

      start_ -= bytes;

      char* rtrn_ptr = start_;
      return rtrn_ptr;
    }

    ~MemoryPool() {
        if (ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
        }
    }
};

static void get_usage(struct rusage& usage) {
  if (getrusage(RUSAGE_SELF, &usage)) {
    perror("Cannot get usage");
    exit(EXIT_SUCCESS);
  }
}

struct Node {
  Node* next;
  unsigned node_id;
};
static inline Node* create_list_in_my_mem(unsigned n, MemoryPool& pool) {
  Node* list = nullptr;

  for (unsigned i = 0; i < n; i++){
    void* mem = pool.allocate(sizeof(Node));
    list = new (mem) Node{list, i};
  }

  return list;
}
static inline Node* create_list(unsigned n) {
  Node* list = nullptr;
  for (unsigned i = 0; i < n; i++)
    list = new Node({list, i});
  return list;
}

static inline void delete_list(Node* list) {
  while (list) {
    Node* node = list;
    list = list->next;
    delete node;
  }
}

static inline void test(unsigned n) {
  struct rusage start, finish;
  get_usage(start);
  delete_list(create_list(n));
  get_usage(finish);

  struct timeval diff;
  timersub(&finish.ru_utime, &start.ru_utime, &diff);
  uint64_t time_used = diff.tv_sec * 1000000 + diff.tv_usec;
  cout << "Time used: " << time_used << " usec\n";

  uint64_t mem_used = (finish.ru_maxrss - start.ru_maxrss) * 1024;
  cout << "Memory used: " << mem_used << " bytes\n";

  auto mem_required = n * sizeof(Node);
  auto overhead = (mem_used - mem_required) * double(100) / mem_used;
  cout << "Overhead: " << std::fixed << std::setw(4) << std::setprecision(1)
       << overhead << "%\n";
}
static inline void test2(unsigned n) {
  struct rusage start, finish;
  
  
  get_usage(start);
  {
    MemoryPool pool = MemoryPool((n) * sizeof(Node));
    create_list_in_my_mem(n, pool);
  }
  get_usage(finish);

  struct timeval diff;
  timersub(&finish.ru_utime, &start.ru_utime, &diff);
  uint64_t time_used = diff.tv_sec * 1000000 + diff.tv_usec;
  cout << "Time used: " << time_used << " usec\n";

  uint64_t mem_used = (finish.ru_maxrss- start.ru_maxrss) * 1024;
  cout << "Memory used: " << mem_used << " bytes\n";

  auto mem_required = n * sizeof(Node);
  auto overhead = (mem_used - mem_required) * double(100) / mem_used;
  cout << "Overhead: " << std::fixed << std::setw(4) << std::setprecision(1)
       << overhead << "%\n";
}

int main(const int argc, const char* argv[]) {
  //test(655360);
  test2(655360);
  return EXIT_SUCCESS;
}
Time used: 44626 usec
Memory used: 20840448 bytes
Overhead: 49.7%

Time used: 9740 usec
Memory used: 10485760 bytes
Overhead:  0.0%