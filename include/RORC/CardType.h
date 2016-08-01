///
/// \file CardType.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once
#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for enum describing a RORC type, and supporting functions
struct CardType
{
    enum type
    {
      UNKNOWN, CRORC, CRU, DUMMY
    };

    /// Converts a CardType to a string
    static std::string toString(const CardType::type& type);

    /// Converts a string to a CardType
    static CardType::type fromString(const std::string& string);
};

namespace CardTypeTag
{
constexpr struct DummyTag_ { static constexpr auto type = CardType::DUMMY; } DummyTag = {};
constexpr struct CrorcTag_ { static constexpr auto type = CardType::CRORC; } CrorcTag = {};
constexpr struct CruTag_ { static constexpr auto type = CardType::CRU; } CruTag = {};
constexpr struct UnknownTag_ { static constexpr auto type = CardType::UNKNOWN; } UnknownTag = {};

template<class Tag>
constexpr bool isValidTag()
{
    using tag = typename std::decay<Tag>::type;
    return std::is_same<tag, CardTypeTag::DummyTag_>::value ||
        std::is_same<tag, CardTypeTag::CrorcTag_>::value ||
        std::is_same<tag, CardTypeTag::CruTag_>::value;
}

template<class Tag>
constexpr bool isValidTag(Tag t)
{
    return isValidTag<decltype(t)>();
}

static_assert(isValidTag<DummyTag_>() && isValidTag(DummyTag), "");
static_assert(isValidTag<CrorcTag_>() && isValidTag(CrorcTag), "");
static_assert(isValidTag<CruTag_>() && isValidTag(CruTag), "");
static_assert(!isValidTag<UnknownTag_>() && !isValidTag(UnknownTag), "");

} // namespace CardTypeTag

} // namespace Rorc
} // namespace AliceO2
