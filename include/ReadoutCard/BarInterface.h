/// \file BarInterface.h
/// \brief Definition of the BarInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_BARINTERFACE_H_
#define ALICEO2_INCLUDE_READOUTCARD_BARINTERFACE_H_

#include <cstdint>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2 {
namespace roc {

/// Provides access to a BAR of a readout card
/// Access to 'dangerous' registers may be restricted: UnsafeReadAccess and UnsafeWriteAccess exceptions may be thrown
class BarInterface: public virtual RegisterReadWriteInterface
{
  public:
    virtual ~BarInterface()
    {
    }

    /// Get the index of this BAR
    virtual int getIndex() const = 0;

    /// Get the size of this BAR in bytes
    virtual size_t getSize() const = 0;

    /// Returns the type of the card
    /// \return The card type
    virtual CardType::type getCardType() = 0;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_BARINTERFACE_H_
