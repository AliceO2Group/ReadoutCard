/// \file ProgramAliceLowlevelFrontendServer.cxx
/// \brief Utility that starts the ALICE Lowlevel Frontend (ALF) DIM server
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Common/Program.h"
#include <chrono>
#include <iostream>
#include <functional>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include <dim/dis.hxx>
#include "AliceLowlevelFrontend.h"
#include "AlfException.h"
#include "folly/ProducerConsumerQueue.h"
#include "Utilities/Util.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ChannelFactory.h"
#include "Sca.h"
#include "ServiceNames.h"
#include "Visitor.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {
namespace {

namespace b = boost;

constexpr auto endm = AliceO2::InfoLogger::InfoLogger::endm;

AliceO2::InfoLogger::InfoLogger& getInfoLogger()
{
  static AliceO2::InfoLogger::InfoLogger logger;
  return logger;
}

using ChannelSharedPtr = std::shared_ptr<BarInterface>;

/// Splits a string
static std::vector<std::string> split(const std::string& string, const char* separators)
{
  std::vector<std::string> split;
  b::split(split, string, b::is_any_of(separators));
  return split;
}

/// Lexical casts a vector
template<typename T, typename U>
static std::vector<T> lexicalCastVector(const std::vector<U>& items)
{
  std::vector<T> result;
  for (const auto& i : items) {
    result.push_back(b::lexical_cast<T>(i));
  }
  return result;
}

struct ServiceDescription
{
    std::string dnsName;
    std::vector<uintptr_t> addresses;
    std::chrono::milliseconds interval;
};

/// Class that handles adding/removing publishers
class PublisherRegistry
{
  public:
    PublisherRegistry(ChannelSharedPtr channel) : mChannel(channel)
    {
    }

    /// Add service
    void add(const ServiceDescription& serviceDescription)
    {
      if (mServices.count(serviceDescription.dnsName)) {
        remove(serviceDescription.dnsName);
      }
      mServices.insert(std::make_pair(serviceDescription.dnsName, std::make_unique<Service>(serviceDescription)));
    }

    /// Remove service
    void remove(std::string dnsName)
    {
      mServices.erase(dnsName);
    }

    /// Call this in a loop
    void loop()
    {
      auto now = std::chrono::steady_clock::now();
      std::chrono::steady_clock::time_point next = now + std::chrono::seconds(1);

      for (auto& kv: mServices) {
        auto& service = *kv.second;
        if (service.nextUpdate < now) {
          service.updateRegisterValues(*mChannel.get());
          service.advanceUpdateTime();
          if (service.nextUpdate < next) {
            next = service.nextUpdate;
          }
        }
      }
      std::this_thread::sleep_until(next);
    }

  private:

    class Service
    {
      public:
        Service(const ServiceDescription& description)
        {
          mDescription = description;
          nextUpdate = std::chrono::steady_clock::now();
          registerValues.resize(mDescription.addresses.size());

          auto format = (b::format("I:%1%") % registerValues.size()).str();
          dimService = std::make_unique<DimService>(mDescription.dnsName.c_str(), format.c_str(), registerValues.data(),
            registerValues.size());

          getInfoLogger() << "Starting publisher '" << mDescription.dnsName << "' with "
            << mDescription.addresses.size() << " address(es) at interval "
            << mDescription.interval.count() << "ms" << endm;
        }

        ~Service()
        {
          getInfoLogger() << "Stopping publisher '" << mDescription.dnsName << "'" << endm;
        }

        void updateRegisterValues(RegisterReadWriteInterface& channel)
        {
          getInfoLogger() << "Updating '" << mDescription.dnsName << "':" << endm;
          for (size_t i = 0; i < mDescription.addresses.size(); ++i) {
            auto index = mDescription.addresses[i] / 4;
            auto value = channel.readRegister(index);
            getInfoLogger() << "  " << mDescription.addresses[i] << " = " << value << endm;
            registerValues.at(i) = channel.readRegister(index);
          }
          dimService->updateService();
        }

        void advanceUpdateTime()
        {
          nextUpdate = nextUpdate + mDescription.interval;
        }

        ServiceDescription mDescription;
        std::chrono::steady_clock::time_point nextUpdate;
        std::vector<uint32_t> registerValues;
        std::unique_ptr<DimService> dimService;
    };

    ChannelSharedPtr mChannel;
    std::unordered_map<std::string, std::unique_ptr<Service>> mServices;
};

struct CommandPublishStart
{
    ServiceDescription description;
};

struct CommandPublishStop
{
    std::string dnsName;
};

using CommandVariant = b::variant<CommandPublishStart, CommandPublishStop>;

class CommandQueue
{
  public:

    template <class ...Args>
    bool write(Args&&... recordArgs)
    {
      return mQueue.write(std::forward<Args>(recordArgs)...);
    }

