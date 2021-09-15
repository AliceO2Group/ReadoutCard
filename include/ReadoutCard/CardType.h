// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CardType.h
/// \brief Definition of the CardType enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_CARDTYPE_H_
#define O2_READOUTCARD_INCLUDE_CARDTYPE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>

namespace o2
{
namespace roc
{

/// Namespace for enum describing a RORC card type, and supporting functions
struct CardType {
  enum type {
    Unknown, ///< Unknown card type
    Crorc,   ///< C-RORC card type
    Cru,     ///< CRU card type
  };

  /// Converts a CardType to a string
  static std::string toString(const CardType::type& type);

  /// Converts a string to a CardType
  static CardType::type fromString(const std::string& string);
};

/// Namespace for type tags that refer to CardType enum values. Provided for use in templates.
namespace CardTypeTag
{
constexpr struct CrorcTag_ {
  static constexpr auto type = CardType::Crorc;
} CrorcTag = {};
constexpr struct CruTag_ {
  static constexpr auto type = CardType::Cru;
} CruTag = {};
constexpr struct UnknownTag_ {
  static constexpr auto type = CardType::Unknown;
} UnknownTag = {};

/// Checks if the given tag represents a valid card type.
/// This means the type needs to be a CrorcTag_ or CruTag_. NOT UnknownTag_ or anything else.
template <class Tag>
constexpr bool isValidTag()
{
  // To make the comparison simpler, we strip Tag of cv-qualifiers and references
  using tag = typename std::decay<Tag>::type;
  return std::is_same<tag, CardTypeTag::CrorcTag_>::value ||
         std::is_same<tag, CardTypeTag::CruTag_>::value;
}

/// Checks if the given tag represents a valid card type
/// Overload that deduces the type from the parameter; see non-parameterized version for more details.
template <class Tag>
constexpr bool isValidTag(Tag)
{
  return isValidTag<Tag>();
}

/// Checks if the given tag represents a non-dummy card type.
template <class Tag>
constexpr bool isNonDummyTag()
{
  // To make the comparison simpler, we strip Tag of cv-qualifiers and references
  using tag = typename std::decay<Tag>::type;
  return std::is_same<tag, CardTypeTag::CrorcTag_>::value ||
         std::is_same<tag, CardTypeTag::CruTag_>::value;
}

static_assert(isValidTag<CrorcTag_>() && isValidTag(CrorcTag), "");
static_assert(isValidTag<CruTag_>() && isValidTag(CruTag), "");
static_assert(!isValidTag<UnknownTag_>() && !isValidTag(UnknownTag), "");
static_assert(!isValidTag<int>(), "");

} // namespace CardTypeTag

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CARDTYPE_H_
