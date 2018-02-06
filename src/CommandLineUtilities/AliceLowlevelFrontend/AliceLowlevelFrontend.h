/// \file AliceLowlevelFrontend.h
/// \brief Definition of ALICE Lowlevel Frontend (ALF) & related DIM items
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_H
#define ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_H

#include <string>
#include <functional>
#include <dim/dim.hxx>
#include <dim/dis.hxx>
#include <dim/dic.hxx>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include "AlfException.h"
#include "Sca.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {

/// Length of the success/failure prefix that's returned in RPC calls
constexpr size_t PREFIX_LENGTH = 8;

/// Converts a 32-bit hex number string (possibly with 0x prefix)
inline uint32_t convertHexString(const std::string& string)
{
  uint64_t n = std::stoul(string, nullptr, 16);
  if (n > std::numeric_limits<uint32_t>::max()) {
    BOOST_THROW_EXCEPTION(std::out_of_range("Parameter does not fit in 32-bit unsigned int"));
  }
  return n;
}

/// We use this in a few places because DIM insists on non-const char*
inline std::vector<char> toCharBuffer(const std::string& string, bool addTerminator = true)
{
  std::vector<char> buffer(string.begin(), string.end());
  if (addTerminator) {
    buffer.push_back('\0');
  }
  return buffer;
}

template <typename DimObject>
inline void setDataString(const std::string& string, DimObject& dimObject, bool addTerminator = true)
{
  auto buffer = toCharBuffer(string, addTerminator);
  dimObject.setData(buffer.data(), buffer.size());
}

template <typename DimObject>
inline void setDataBuffer(std::vector<char>& buffer, DimObject& dimObject)
{
  dimObject.setData(buffer.data(), buffer.size());
}

inline std::string argumentSeparator()
{
  return "\n";
}

inline std::string scaPairSeparator()
{
  return ",";
}

inline std::string successPrefix()
{
  return "success" + argumentSeparator();
}

inline std::string failPrefix()
{
  return "failure" + argumentSeparator();
}

inline std::string makeSuccessString(const std::string& string)
{
  return successPrefix() + string;
}

inline std::string makeFailString(const std::string& string)
{
  return failPrefix() + string;
}

inline bool isSuccess(const std::string& string)
{
  return boost::starts_with(string, successPrefix());
}

inline bool isFail(const std::string& string)
{
  return boost::starts_with(string, failPrefix());
}

inline std::string stripPrefix(const std::string& string)
{
  if (string.length() < PREFIX_LENGTH) {
    printf("len=%lu  str=%s\n", string.length(), string.c_str());
    BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("string too short to contain prefix"));
  }
  return string.substr(PREFIX_LENGTH);
}

class DimRpcInfoWrapper
{
  public:
    DimRpcInfoWrapper(const std::string& serviceName)
      : mRpcInfo(std::make_unique<DimRpcInfo>(serviceName.c_str(), toCharBuffer("").data()))
    {
    }

    void setString(const std::string& string)
    {
      setDataString(string, getDimRpcInfo());
    }

    std::string getString()
    {
      auto string = std::string(mRpcInfo->getString());
      if (isFail(string)) {
        BOOST_THROW_EXCEPTION(
          AlfException() << ErrorInfo::Message("ALF server failure: " + string));
      }
      return string;
    }

    template <typename T>
    std::vector<T> getBlob()
    {
      auto data = reinterpret_cast<T*>(mRpcInfo->getData());
      auto size = mRpcInfo->getSize();
      std::vector<T> buffer(data, data + (size / sizeof(T)));
      return buffer;
    }

    DimRpcInfo& getDimRpcInfo() const {
      return *mRpcInfo.get();
    }

  private:
    std::unique_ptr<DimRpcInfo> mRpcInfo;
};

class PublishRegistersStartRpc : DimRpcInfoWrapper
{
  public:
    PublishRegistersStartRpc(const std::string& serviceName)
        : DimRpcInfoWrapper(serviceName)
    {
    }

