#include "parser.hpp"
#include "predicates.hpp"

#include <cstdlib>
#include <iostream>

TS_DECL_PREDICATE(LGPL, IsStaticallyLinked) {
  std::cout << "Testing" << std::endl;
  std::cout << "Testing" << std::endl;
  std::cout << "Testing" << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  if (argc < 2) {
    std::exit(EXIT_FAILURE);
  }
  ts::Document doc = ts::parse_file(argv[1]);

  for (const auto pkg : doc.packages) {
    ts::Registry::GetInstance().RunPredicatesForLicense(*pkg.licenseConcluded);
    std::cout << *pkg.licenseConcluded << std::endl;
  }
}

