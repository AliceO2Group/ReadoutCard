
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
/// \file Visitor.h
/// \brief Definition of class for creating lambda-based visitors for boost::variant
///
/// Note: this was copied from the Configuration library. Should be merged at some point into Common.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_VISITOR_H_
#define O2_READOUTCARD_SRC_VISITOR_H_

#include "VisitorImplementation.h"
#include <boost/variant/apply_visitor.hpp>

namespace o2
{
namespace roc
{
namespace Visitor
{

/// Creates a boost::variant visitor with functions.
///
/// \tparam ReturnType Return type of the visitor.
/// \tparam Types of the lambda functions.
/// \param functions Functions. One for each type that can be visited.
template <typename ReturnType, typename... Functions>
Implementation::Visitor<ReturnType, Functions...> make(Functions... functions)
{
  return { functions... };
}

/// Convenience function to create a boost::variant visitor with functions and immediately apply it.
///
/// \tparam ReturnType Return type of the visitor.
/// \tparam Variant Type of the variant.
/// \tparam Functions Types of the lambda functions.
/// \param variant The variant to apply the visitor to.
/// \param functions Functions. One for each type that can be visited.
template <typename ReturnType = void, typename Variant, typename... Functions>
ReturnType apply(const Variant& variant, Functions... functions)
{
  return boost::apply_visitor(Visitor::make<ReturnType>(functions...), variant);
}

} // namespace Visitor
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_VISITOR_H_
