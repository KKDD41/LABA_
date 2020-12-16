#include "cache_emulator.h"

#include <cassert>

namespace cache {

// FINDING LINE IN CACHE

bool CacheEmulator::FindLine(uint64_t line_number_in_memory, CacheLine** line) {
  uint64_t bank_num = line_number_in_memory % (number_of_banks_);
  uint64_t line_pos_to_change = bank_num * associativity_;
  uint64_t index = bank_num * associativity_;
  for (; index < bank_num * associativity_ + associativity_; index++) {
    if (cache_data_.at(index).line_number_ == line_number_in_memory) {
      cache_hit_++;
      (*line) = &cache_data_.at(index);
      return true;
    }
    if (replacement_policy_ == kLeastRecentlyUsed &&
        cache_data_.at(line_pos_to_change).last_time_ >
            cache_data_.at(index).last_time_) {
      line_pos_to_change = index;
    }
  }
  if (replacement_policy_ == kRandom) {
    line_pos_to_change = bank_num * associativity_
                         + random_generator_->Generate();
  }
  (*line) = &cache_data_.at(line_pos_to_change);
  cache_miss_++;
  return false;
}

// PRIVATE LINE METHODS

CacheEmulator::CacheLine* CacheEmulator::ReadLine(uint64_t cache_line_number) {
  CacheLine* line;
  if (FindLine(cache_line_number, &line)) {
    line->last_time_ = ++time_;
    return line;
  }
  if (write_policy_ == CacheWritePolicy::kWriteBack && line->not_in_memory_) {
    main_memory_->WriteLine(line->line_number_, line->line_bytes_);
  }
  line->line_number_ = cache_line_number;
  line->line_bytes_ = main_memory_->ReadLine(cache_line_number);
  line->last_time_ = ++time_;
  line->not_in_memory_ = false;
  return line;
}

void CacheEmulator::WriteLine(CacheLine* data_line) {
  if (write_policy_ == CacheWritePolicy::kWriteBack) {
    data_line->not_in_memory_ = true;
    return;
  }
  main_memory_->WriteLine(data_line->line_number_, data_line->line_bytes_);
}

// READ & WRITE BYTES

std::vector<uint8_t> CacheEmulator::ReadBytes(uint64_t address,
                                              uint64_t count) {
  uint64_t line_number_in_memory = address / line_size_;
  uint64_t read_pos = address - line_number_in_memory * line_size_;
  std::vector<uint8_t> return_data(count);
  CacheLine* line = this->ReadLine(line_number_in_memory);
  for (size_t i = 0; i < count; i++) {
    if (read_pos == line_size_) {
      read_pos = 0;
      line = this->ReadLine(++line_number_in_memory);
    }
    return_data.at(i) = line->line_bytes_.at(read_pos);
    read_pos++;
  }
  return return_data;
}

void CacheEmulator::WriteBytes(const std::vector<uint8_t>& data,
                               uint64_t address) {
  uint64_t line_number_in_memory = address / line_size_;
  uint64_t write_pos = address - line_number_in_memory * line_size_;
  CacheLine* line = this->ReadLine(line_number_in_memory);
  for (uint8_t item : data) {
    if (write_pos == line_size_) {
      this->WriteLine(line);
      write_pos = 0;
      line = this->ReadLine(++line_number_in_memory);
    }
    line->line_bytes_.at(write_pos) = item;
    write_pos++;
  }
  this->WriteLine(line);
}

// STATISTICS PROCESSING

int CacheEmulator::GetHitsCount() const {
  return cache_hit_;
}

int CacheEmulator::GetMissesCount() const {
  return cache_miss_;
}

void CacheEmulator::ResetStatistics() {
  cache_miss_ = 0;
  cache_hit_ = 0;
}

// SETTING POLICY

void CacheEmulator::SetPolicy(CacheWritePolicy policy) {
  if (policy == write_policy_) {
    return;
  }
  write_policy_ = policy;
  if (write_policy_ == CacheWritePolicy::kWriteThrough) {
    for (auto& line : cache_data_) {
      if (line.not_in_memory_) {
        main_memory_->WriteLine(line.line_number_, line.line_bytes_);
        line.not_in_memory_ = false;
      }
    }
  }
}

void CacheEmulator::SetPolicy(CacheReplacementPolicy policy) {
  replacement_policy_ = policy;
}

}  // namespace cache
