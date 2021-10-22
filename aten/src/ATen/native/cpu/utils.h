#pragma once

#include <ATen/cpu/vec/vec.h>
#include <c10/util/llvmMathExtras.h>

namespace at {
namespace native {

namespace {

template <typename T>
inline T data_index_init(T offset) {
  return offset;
}

template <typename T, typename... Args>
inline T data_index_init(T offset, T& x, const T& X, Args&&... args) {
  offset = data_index_init(offset, std::forward<Args>(args)...);
  x = offset % X;
  return offset / X;
}

inline bool data_index_step() {
  return true;
}

template <typename T, typename... Args>
inline bool data_index_step(T& x, const T& X, Args&&... args) {
  if (data_index_step(std::forward<Args>(args)...)) {
    x = ((x + 1) == X) ? 0 : (x + 1);
    return x == 0;
  }
  return false;
}

// Helper struct for bfloat16 vectorization
// Useful when you need float as immediate dtype or accumulate dtype
using namespace vec;
struct Vec2 {
  Vectorized<float> val0, val1;
  Vec2(Vectorized<float> v0, Vectorized<float> v1) : val0(v0), val1(v1) {}
  Vec2(float v) : val0(v), val1(v) {}
  static Vec2 loadu(const BFloat16* ptr) {
    Vectorized<float> v0, v1;
    std::tie(v0, v1) = convert_bfloat16_float(Vectorized<BFloat16>::loadu(ptr));
    return {v0, v1};
  }
  void store(BFloat16* ptr) const {
    Vectorized<BFloat16> val = convert_float_bfloat16(val0, val1);
    val.store(ptr);
  }
};
inline Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.val0 + b.val0, a.val1 + b.val1}; }
inline Vec2 operator*(const Vec2& a, const Vec2& b) { return {a.val0 * b.val0, a.val1 * b.val1}; }

template <typename scalar_t> struct VectorizedType { using type = Vectorized<scalar_t>; };
template <> struct VectorizedType<BFloat16> { using type = Vec2; };
template <typename scalar_t> using VecType = typename VectorizedType<scalar_t>::type;

} // namespace

namespace utils {

template <typename T>
T CeilLog2(const T& x) {
  if (x <= 2) {
    return 1;
  }
  // Last set bit is floor(log2(x)), floor + 1 is ceil
  // except when x is an exact powers of 2, so subtract 1 first
  return static_cast<T>(llvm::findLastSet(static_cast<uint64_t>(x) - 1)) + 1;
}

} // namespace utils

} // namespace native
} // namespace at
