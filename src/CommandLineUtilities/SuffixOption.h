#include <string>
#include <boost/program_options/value_semantic.hpp>

#ifndef SRC_COMMANDLINEUTILITIES_SUFFIXOPTION_H_
#define SRC_COMMANDLINEUTILITIES_SUFFIXOPTION_H_

namespace AliceO2 {
namespace Rorc {
namespace CommandLineUtilities {
namespace Options {

namespace _SuffixOptionTable {
const std::vector<std::pair<const char*, const size_t>>& get();
} // namespace _SuffixOptionTable

template <typename Number>
class SuffixOption final : public boost::program_options::value_semantic
{
  public:
    using This = SuffixOption<Number>;

    SuffixOption(Number* storeTo = nullptr) : mStoreTo(storeTo)
    {
    }

    This* required()
    {
      mRequired = true;
      return this;
    }

    This* default_value(std::string defaultValue)
    {
      mDefaultValue = defaultValue;
      return this;
    }

    virtual ~SuffixOption()
    {
    }

    virtual std::string name() const override
    {
      return "SuffixOption";
    }

    virtual unsigned min_tokens() const override
    {
      return 1;
    }

    virtual unsigned max_tokens() const override
    {
      return 16;
    }

    virtual bool adjacent_tokens_only() const override
    {
      return true;
    }

    virtual bool is_composing() const override
    {
      return false;
    }

    virtual bool is_required() const override
    {
      return mRequired;
    }

    virtual void parse(boost::any& valueStore, const std::vector<std::string>& newTokens, bool utf8) const override
    {
      const auto& token = newTokens.at(0);
      // Find the non-numeric suffix
      auto pos = token.find_first_not_of("0123456789");

      if (pos == std::string::npos) {
        // We have a plain number
        valueStore = boost::lexical_cast<Number>(token);
        return;
      }

      auto numString = token.substr(0, pos);
      auto unitString = token.substr(pos);

      for (const auto& unit : _SuffixOptionTable::get()) {
        if (unitString == unit.first) {
          auto num = boost::lexical_cast<Number>(numString);
          valueStore = num * Number(unit.second);
          return;
        }
      }
    }

    virtual bool apply_default(boost::any& value) const override
    {
      if (mDefaultValue.empty()) {
        return false;
      }

      value = mDefaultValue;
      return true;
    }

    virtual void notify(const boost::any& value) const override
    {
      if (mStoreTo) {
        *mStoreTo = boost::any_cast<Number>(value);
      }
    }

  private:

    Number* mStoreTo = nullptr;
    bool mRequired = false;
    std::string mDefaultValue;
};

} // namespace Options
} // namespace CommandLineUtilities
} // namespace Rorc
} // namespace AliceO2

#endif // SRC_COMMANDLINEUTILITIES_SUFFIXOPTION_H_ 
