
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
/// \file VisitorImplementation.h
/// \brief Definition of class for creating lambda-based visitors for boost::variant
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_VISITOR_IMPLEMENTATION_H_
#define O2_READOUTCARD_SRC_VISITOR_IMPLEMENTATION_H_

#include <boost/variant/static_visitor.hpp>

namespace o2
{
namespace roc
{
namespace Visitor
{
namespace Implementation
{

/// Class for creating lambda-based visitors for boost::variant
/// Based on: http://stackoverflow.com/questions/7870498/using-declaration-in-variadic-template/7870614#7870614
template <typename ReturnT, typename... Lambdas>
struct Visitor;

template <typename ReturnT, typename L1, typename... Lambdas>
struct Visitor<ReturnT, L1, Lambdas...> : public L1, public Visitor<ReturnT, Lambdas...> {
  using L1::operator();
  using Visitor<ReturnT, Lambdas...>::operator();
  Visitor(L1 l1, Lambdas... lambdas)
    : L1(l1), Visitor<ReturnT, Lambdas...>(lambdas...)
  {
  }
};

template <typename ReturnT, typename L1>
struct Visitor<ReturnT, L1> : public L1, public boost::static_visitor<ReturnT> {
  using L1::operator();
  Visitor(L1 l1)
    : L1(l1), boost::static_visitor<ReturnT>()
  {
  }
};

template <typename ReturnT>
struct Visitor<ReturnT> : public boost::static_visitor<ReturnT> {
  Visitor()
    : boost::static_visitor<ReturnT>()
  {
  }
};

} // namespace Implementation
} // namespace Visitor
} // namespace roc
} // namespace o2

#endif
