/// \file PageAddress.h
/// \brief Definition of the PageAddress struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef SRC_PAGEADDRESS_H_
#define SRC_PAGEADDRESS_H_

namespace AliceO2 {
namespace Rorc {

/// A simple struct that holds the userspace and bus address of a page
struct PageAddress
{
    volatile void* user;
    volatile void* bus;
};

} // namespace Rorc
} // namespace AliceO2

#endif // SRC_PAGEADDRESS_H_
