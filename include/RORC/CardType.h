/// \file CardType.h
/// \brief Definition of the CardType enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once
#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for enum describing a RORC card type, and supporting functions
struct CardType
{
    enum type
    {
      UNKNOWN, ///< Unknown card type
      CRORC,   ///< C-RORC card type
      CRU,     ///< CRU card type
      DUMMY    ///< Dummy card type
    };

    /// Converts a CardType to a string
    static std::string toString(const CardType::type& type);

    /// Converts a string to a CardType
    static CardType::type fromString(const std::string& string);
};

/// Namespace for type tags that refer to CardType enum values. Provided for use in templates.
namespace CardTypeTag
{
constexpr struct DummyTag_ { static constexpr auto type = CardType::DUMMY; } DummyTag = {};
constexpr struct CrorcTag_ { static constexpr auto type = CardType::CRORC; } CrorcTag = {};
constexpr struct CruTag_ { static constexpr auto type = CardType::CRU; } CruTag = {};
constexpr struct UnknownTag_ { static constexpr auto type = CardType::UNKNOWN; } UnknownTag = {};

/// Checks if the given tag represents a valid card type.
/// This means the type needs to be a DummyTag_ CrorcTag_ or CruTag_. NOT UnknownTag_ or anything else.
template<class Tag>
constexpr bool isValidTag()
{
    // To make the comparison simpler, we strip Tag of cv-qualifiers and references
    using tag = typename std::decay<Tag>::type;
    return std::is_same<tag, CardTypeTag::DummyTag_>::value ||
        std::is_same<tag, CardTypeTag::CrorcTag_>::value ||
        std::is_same<tag, CardTypeTag::CruTag_>::value;
}

/// Checks if the given tag represents a valid card type
/// Overload that deduces the type from the parameter; see non-parameterized version for more details.
template<class Tag>
constexpr bool isValidTag(Tag)
{
  return isValidTag<Tag>();
}

static_assert(isValidTag<DummyTag_>() && isValidTag(DummyTag), "");
static_assert(isValidTag<CrorcTag_>() && isValidTag(CrorcTag), "");
static_assert(isValidTag<CruTag_>() && isValidTag(CruTag), "");
static_assert(!isValidTag<UnknownTag_>() && !isValidTag(UnknownTag), "");
static_assert(!isValidTag<int>(), "");

} // namespace CardTypeTag

} // namespace Rorc
} // namespace AliceO2
