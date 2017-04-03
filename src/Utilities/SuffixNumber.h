#include <istream>
#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <ExceptionInternal.h>

#ifndef SRC_UTILITIES_SUFFIXNUMBER_H_
#define SRC_UTILITIES_SUFFIXNUMBER_H_

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

namespace _SuffixNumberTable {
const std::vector<std::pair<const char*, const size_t>>& get();
} // namespace _SuffixNumberTable

/// Number with SI suffix. Only been tested with integer types, but floats might work.
/// Supported suffixes:
/// - Power of 10: k, M, G, T, P, E
/// - Power of 2: Ki, Mi, Gi, Ti, Pi, Ei
template <typename Number>
class SuffixNumber
{
  public:
    using NumberType = Number;

    SuffixNumber() : mNumber(0)
    {
    }

    SuffixNumber(Number number) : mNumber(number)
    {
    }

    SuffixNumber(std::string input) : mNumber(parse(input))
    {
    }

    Number getNumber() const
    {
      return mNumber;
    }

    void setNumber(std::string input)
    {
      mNumber = parse(input);
    }

    void setNumber(Number number)
    {
      mNumber = number;
    }

  private:
    Number parse(std::string input) const
    {
      // Find the non-numeric suffix
      auto pos = input.find_first_not_of(".0123456789");

      // Convert numeric part
      auto numberString = input.substr(0, pos);
      Number number;
      if (!boost::conversion::try_lexical_convert<Number>(numberString, number)) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not convert number")
            << ErrorInfo::String(numberString));
      }

      if (pos == std::string::npos) {
        // There's no unit
        return number;
      }

      // Find unit and multiply number with it
      auto unitString = input.substr(pos);
      for (const auto& unit : _SuffixNumberTable::get()) {
        if (unitString == unit.first) {
          // We found the right unit, multiply it with the number
          Number a = number;
          Number b = unit.second;
          Number multiplied = a * b;
          if (std::is_integral<Number>::value) {
            // Check for overflow for integers
            if ((a != 0) && ((multiplied / a) != b)) {
              BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Number too large for representation")
                << ErrorInfo::String(input));
            }
          }
          return multiplied;
        }
      }
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Unrecognized unit") << ErrorInfo::Suffix(unitString));
    }

    Number mNumber;
};

template <typename T>
std::istream& operator>>(std::istream &stream, AliceO2::Rorc::Utilities::SuffixNumber<T>& suffixNumber)
{
  std::string string;
  stream >> string;
  suffixNumber.setNumber(string);
  return stream;
}

} // namespace Utilities
} // namespace Rorc
} // namespace AliceO2

#endif // SRC_UTILITIES_SUFFIXNUMBER_H_
