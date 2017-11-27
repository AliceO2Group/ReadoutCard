/// \file SmartPointer.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_UTILITIES_SMARTPOINTER_H_
#define ALICEO2_SRC_READOUTCARD_UTILITIES_SMARTPOINTER_H_

namespace AliceO2 {
namespace roc {
namespace Utilities {

/// Convenience function to reset a smart pointer
template <typename SmartPtr, typename ...Args>
void resetSmartPtr(SmartPtr& ptr, Args&&... args)
{
  ptr.reset(new typename SmartPtr::element_type(std::forward<Args>(args)...));
}

} // namespace Util
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_UTILITIES_SMARTPOINTER_H_