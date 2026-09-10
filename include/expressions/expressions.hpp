#pragma once

#include <string>

namespace ts {

enum class Operator { And, Or, With, _max };
struct Expression {
  std::string rhs, lhs;
  Operator op;
};

Expression evaluate(const std::string &expr);

} // namespace ts
