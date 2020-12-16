#include "cache_emulator.h"
#include <gtest/gtest.h>

using hardware_interface::FakeMemoryInterface;
using hardware_interface::MemoryInterface;
using hardware_interface::FakeRandomGenerator;
using hardware_interface::RandomGeneratorInterface;

using cache::CacheEmulator;
using cache::CacheWritePolicy;
using cache::CacheReplacementPolicy;

TEST(Test_0, CompilePlease) {
  std::vector<std::vector<uint8_t>> data(8);
  for (uint64_t i = 0; i < 8; i++) {
    std::vector<uint8_t> memory_line(4);
    for (uint8_t j = 0; j < 4; j++) {
      memory_line[j] = 4 * i + j;
    }
    data[i] = memory_line;
  }
  FakeMemoryInterface mm;
  mm.Set(data);
  FakeRandomGenerator ri(1);
  std::shared_ptr<FakeMemoryInterface>(main_memory_ptr) =
      std::make_shared<FakeMemoryInterface>(mm);
  std::shared_ptr<FakeRandomGenerator>(rr_ptr) =
      std::make_shared<FakeRandomGenerator>(ri);

  CacheEmulator cache(16, 1, 4,
                      cache::CacheReplacementPolicy::kLeastRecentlyUsed,
                      cache::CacheWritePolicy::kWriteThrough,
                      main_memory_ptr, rr_ptr);
  std::vector<uint8_t> expected_line{0, 1, 2, 3};
  EXPECT_TRUE(cache.ReadBytes(0, 4) == expected_line);
  EXPECT_EQ(cache.GetMissesCount(), 1);
  EXPECT_EQ(cache.GetHitsCount(), 0);
  EXPECT_TRUE(cache.ReadBytes(0, 4) == expected_line);
  EXPECT_EQ(cache.GetHitsCount(), 1);

  // ---------------------------------------------------------
  expected_line = {2, 3, 4, 5};
  EXPECT_TRUE(cache.ReadBytes(2, 4) == expected_line);
  EXPECT_EQ(cache.GetMissesCount(), 2);
  EXPECT_EQ(cache.GetHitsCount(), 2);

  // ---------------------------------------------------------
  expected_line = {14, 15, 16, 17, 18, 19};
  EXPECT_TRUE(cache.ReadBytes(14, 6) == expected_line);
  EXPECT_EQ(cache.GetHitsCount(), 2);
  EXPECT_EQ(cache.GetMissesCount(), 4);

  // ---------------------------------------------------------
  expected_line.clear();
  expected_line.reserve(32);
  for (int i = 0; i < 32; i++) {
    expected_line.push_back(i);
  }
  EXPECT_TRUE(cache.ReadBytes(0, 32) == expected_line);
  EXPECT_EQ(cache.GetMissesCount(), 10);
  EXPECT_EQ(cache.GetHitsCount(), 4);
  cache.ResetStatistics();
  expected_line = {16, 17, 18, 19, 20, 21};
  EXPECT_TRUE(cache.ReadBytes(16, 6) == expected_line);
  EXPECT_EQ(cache.GetHitsCount(), 2);
  EXPECT_EQ(cache.GetMissesCount(), 0);
  cache.ResetStatistics();

  // ---------------------------------------------------------
  std::vector<uint8_t> data_to_write{0, 0, 0, 0, 0};
  cache.WriteBytes(data_to_write, 0);
  EXPECT_TRUE(cache.ReadBytes(0, 5) == data_to_write);
  expected_line = {0, 5, 6, 7};
  EXPECT_TRUE(cache.ReadBytes(4, 4) == expected_line);

  // ---------------------------------------------------------
  cache.SetPolicy(cache::CacheWritePolicy::kWriteBack);
  cache.WriteBytes(data_to_write, 24);
  expected_line = {0, 0, 0, 0, 0, 29};
  EXPECT_TRUE(cache.ReadBytes(24, 6) == expected_line);
  cache.SetPolicy(cache::CacheWritePolicy::kWriteThrough);
  data_to_write = {1, 1, 1, 1, 1};
  cache.WriteBytes(data_to_write, 8);
  cache.ResetStatistics();
  EXPECT_TRUE(cache.ReadBytes(24, 6) == expected_line);
  EXPECT_EQ(cache.GetMissesCount(), 2);
}

