#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout
        << "Usage: merge_list <file1> <file2> [fileout, def to merged.txt]"
        << std::endl;
    return 1;
  }
  std::ifstream file1{argv[1]};
  std::ifstream file2{argv[2]};
  std::ofstream fileout{argc == 4 ? argv[3] : "merged.txt"};
  std::string line1{}, line2{};
  while (std::getline(file1, line1) && std::getline(file2, line2)) {
    auto ss1 = std::stringstream{line1};
    double e1{}, v1{};
    ss1 >> e1 >> v1;
    auto ss2 = std::stringstream{line2};
    double e2{}, v2{};
    ss2 >> e2 >> v2;
    assert(e1 == e2);
    fileout << e1 << " " << (v1 * 7. + v2 * 6.) / 13. << std::endl;
  }
  return 0;
}