#include "parser.hpp"
#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  if (argc < 2) {
    std::cout << "usage: " << argv[0] << "<path_to_sbom>\n";
    std::exit(EXIT_FAILURE);
  }
  ts::Document doc = ts::parse_file(argv[1]);

  for (const auto &package : doc.packages) {
	  std::cout << *(package.licenseDeclared) << "\n";
  }
}
