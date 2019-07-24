// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file SmartPointer.h
/// \brief Definition of various useful utilities that don't really belong anywhere in particular
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_UTILITIES_SMARTPOINTER_H_
#define ALICEO2_SRC_READOUTCARD_UTILITIES_SMARTPOINTER_H_

namespace AliceO2
{
namespace roc
{
namespace Utilities
{

/// Convenience function to reset a smart pointer
template <typename SmartPtr, typename... Args>
void resetSmartPtr(SmartPtr& ptr, Args&&... args)
{
  ptr.reset(new typename SmartPtr::element_type(std::forward<Args>(args)...));
}

} // namespace Utilities
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_UTILITIES_SMARTPOINTER_H_
