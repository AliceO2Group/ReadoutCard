#include <string>
#include <boost/program_options/value_semantic.hpp>
#include "Utilities/SuffixNumber.h"

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
    using ThisType = SuffixOption<Number>;
    using SuffixNumberType = Utilities::SuffixNumber<Number>;

    SuffixOption(SuffixNumberType* storeTo = nullptr) : mStoreToSuffixNumber(storeTo)
    {
    }

    SuffixOption(Number* storeTo = nullptr) : mStoreToNumber(storeTo)
    {
    }

    ThisType* required()
    {
      mRequired = true;
      return this;
    }

    ThisType* default_value(std::string defaultValue)
    {
      mHasDefault = true;
      mName = std::string("(=" + defaultValue + ")");
      mDefaultValue.setNumber(defaultValue);
      return this;
    }

    ThisType* default_value(Number defaultValue)
    {
      mHasDefault = true;
      mName = std::string("(=" + std::to_string(defaultValue) + ")");
      mDefaultValue.setNumber(defaultValue);
      return this;
    }


    virtual ~SuffixOption()
    {
    }

    virtual std::string name() const override
    {
      return mName;
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
      valueStore = SuffixNumberType(newTokens.at(0));
    }

    virtual bool apply_default(boost::any& value) const override
    {
      if (!mHasDefault) {
        return false;
      }
      value = mDefaultValue;
      return true;
    }

    virtual void notify(const boost::any& value) const override
    {
      if (mStoreToSuffixNumber) {
        *mStoreToSuffixNumber = boost::any_cast<SuffixNumberType>(value);
      }

      if (mStoreToNumber) {
        *mStoreToNumber = boost::any_cast<SuffixNumberType>(value).getNumber();
      }
    }

    static ThisType* make(SuffixNumberType* number)
    {
      return new ThisType(number);
    }

    static ThisType* make(Number* number)
    {
      return new ThisType(number);
    }

  private:
    Number* mStoreToNumber = nullptr;
    SuffixNumberType* mStoreToSuffixNumber = nullptr;
    bool mRequired = false;
    bool mHasDefault = false;
    SuffixNumberType mDefaultValue;
    std::string mName;
};

} // namespace Options
} // namespace CommandLineUtilities
} // namespace Rorc
} // namespace AliceO2

#endif // SRC_COMMANDLINEUTILITIES_SUFFIXOPTION_H_ 
