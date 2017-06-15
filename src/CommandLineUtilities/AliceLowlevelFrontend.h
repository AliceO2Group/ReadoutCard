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
#include "ExceptionInternal.h"

namespace Alf {

struct ServiceNames
{
    ServiceNames(int serialNumber, int channelNumber)
        : serial(serialNumber), channel(channelNumber)
    {
    }

    std::string registerReadRpc()
    {
      return format("REGISTER_READ");
    }

    std::string registerWriteRpc()
    {
      return format("REGISTER_WRITE");
    }

    std::string publishStartCommandRpc()
    {
      return format("PUBLISH_SERVICE");
    }

    std::string publishStopCommandRpc()
    {
      return format("PUBLISH_SERVICE_STOP");
    }

    std::string scaWrite()
    {
      return format("SCA_WRITE");
    }

    std::string scaRead()
    {
      return format("SCA_WRITE");
    }

    std::string temperature()
    {
      return format("TEMPERATURE");
    }

    const int serial;
    const int channel;

  private:
    std::string format(std::string name) {
      return ((boost::format("ALF/SERIAL_%d/CHANNEL_%d/%s") % serial % channel % name)).str();
    }
};

/// We use this in a few places because DIM insists on non-const char*
std::vector<char> toCharBuffer(const std::string& string, bool addTerminator = true)
{
  std::vector<char> buffer(string.begin(), string.end());
  if (addTerminator) {
    buffer.push_back('\0');
  }
  return buffer;
}

template <typename DimObject>
void setDataString(const std::string& string, DimObject& dimObject, bool addTerminator = true)
{
  auto buffer = toCharBuffer(string, addTerminator);
  dimObject.setData(buffer.data(), buffer.size());
}

constexpr size_t PREFIX_LENGTH = 8;

inline std::string successPrefix()
{
  return "success:";
}

inline std::string failPrefix()
{
  return "failure:";
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
    BOOST_THROW_EXCEPTION(AliceO2::roc::Exception()
      << AliceO2::roc::ErrorInfo::Message("string too short to contain prefix"));
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

    void setString(std::string string)
    {
      setDataString(string, getDimRpcInfo());
    }

    std::string getString()
    {
      auto string = std::string(mRpcInfo->getString());
      if (isFail(string)) {
        BOOST_THROW_EXCEPTION(AliceO2::roc::Exception() << AliceO2::roc::ErrorInfo::Message(string));
      }
      return string;
    }

    DimRpcInfo& getDimRpcInfo() const {
      return *mRpcInfo.get();
    }

  private:
    std::unique_ptr<DimRpcInfo> mRpcInfo;
};

class PublishRpc : DimRpcInfoWrapper
{
  public:
    PublishRpc(const std::string& serviceName)
        : DimRpcInfoWrapper(serviceName)
    {
    }

    void publish(std::string dnsName, double frequency, std::vector<size_t> addresses)
    {
      std::ostringstream stream;
      stream << dnsName << ';';
      for (size_t i = 0; i < addresses.size(); ++i) {
        stream << addresses[i];
        if ((i + 1) < addresses.size()) {
          stream << ',';
        }
      }
      stream << ';' << frequency;
      printf("Publish: %s\n", stream.str().c_str());
      setString(stream.str());
      getString();
    }
};

class PublishStopRpc: DimRpcInfoWrapper
{
  public:
    PublishStopRpc(const std::string &serviceName)
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
      setString(std::to_string(registerAddress));
      return boost::lexical_cast<uint32_t>(stripPrefix(getString()));
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
      auto string = std::to_string(registerAddress) + ',' + std::to_string(registerValue);
      setString(string);
      getString();
    }
};

class RegisterWriteBlockRpc: DimRpcInfoWrapper
{
  public:
    RegisterWriteBlockRpc(const std::string& serviceName)
        : DimRpcInfoWrapper(serviceName)
    {
    }

    void writeRegister(uint64_t registerAddress, uint32_t registerValue)
    {
      auto string = std::to_string(registerAddress) + ',' + std::to_string(registerValue);
      setString(string);
      getString();
    }
};

class StringRpcServer: public DimRpc
{
  public:
    using Callback = std::function<std::string(const std::string&)>;

    StringRpcServer(const std::string& serviceName, Callback callback)
        : DimRpc(serviceName.c_str(), "C", "C"), callback(callback)
    {
    }

    StringRpcServer(const StringRpcServer& b) = delete;
    StringRpcServer(StringRpcServer&& b) = delete;

  private:
    void rpcHandler() override
    {
      try {
        auto returnValue = callback(std::string(getString()));
        Alf::setDataString(Alf::makeSuccessString(returnValue), *this);
      } catch (const std::exception& e) {
        Alf::setDataString(Alf::makeFailString(e.what()), *this);
      }
    }

    Callback callback;
};

}

#endif // ALICEO2_READOUTCARD_UTILITIES_ALF_ALICELOWLEVELFRONTEND_H