    bool read(CommandVariant& command)
    {
      return mQueue.read(command);
    }

  private:
    folly::ProducerConsumerQueue<CommandVariant> mQueue {512};
};

class ProgramAliceLowlevelFrontendServer: public AliceO2::Common::Program
{
  public:

    virtual Description getDescription() override
    {
      return {"ALF DIM Server", "ALICE low-level front-end DIM Server", "roc-alf-server --serial=12345"};
    }

    virtual void addOptions(b::program_options::options_description& options) override
    {
      options.add_options()("serial", b::program_options::value<int>(&mSerialNumber), "Card serial number");
    }

    virtual void run(const b::program_options::variables_map&) override
    {
      // Get DIM DNS node from environment
      if (getenv(std::string("DIM_DNS_NODE").c_str()) == nullptr) {
        BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Environment variable 'DIM_DNS_NODE' not set"));
      }

      // Get card channel for register access
      auto bar0 = ChannelFactory().getBar(Parameters::makeParameters(mSerialNumber, 0));
      auto bar2 = ChannelFactory().getBar(Parameters::makeParameters(mSerialNumber, 2));

      try {
        Sca(*bar2, bar2->getCardType()).initialize();
      }
      catch (const ScaException& e) {
        getInfoLogger() << "ScaException: " << e.what() << endm;
      }

      DimServer::start("ALF");

      // Object for service DNS names
      Alf::ServiceNames names(mSerialNumber);

      auto makeServer = [&](std::string name, auto callback) {
        getInfoLogger() << "Starting service " << name << endm;
        return std::make_unique<Alf::StringRpcServer>(name, callback);
      };

      // Start RPC servers
      auto serverRead = makeServer(names.registerReadRpc(),
        [&](auto parameter){return registerRead(parameter, bar0);});
      auto serverWrite = makeServer(names.registerWriteRpc(),
        [&](auto parameter){return registerWrite(parameter, bar0);});
      auto serverPublishStart = makeServer(names.publishStartCommandRpc(),
        [&](auto parameter){return publishStartCommand(parameter, mCommandQueue);});
      auto serverPublishStop = makeServer(names.publishStopCommandRpc(),
        [&](auto parameter){return publishStopCommand(parameter, mCommandQueue);});
      auto serverScaRead = makeServer(names.scaRead(),
        [&](auto parameter){return scaRead(parameter, bar2);});
      auto serverScaWrite = makeServer(names.scaWrite(),
        [&](auto parameter){return scaWrite(parameter, bar2);});
      auto serverScaWriteSequence = makeServer(names.scaWriteSequence(),
        [&](auto parameter){return scaBlobWrite(parameter, bar2);});
      auto serverScaGpioRead = makeServer(names.scaGpioRead(),
        [&](auto parameter){return scaGpioRead(parameter, bar2);});
      auto serverScaGpioWrite = makeServer(names.scaGpioWrite(),
        [&](auto parameter){return scaGpioWrite(parameter, bar2);});

      // Start dummy temperature service
      DimService temperatureService(names.temperature().c_str(), mTemperature);

      PublisherRegistry publisherRegistry {bar0};
      while (!isSigInt())
      {
        {
          // Take care of publishing commands from the queue
          CommandVariant command;
          while (mCommandQueue.read(command)) {
            Visitor::apply(command,
                [&](const CommandPublishStart& command){
                  publisherRegistry.add(command.description);
                },
                [&](const CommandPublishStop& command){
                  publisherRegistry.remove(command.dnsName);
                }
            );
          }
        }

        publisherRegistry.loop();
        mTemperature = double((std::rand() % 100) + 400) / 10.0;
        temperatureService.updateService(mTemperature);
      }
    }

  private:
    /// Checks if the address is in range
    static void checkAddress(uint64_t address)
    {
      if (address < 0x1e8 || address > 0x1fc) {
        BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Address out of range"));
      }
    }

    /// RPC handler for register reads
    static std::string registerRead(const std::string& parameter, ChannelSharedPtr channel)
    {
      auto address = convertHexString(parameter);
      checkAddress(address);

      uint32_t value = channel->readRegister(address / 4);

      // getInfoLogger() << "READ   " << Common::makeRegisterString(address, value) << endm;

      return (b::format("0x%x") % value).str();
    }

    /// RPC handler for register writes
    static std::string registerWrite(const std::string& parameter, ChannelSharedPtr channel)
    {
      std::vector<std::string> params = split(parameter, ",");

      if (params.size() != 2) {
        BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Write RPC call did not have 2 parameters"));
      }

      auto address = convertHexString(params[0]);;
      auto value = convertHexString(params[1]);;
      checkAddress(address);

      // getInfoLogger() << "WRITE  " << Common::makeRegisterString(address, value) << endm;

      if (address == 0x1f4) {
        // This is to the command register, we need to wait until the card indicates it's not busy before sending a
        // command
        while (!isSigInt() && (channel->readRegister(0x1f0 / 4) & 0x80000000)) {
        }
      }

      channel->writeRegister(address / 4, value);
      return "";
    }

