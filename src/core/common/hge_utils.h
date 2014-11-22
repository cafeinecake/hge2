#pragma once

#include <cfloat>
#include <cmath>

namespace hgeut {

inline bool flt_equal(float x, float y)
{
  return std::abs(x-y) <= FLT_EPSILON;
}
inline bool flt_not_equal(float x, float y)
{
  return std::abs(x-y) > FLT_EPSILON;
}

}
