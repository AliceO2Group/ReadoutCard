#include "SuffixOption.h"
#include <vector>
#include <boost/lexical_cast.hpp>

namespace AliceO2 {
namespace Rorc {
namespace CommandLineUtilities {
namespace Options {
namespace _SuffixOptionTable {

const std::vector<std::pair<const char*, const size_t>>& get()
{
  static const std::vector<std::pair<const char*, const size_t>> units {
    {"k", 1000},
    {"M", 1000000},
    {"G", 1000000000},
    {"T", 1000000000000},
    {"P", 1000000000000000},
    {"Ki", 1024},
    {"Mi", 1048576},
    {"Gi", 1073741824},
    {"Ti", 1099511627776},
    {"Pi", 1125899906842624},
  };
  return units;
}

} // namespace _SuffixOptionTable
} // namespace Options
} // namespace CommandLineUtilities
} // namespace Rorc
} // namespace AliceO2