    void publish(std::string dnsName, double interval, std::vector<size_t> addresses)
    {
      std::ostringstream stream;
      auto sep = argumentSeparator();
      stream << dnsName << sep << interval;
      for (size_t i = 0; i < addresses.size(); ++i) {
        stream << sep << addresses[i];
      }
      printf("Publish: %s\n", stream.str().c_str());
      setString(stream.str());
      getString();
    }
};

class PublishScaSequenceStartRpc : DimRpcInfoWrapper
{
  public:
    PublishScaSequenceStartRpc(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    void publish(std::string dnsName, double interval, const std::vector<Sca::CommandData>& commandDataPairs)
    {
      std::ostringstream stream;
      auto sep = argumentSeparator();
      stream << dnsName << sep << interval;
      for (size_t i = 0; i < commandDataPairs.size(); ++i) {
        stream << sep << commandDataPairs[i].command << scaPairSeparator() << commandDataPairs[i].data;
      }
      printf("Publish SCA: %s\n", stream.str().c_str());
      setString(stream.str());
      getString();
    }
};

class PublishRegistersStopRpc: DimRpcInfoWrapper
{
  public:
    PublishRegistersStopRpc(const std::string &serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    void stop(std::string dnsName)
    {
      setString(dnsName);
      getString();
    }
};

class PublishScaSequenceStopRpc: DimRpcInfoWrapper
{
  public:
    PublishScaSequenceStopRpc(const std::string &serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    void stop(std::string dnsName)
    {
      setString(dnsName);
      getString();
    }
};

class RegisterReadRpc: DimRpcInfoWrapper
{
  public:
    RegisterReadRpc(const std::string& serviceName)
        : DimRpcInfoWrapper(serviceName)
    {
    }

    uint32_t readRegister(uint64_t registerAddress)
    {
      setString((boost::format("0x%x") % registerAddress).str());
      return convertHexString(stripPrefix(getString()));
    }
};

class RegisterWriteRpc: DimRpcInfoWrapper
{
  public:
    RegisterWriteRpc(const std::string& serviceName)
        : DimRpcInfoWrapper(serviceName)
    {
    }

    void writeRegister(uint64_t registerAddress, uint32_t registerValue)
    {
      setString((boost::format("0x%x%s0x%x") % registerAddress % argumentSeparator() % registerValue).str());
      getString();
    }
};

class ScaReadRpc: DimRpcInfoWrapper
{
  public:
    ScaReadRpc(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    std::string read()
    {
      setString("");
      return stripPrefix(getString());
    }
};

class ScaWriteRpc: DimRpcInfoWrapper
{
  public:
    ScaWriteRpc(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    std::string write(uint32_t command, uint32_t data)
    {
      setString((boost::format("0x%x%s0x%x") % command % scaPairSeparator() % data).str());
      return stripPrefix(getString());
    }
};

class ScaGpioWriteRpc: DimRpcInfoWrapper
{
  public:
    ScaGpioWriteRpc(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    std::string write(uint32_t data)
    {
      setString((boost::format("0x%x") % data).str());
      return stripPrefix(getString());
    }
};

class ScaGpioReadRpc: DimRpcInfoWrapper
{
  public:
    ScaGpioReadRpc(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    std::string read()
    {
      setString("");
      return stripPrefix(getString());
    }
};

class ScaWriteSequence: DimRpcInfoWrapper
{
  public:
    ScaWriteSequence(const std::string& serviceName)
      : DimRpcInfoWrapper(serviceName)
    {
    }

    std::string write(const std::string& buffer)
    {
      setString(buffer);
      return getString();
    }

    std::string write(const std::vector<std::pair<uint32_t, uint32_t>>& sequence)
    {
      std::stringstream buffer;
      for (size_t i = 0; i < sequence.size(); ++i) {
        buffer << sequence[i].first << scaPairSeparator() << sequence[i].second;
        if (i + 1 < sequence.size()) {
          buffer << argumentSeparator();
        }
      }
      return write(buffer.str());
    }
};


} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_H
