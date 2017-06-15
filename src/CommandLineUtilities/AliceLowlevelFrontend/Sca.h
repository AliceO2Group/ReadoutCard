/// \file Sca.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_SCA_H
#define ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_SCA_H

#include <string>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {


class Sca
{
  public:
    struct WriteCommand
    {
        uint8_t command;
        uint8_t transaction;
        uint8_t channel;
    };

    struct ReadResult
    {
        uint32_t data;
        uint32_t command;
        uint32_t time;
    };

    Sca(RegisterReadWriteInterface& bar2, CardType::type cardType);

    void init();
    void write(uint32_t command, uint32_t data);
    ReadResult read();
    ReadResult gpioWrite(uint32_t data);

  private:
    void barWrite(int index, uint32_t data);
    uint32_t barRead(int index);

    RegisterReadWriteInterface& bar2;
    int offset;
};


} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_SCA_H
