/// \file CrorcBar.cxx
/// \brief Implementation of the CrorcBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Crorc/CrorcBar.h"

namespace AliceO2 {
namespace roc {

CrorcBar::CrorcBar(const Parameters& parameters)
    : BarInterfaceBase(parameters)
{
}

CrorcBar::~CrorcBar()
{
}

boost::optional<int32_t> CrorcBar::getSerial()
{
  return Crorc::getSerial(*(mPdaBar.get()));
}

boost::optional<std::string> CrorcBar::getFirmwareInfo()
{
  uint32_t version = mPdaBar->readRegister(Rorc::RFID);
  auto bits = [&](int lsb, int msb) { return Utilities::getBits(version, lsb, msb); };

  uint32_t reserved = bits(24, 31);
  uint32_t major = bits(20, 23);
  uint32_t minor = bits(13, 19);
  uint32_t year = bits(9, 12) + 2000;
  uint32_t month = bits(5, 8);
  uint32_t day = bits(0, 4);

  if (reserved != 0x2) {
    BOOST_THROW_EXCEPTION(CrorcException()
        << ErrorInfo::Message("Static field of version register did not equal 0x2"));
  }

  std::ostringstream stream;
  stream << major << '.' << minor << ':' << year << '-' << month << '-' << day;
  return stream.str();
}

} // namespace roc
} // namespace AliceO2
