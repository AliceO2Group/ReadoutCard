/// \file SmartPointer.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

namespace AliceO2 {
namespace Rorc {
namespace Utilities {

/// Convenience function to reset a smart pointer
template <typename SmartPtr, typename ...Args>
void resetSmartPtr(SmartPtr& ptr, Args&&... args)
{
  ptr.reset(new typename SmartPtr::element_type(std::forward<Args>(args)...));
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
