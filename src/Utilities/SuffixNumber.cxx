#include "SuffixNumber.h"
#include <vector>

namespace AliceO2 {
namespace Rorc {
namespace Utilities {
namespace _SuffixNumberTable {

const std::vector<std::pair<const char*, const size_t>>& get()
{
  static const std::vector<std::pair<const char*, const size_t>> units {
    {"k", 1000},
    {"M", 1000000},
    {"G", 1000000000},
    {"T", 1000000000000},
    {"P", 1000000000000000},
    {"E", 1000000000000000000},
//    {"Z", 1000000000000000000000}, // Too large
//    {"Y", 1000000000000000000000000}, // Too large
    {"Ki", 1024},
    {"Mi", 1048576},
    {"Gi", 1073741824},
    {"Ti", 1099511627776},
    {"Pi", 1125899906842624},
    {"Ei", 1152921504606846976},
//    {"Zi", 1180591620717411303424}, // Too large
//    {"Yi", 1208925819614629174706176}, // Too large
  };
  return units;
}

} // namespace _SuffixNumberTable
} // namespace Utilities
} // namespace Rorc
} // namespace AliceO2
