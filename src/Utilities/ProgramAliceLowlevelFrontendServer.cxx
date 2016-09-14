/// \file ProgramAliceLowlevelFrontendServer.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that starts the ALICE Lowlevel Frontend (ALF) DIM server

#include "Utilities/Program.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <dim/dis.hxx>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include "RORC/ChannelFactory.h"
#include "RORC/ChannelParameters.h"
#include "RORC/Exception.h"
#include "AliceLowlevelFrontend.h"
#include "Util.h"

namespace {
using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
namespace b = boost;
using std::cout;
using std::endl;
using llint = long long int;

/// Splits a string
std::vector<std::string> split(const std::string& string, const char* separators = ",")
{
  std::vector<std::string> split;
  b::split(split, string, boost::is_any_of(separators));
  return split;
}

struct DimServerStartStopper
{
    DimServerStartStopper()
    {
      DimServer::start("ALF");
    }

    ~DimServerStartStopper()
    {
      DimServer::stop();
    }
};

class ProgramAliceLowlevelFrontendServer: public Program
{
  public:

    virtual UtilsDescription getDescription() override
    {
      return UtilsDescription("ALF DIM Server", "ALICE low-level front-end DIM Server",
          "./rorc-alf-server --serial=12345 --channel=0");
    }

    virtual void addOptions(b::program_options::options_description& options) override
    {
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
    }

    virtual void run(const boost::program_options::variables_map& map) override
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

      if (getenv("DIM_DNS_NODE") == nullptr) {
        cout << "Using localhost as DIM DNS node\n";
        putenv("DIM_DNS_NODE=localhost");
      }

      DimServerStartStopper dimStartStopper;

      Alf::ServiceNames names(serialNumber, channelNumber);

      DimService temperatureService(names.temperature().c_str(), mTemperature);

      Alf::StringRpcServer registerReadServer(names.registerReadRpc(),
          [&channel](const std::string& parameter) { return registerRead(parameter, channel); });

      Alf::StringRpcServer registerWriteServer(names.registerWriteRpc(),
          [&channel](const std::string& parameter) { return registerWrite(parameter, channel); });

      while (!isSigInt()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        mTemperature = double((std::rand() % 100) + 400) / 10.0;
        temperatureService.updateService(mTemperature);
      }
    }

  private:

    static void assertAddress(uint64_t address)
    {
      if (address < 0x1e8 || address > 0x1fc) {
        BOOST_THROW_EXCEPTION(Exception()
            << errinfo_rorc_error_message("Address out of range"));
      }
    }

    /// RPC handler
    static std::string registerRead(const std::string& parameter, std::shared_ptr<ChannelSlaveInterface> channel)
    {
      cout << "Got read RPC: " << parameter << endl;
      auto address = b::lexical_cast<uint64_t>(parameter);
      assertAddress(address);

      uint32_t value = channel->readRegister(address / 4);

      cout << "READ   " << Common::makeRegisterString(address, value);
      return std::to_string(value);
    }

    /// RPC handler
    static const std::string registerWrite(const std::string& parameter, std::shared_ptr<ChannelSlaveInterface> channel)
    {
      cout << "Got write RPC: " << parameter << endl;
      std::vector<std::string> params = split(parameter);

      if (params.size() != 2) {
        BOOST_THROW_EXCEPTION(Exception()
            << errinfo_rorc_error_message("Write RPC call did not have 2 parameters"));
      }

      uint64_t address;
      uint32_t value;
      Util::convertAssign(params, address, value);

      assertAddress(address);

      cout << "WRITE  " << Common::makeRegisterString(address, value);

      if (address == 0x1f4) {
        // This is to the command register, we need to wait until the card is no longer busy
        while (!isSigInt() && (channel->readRegister(0x1f0 / 4) & 0x80000000)) {}
      }

      channel->writeRegister(address / 4, value);
      return "";
    }

    double mTemperature = 45;
    uint32_t mReadRegisterInt = 0;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramAliceLowlevelFrontendServer().execute(argc, argv);
}
