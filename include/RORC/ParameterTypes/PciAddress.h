/// \file PciAddress.h
/// \brief Definition of the PciAddress class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_INCLUDE_PCIADDRESS_H_
#define ALICEO2_RORC_INCLUDE_PCIADDRESS_H_

#include <string>

namespace AliceO2 {
namespace Rorc {

/// Data holder class for a PCI address, consisting of a bus, slot and function number
class PciAddress
{
  public:
    /// Constructs a PciAddress object using a string in "lspci" format: "[bus]:[slot].[function]".
    /// For example: "01:23.0"
    /// \param string String with lspci format
    PciAddress(const std::string& string);

    /// Constructs a PciAddress object
    /// \param bus Bus number, allowed numbers: 0 to 255 (0xff)
    /// \param slot Slot number, allowed numbers: 0 to 31 (0x1f)
    /// \param function Function number, allowed numbers: 0 to 7
    PciAddress(int bus, int slot, int function);

    /// Converts a PciAddress object to a string in "lspci" format: "[bus]:[slot].[function]".
    /// For example: "01:23.0"
    std::string toString() const;

    bool operator==(const PciAddress& other) const
    {
      return (bus == other.bus) && (slot == other.slot) && (function == other.function);
    }

    /// Gets the bus number of this address
    /// \return Integer from 0 to 255 (0xff)
    int getBus() const
    {
      return bus;
    }

    /// Gets the function number of this address
    /// \return Integer from 0 to 31 (0x1f)
    int getFunction() const
    {
      return function;
    }

    /// Gets the slot number of this address
    /// \return Integer from 0 to 7
    int getSlot() const
    {
      return slot;
    }

  private:
    int bus;
    int slot;
    int function;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_INCLUDE_RORC_H_
