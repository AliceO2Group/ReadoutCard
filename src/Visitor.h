/// \file Visitor.h
/// \brief Definition of class for creating lambda-based visitors for boost::variant
///
/// Note: this was copied from the Configuration library. Should be merged at some point into Common.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef SRC_RORC_VISITOR_H_
#define SRC_RORC_VISITOR_H_

#include "VisitorImplementation.h"
#include <boost/variant/apply_visitor.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Visitor {

/// Creates a boost::variant visitor with functions.
///
/// \tparam ReturnType Return type of the visitor.
/// \tparam Types of the lambda functions.
/// \param functions Functions. One for each type that can be visited.
template<typename ReturnType, typename ... Functions>
Implementation::Visitor<ReturnType, Functions...> make(Functions ... functions)
{
  return {functions...};
}

/// Convenience function to create a boost::variant visitor with functions and immediately apply it.
///
/// \tparam ReturnType Return type of the visitor.
/// \tparam Variant Type of the variant.
/// \tparam Functions Types of the lambda functions.
/// \param variant The variant to apply the visitor to.
/// \param functions Functions. One for each type that can be visited.
template<typename ReturnType = void, typename Variant, typename ... Functions>
ReturnType apply(const Variant& variant, Functions ... functions)
{
  return boost::apply_visitor(Visitor::make<ReturnType>(functions...), variant);
}

} // namespace Visitor
} // namespace Rorc
} // namespace AliceO2

#endif
