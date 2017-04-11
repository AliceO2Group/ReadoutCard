/// \file VisitorImplementation.h
/// \brief Definition of class for creating lambda-based visitors for boost::variant
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_RORC_VISITOR_IMPLEMENTATION_H_
#define ALICEO2_SRC_RORC_VISITOR_IMPLEMENTATION_H_

#include <boost/variant/static_visitor.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Visitor {
namespace Implementation {

/// Class for creating lambda-based visitors for boost::variant
/// Based on: http://stackoverflow.com/questions/7870498/using-declaration-in-variadic-template/7870614#7870614
template<typename ReturnT, typename ... Lambdas>
struct Visitor;

template<typename ReturnT, typename L1, typename ... Lambdas>
struct Visitor<ReturnT, L1, Lambdas...> : public L1, public Visitor<ReturnT, Lambdas...>
{
    using L1::operator();
    using Visitor<ReturnT, Lambdas...>::operator();
    Visitor(L1 l1, Lambdas ... lambdas)
        : L1(l1), Visitor<ReturnT, Lambdas...>(lambdas...)
    {
    }
};

template<typename ReturnT, typename L1>
struct Visitor<ReturnT, L1> : public L1, public boost::static_visitor<ReturnT>
{
    using L1::operator();
    Visitor(L1 l1)
        : L1(l1), boost::static_visitor<ReturnT>()
    {
    }
};

template<typename ReturnT>
struct Visitor<ReturnT> : public boost::static_visitor<ReturnT>
{
    Visitor()
        : boost::static_visitor<ReturnT>()
    {
    }
};

} // namespace Implementation
} // namespace Visitor
} // namespace Rorc
} // namespace AliceO2

#endif
