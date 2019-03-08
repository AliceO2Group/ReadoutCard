// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Sca.h
/// \brief Definition of SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_SCA_H
#define ALICEO2_READOUTCARD_SCA_H

#include <string>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace roc {

struct ScaException : AliceO2::Common::Exception {};

/// Class for interfacing with the C-RORC's and CRU's Slow-Control Adapter (SCA)
class Sca
{
  public:
    struct ReadResult
    {
        uint32_t command;
        uint32_t data;
    };

    struct CommandData
    {
        uint32_t command;
        uint32_t data;
    };

    /// \param bar2 SCA is on BAR 2
    /// \param cardType Needed to get offset for SCA registers
    /// \param link Needed to get offset for SCA registers
    Sca(RegisterReadWriteInterface& bar2, CardType::type cardType, int link);

    void initialize();
    void write(uint32_t command, uint32_t data);
    void write(CommandData commandData)
    {
        write(commandData.command, commandData.data);
    }
    ReadResult read();
    ReadResult gpioRead();
    ReadResult gpioWrite(uint32_t data);

  private:
    void init();
    void gpioEnable();
    void barWrite(int index, uint32_t data);
    uint32_t barRead(int index);
    void executeCommand();
    void waitOnBusyClear();
    void checkError(uint32_t command);
    bool isChannelBusy(uint32_t command);

    /// Interface for BAR 2
    RegisterReadWriteInterface& mBar2;

    /// Offset for the registers. May differ per card
    int mOffset;
};


} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_SCA_H
