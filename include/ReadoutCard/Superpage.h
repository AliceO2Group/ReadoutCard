
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
/// \file Superpage.h
/// \brief Definition of the Superpage struct
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_SUPERPAGE_H_
#define O2_READOUTCARD_INCLUDE_SUPERPAGE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <cstddef>

namespace o2
{
namespace roc
{

/// Simple struct for holding basic info about a superpage
struct Superpage {
 public:
  Superpage() = default;

  Superpage(size_t offset, size_t size, void* userData = nullptr)
    : mOffset(offset), mSize(size), mUserData(userData)
  {
  }

  /// Returns true if the superpage is ready, meaning the transfer is complete. This does not necessarily mean the
  /// superpage is filled.
  bool isReady() const
  {
    return mReady;
  }

  /// Returns true if the superpage is completely filled
  bool isFilled() const
  {
    return mReceived == getSize();
  }

  /// Offset from the start of the DMA buffer to the start of the superpage.
  size_t getOffset() const
  {
    return mOffset;
  }

  /// Size of the superpage in bytes
  size_t getSize() const
  {
    return mSize;
  }

  /// Size of the received data in bytes
  size_t getReceived() const
  {
    return mReceived;
  }

  /// Get the user data pointer
  void* getUserData() const
  {
    return mUserData;
  }

  /// Set the ready flag
  void setReady(bool ready)
  {
    mReady = ready;
  }

  /// Set the size of the received data in bytes
  void setReceived(size_t received)
  {
    mReceived = received;
  }

  /// Set the offset from the start of the DMA buffer to the start of the superpage
  void setOffset(size_t offset)
  {
    mOffset = offset;
  }

  /// Set the size of the Superpage in bytes
  void setSize(size_t size)
  {
    mSize = size;
  }

  /// Get the user data pointer
  void setUserData(void* userData)
  {
    mUserData = userData;
  }

 private:
  size_t mOffset = 0;        ///< Offset from the start of the DMA buffer to the start of the superpage
  size_t mSize = 0;          ///< Size of the superpage in bytes
  void* mUserData = nullptr; ///< Pointer that users can use for whatever, e.g. to associate data with the superpage
  size_t mReceived = 0;      ///< Size of the received data in bytes
  bool mReady = false;       ///< Indicates this superpage is ready
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_SUPERPAGE_H_
