/// \file ServiceNames.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) DIM service names
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_SERVICENAMES_H
#define ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_SERVICENAMES_H

#include <string>

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {

class ServiceNames
{
  public:
    ServiceNames(int serialNumber, int channelNumber)
        : serial(serialNumber), channel(channelNumber)
    {
    }

    std::string registerReadRpc() const;
    std::string registerWriteRpc() const;
    std::string publishStartCommandRpc() const;
    std::string publishStopCommandRpc() const;
    std::string scaWrite() const;
    std::string scaRead() const;
    std::string temperature() const;

  private:
    std::string format(std::string name) const;
    const int serial;
    const int channel;
};

} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_SERVICENAMES_H