TEST(Test_1, AssociativityChecking) {
  std::vector<std::vector<uint8_t>> data(8);
  for (uint64_t i = 0; i < 8; i++) {
    std::vector<uint8_t> memory_line(4);
    for (uint8_t j = 0; j < 4; j++) {
      memory_line[j] = 4 * i + j;
    }
    data[i] = memory_line;
  }
  FakeMemoryInterface mm;
  mm.Set(data);

  FakeRandomGenerator ri(2);
  std::shared_ptr<FakeMemoryInterface>(main_memory_ptr) =
      std::make_shared<FakeMemoryInterface>(mm);
  std::shared_ptr<FakeRandomGenerator>(rr_ptr) =
      std::make_shared<FakeRandomGenerator>(ri);

  std::vector<uint8_t> expected_line;
  std::vector<uint8_t> expected_line_in_mm;
  {
    CacheEmulator cache(16, 2, 4,
              cache::CacheReplacementPolicy::kLeastRecentlyUsed,
                cache::CacheWritePolicy::kWriteBack,
                main_memory_ptr, rr_ptr);

    // -----------------------------------------------------------
    expected_line = {2, 3, 4, 5, 6, 7};
    EXPECT_TRUE(cache.ReadBytes(2, 6) == expected_line);
    EXPECT_EQ(cache.GetMissesCount(), 2);
    EXPECT_EQ(cache.GetHitsCount(), 0);
    expected_line.push_back(8);
    EXPECT_TRUE(cache.ReadBytes(2, 7) == expected_line);
    EXPECT_EQ(cache.GetMissesCount(), 3);
    EXPECT_EQ(cache.GetHitsCount(), 2);
    expected_line = {0, 1, 2, 3};
    /* cache :
                0 1 2 3 time : 3
                8 9 10 11 time : 5
                4 5 6 7 time : 4
                time : 0
    */
    EXPECT_TRUE(cache.ReadBytes(0, 4) == expected_line);
    EXPECT_EQ(cache.GetHitsCount(), 3);
    cache.ResetStatistics();

    // ------------------------------------------------------------
    expected_line = {1, 1, 1, 1};
    cache.WriteBytes(expected_line, 27);
    EXPECT_TRUE(cache.ReadBytes(27, 4) == expected_line);
    expected_line_in_mm = {24, 25, 26, 27};
    EXPECT_TRUE(mm.ReadLine(6) == expected_line_in_mm);
    /* cache:
               0 1 2 3 time : 6
               24 25 26 1 time : 9
               4 5 6 7 time : 4
               1 1 1 31 time : 10
    */
    expected_line = {24, 25, 26, 1};
    EXPECT_TRUE(cache.ReadBytes(24, 4) == expected_line);
    EXPECT_EQ(cache.GetHitsCount(), 3);

    expected_line_in_mm = {28, 29, 30, 31};
    EXPECT_TRUE(mm.ReadLine(7) == expected_line_in_mm);

    expected_line = {20, 21, 22, 23, 24, 25, 26, 1};
    EXPECT_TRUE(cache.ReadBytes(20, 8) == expected_line);
    /* cache :
                0 1 2 3 time : 6
                24 25 26 1 time : 13
                20 21 22 23 time : 12
                1 1 1 31 time : 10
    */
    EXPECT_EQ(cache.GetHitsCount(), 4);
    EXPECT_TRUE(mm.ReadLine(7) == expected_line_in_mm);
    expected_line = {0, 1, 2, 3};
    EXPECT_TRUE(cache.ReadBytes(0, 4) == expected_line);
    EXPECT_EQ(cache.GetHitsCount(), 5);
    cache.SetPolicy(cache::CacheWritePolicy::kWriteThrough);
    expected_line_in_mm = {1, 1, 1, 31};
    EXPECT_TRUE(mm.ReadLine(7) == expected_line_in_mm);
    expected_line_in_mm = {24, 25, 26, 1};
    EXPECT_TRUE(mm.ReadLine(6) == expected_line_in_mm);
  }
}

template <class T>
bool CompareReading(const T& start, CacheEmulator& cache) {
  cache.WriteObject(start, 0);
  T finish = cache.ReadObject<T>(0);
  return start == finish;
}

TEST(Test_3, ReadingObjects) {
  FakeMemoryInterface mm;
  FakeRandomGenerator ri(1);
  std::shared_ptr<FakeMemoryInterface>(main_memory_ptr) =
      std::make_shared<FakeMemoryInterface>(mm);
  std::shared_ptr<FakeRandomGenerator>(rr_ptr) =
      std::make_shared<FakeRandomGenerator>(ri);

  std::vector<std::vector<uint8_t>> data(256, {0, 0, 0, 0});
  mm.Set(data);

  CacheEmulator cache(128, 1, 4,
              cache::CacheReplacementPolicy::kLeastRecentlyUsed,
                   cache::CacheWritePolicy::kWriteThrough,
                             main_memory_ptr, rr_ptr);

  // int64_t start = 1;
  // EXPECT_TRUE(CompareReading(start, cache));
  // int start_0 = 260;
  // EXPECT_TRUE(CompareReading(start_0, cache));
  // char start_1 = 'a';
  // EXPECT_TRUE(CompareReading(start_1, cache));
}