    /// RPC handler for publish commands
    static std::string publishStartCommand(const std::string& parameter, CommandQueue& commandQueue)
    {
      getInfoLogger() << "Received publish command: '" << parameter << "'" << endm;
      auto params = split(parameter, ";");

      ServiceDescription description;
      description.addresses = lexicalCastVector<uintptr_t >(split(params.at(1), ","));
      description.dnsName = params.at(0);
      description.interval = std::chrono::milliseconds(int64_t(b::lexical_cast<double>(params.at(2)) * 1000.0));

      if (!commandQueue.write(CommandPublishStart{std::move(description)})) {
        getInfoLogger() << "  command queue was full!" << endm;
      }
      return "";
    }

    /// RPC handler for publish stop commands
    static std::string publishStopCommand(const std::string& parameter, CommandQueue& commandQueue)
    {
      getInfoLogger() << "Received stop command: '" << parameter << "'" << endm;
      if (!commandQueue.write(CommandPublishStop{parameter})) {
        getInfoLogger() << "  command queue was full!" << endm;
      }
      return "";
    }

    static std::string scaRead(const std::string&, ChannelSharedPtr bar2)
    {
      getInfoLogger() << "SCA_READ" << endm;
      auto result = Sca(*bar2, bar2->getCardType()).read();
      return (b::format("0x%x,0x%x") % result.command % result.data).str();
    }

    static std::string scaWrite(const std::string& parameter, ChannelSharedPtr bar2)
    {
      getInfoLogger() << "SCA_WRITE: '" << parameter << "'" << endm;
      auto params = split(parameter, ",");
      auto command = convertHexString(params.at(0));
      auto data = convertHexString(params.at(1));
      Sca(*bar2, bar2->getCardType()).write(command, data);
      return "";
    }

    static std::string scaGpioRead(const std::string&, ChannelSharedPtr bar2)
    {
      getInfoLogger() << "SCA_GPIO_READ" << endm;
      auto result = Sca(*bar2, bar2->getCardType()).gpioRead();
      return (b::format("0x%x") % result.data).str();
    }

    static std::string scaGpioWrite(const std::string& parameter, ChannelSharedPtr bar2)
    {
      getInfoLogger() << "SCA_GPIO_WRITE: '" << parameter << "'" << endm;
      auto data = convertHexString(parameter);
      Sca(*bar2, bar2->getCardType()).gpioWrite(data);
      return "";
    }

    static std::string scaBlobWrite(const std::string& parameter, ChannelSharedPtr bar2)
    {
      getInfoLogger() << "SCA_BLOB_WRITE size=" << parameter.size() << " bytes" << endm;

      // We first split on \n to get the pairs of SCA command and SCA data
      // Since this can be an enormous list of pairs, we walk through it using the tokenizer
      using tokenizer = boost::tokenizer<boost::char_separator<char>>;
      boost::char_separator<char> sep("\n", "", boost::drop_empty_tokens); // Drop '\n' delimiters, keep none
      std::stringstream resultBuffer;
      auto sca = Sca(*bar2, bar2->getCardType());

      for (std::string token : tokenizer(parameter, sep)) {
        // Walk through the tokens, these should be the pairs (or comments).
        if (token.find('#') == 0) {
          // We have a comment, skip this token
          continue;
        } else {
          // The pairs are comma-separated, so we split them.
          std::vector<std::string> pair = split(token, ",");
          if (pair.size() != 2) {
            BOOST_THROW_EXCEPTION(
              AlfException() << ErrorInfo::Message("SCA command-data pair not formatted correctly"));
          }
          auto command = convertHexString(pair[0]);
          auto data = convertHexString(pair[1]);
          try {
            sca.write(command, data);
            auto result = sca.read();
            getInfoLogger() << (b::format("cmd=0x%x data=0x%x result=0x%x") % command % data % result.data).str()
              << endm;
            resultBuffer << std::hex << result.data << '\n';
          } catch (const ScaException &e) {
            // If an SCA error occurs, we stop executing the sequence of commands and return the results as far as we got
            // them
            break;
          }
        }
      }

      auto result = resultBuffer.str();
      if (result.size() > 0) {
        result.pop_back(); // Pop the last ';'
      }
      return result;
    }

    int mSerialNumber = 0;
    CommandQueue mCommandQueue;
    double mTemperature = 40;
};
} // Anonymous namespace
} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2

int main(int argc, char** argv)
{
  return AliceO2::roc::CommandLineUtilities::Alf::ProgramAliceLowlevelFrontendServer().execute(argc, argv);
}
