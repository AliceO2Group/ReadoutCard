///
/// \file ReadyFifoWrapper.cxx
/// \author Pascal Boeschoten
///

#include "ReadyFifoWrapper.h"

namespace AliceO2 {
namespace Rorc {

ReadyFifoWrapper::ReadyFifoWrapper(void* userAddress, void* deviceAddress, int length)
    : userAddress(static_cast<FifoEntry*>(userAddress)), deviceAddress(deviceAddress), length(length), readIndex(0), writeIndex(
        0)
{
}

void ReadyFifoWrapper::resetAll()
{
  for (int i = 0; i < length; ++i ) {
    userAddress[i].reset();
  }
}

void ReadyFifoWrapper::advanceWriteIndex()
{
  writeIndex = (writeIndex + 1) % length;
}

void ReadyFifoWrapper::advanceReadIndex()
{
  readIndex = (readIndex + 1) % length;
}

ReadyFifoWrapper::FifoEntry& ReadyFifoWrapper::getWriteEntry()
{
  return userAddress[writeIndex];
}

ReadyFifoWrapper::FifoEntry& ReadyFifoWrapper::getReadEntry()
{
  return userAddress[readIndex];
}

ReadyFifoWrapper::FifoEntry& ReadyFifoWrapper::getEntry(int i)
{
  return userAddress[i];
}

int ReadyFifoWrapper::getLength() const
{
  return length;
}

int ReadyFifoWrapper::getWriteIndex() const
{
  return writeIndex;
}

int ReadyFifoWrapper::getReadIndex() const
{
  return readIndex;
}

void* const Rorc::ReadyFifoWrapper::getUserAddress() const
{
  return userAddress;
}

void* const Rorc::ReadyFifoWrapper::getDeviceAddress() const
{
  return deviceAddress;
}

} // namespace Rorc
} // namespace AliceO2
