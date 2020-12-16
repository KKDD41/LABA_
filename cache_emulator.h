#ifndef CACHE_EMULATOR_H_
#define CACHE_EMULATOR_H_

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>

#ifdef IGNORE_SOLUTION_MAIN
#include "hardware_interface.h"
#else
#include "basic_hardware_interface.h"
#endif


namespace cache {

enum CacheReplacementPolicy {
  kLeastRecentlyUsed,
  kRandom
};

enum CacheWritePolicy {
  kWriteThrough,
  kWriteBack
};

class CacheEmulator {
 public:
  CacheEmulator(
      int cache_size, int associativity, int line_size,
      CacheReplacementPolicy replacement_policy,
      CacheWritePolicy write_policy,
      std::shared_ptr<hardware_interface::MemoryInterface> main_memory_emulator,
      std::shared_ptr<hardware_interface::RandomGeneratorInterface>
          random_generator) :
      cache_size_(cache_size),
      associativity_(associativity),
      line_size_(line_size),
      replacement_policy_(replacement_policy),
      write_policy_(write_policy),
      main_memory_(std::move(main_memory_emulator)),
      random_generator_(std::move(random_generator)) {
    time_ = 0;
    number_of_banks_ = cache_size / associativity / line_size;
    cache_hit_ = 0;
    cache_miss_ = 0;
    cache_data_.assign(cache_size_ / line_size, CacheLine());
  }
  ~CacheEmulator() {
    if (write_policy_ == CacheWritePolicy::kWriteBack) {
      for (auto& line : cache_data_) {
        if (line.not_in_memory_) {
          main_memory_->WriteLine(line.line_number_, line.line_bytes_);
        }
        line.not_in_memory_ = false;
        line.line_number_ = UINT64_MAX;
        line.line_bytes_ = {};
        line.last_time_ = 0;
      }
    }
  }

  // READING & WRITING BYTES FROM MAIN MEMORY
  std::vector<uint8_t> ReadBytes(uint64_t address, uint64_t count);
  void WriteBytes(const std::vector<uint8_t>& data, uint64_t address);

  // GETTERS
  int GetHitsCount() const;
  int GetMissesCount() const;
  void ResetStatistics();


  // SETTING POLICY
  void SetPolicy(CacheReplacementPolicy);
  void SetPolicy(CacheWritePolicy);

  // READING & WRITING OBJECT

  template<typename T>
  T ReadObject(uint64_t address) {
    std::vector<uint8_t> obj_to_read = this->ReadBytes(address, sizeof(T));
    return *(reinterpret_cast<T*>(obj_to_read.data()));
  }

  template<typename T>
  void WriteObject(const T& value, uint64_t address) {
    std::vector<uint8_t> obj_to_write(reinterpret_cast<const uint8_t*>(&value),
                                      reinterpret_cast<
                                      const uint8_t*>(&value) + sizeof(T));
    this->WriteBytes(obj_to_write, address);
  }
  void print() {
    for (auto& line : cache_data_) {
      for (auto i : line.line_bytes_) {
        std::cout << unsigned (i) << " ";
      }
      std::cout << std::endl;
    }
  }

 private:
  uint32_t cache_size_{0};
  uint8_t associativity_{0};
  uint16_t line_size_{0};
  uint32_t number_of_banks_{0};

  CacheReplacementPolicy replacement_policy_;
  CacheWritePolicy write_policy_;

  uint64_t cache_hit_{0};
  uint64_t cache_miss_{0};

  std::shared_ptr<hardware_interface::MemoryInterface> main_memory_{nullptr};
  std::shared_ptr<hardware_interface::RandomGeneratorInterface>
      random_generator_{nullptr};
  struct CacheLine {
    std::vector<uint8_t> line_bytes_{};
    uint64_t line_number_{UINT64_MAX};
    int64_t last_time_{0};
    bool not_in_memory_{false};
  };
  std::vector<CacheLine> cache_data_{};

  uint64_t time_{0};

  bool FindLine(uint64_t cache_line_number, CacheLine** line);
  CacheLine* ReadLine(uint64_t address);
  void WriteLine(CacheLine* data_line);
};

}  // namespace cache

#endif  // CACHE_EMULATOR_H_
