/// \file ProgramAliceLowlevelFrontendServer.cxx
/// \brief Utility that starts the ALICE Lowlevel Frontend (ALF) DIM server
///
/// This file contains a bunch of classes that together form the server part of ALF.
/// It is single-threaded, because an earlier multi-threaded implementation ran into strange locking issues with DIM's
/// thread, and it was determined that it would cost less time to rewrite than to debug.
///
/// The idea is that the DIM thread calls the RPC handler functions of the ProgramAliceLowlevelFrontendServer class.
/// These handlers then, depending on the RPC:
/// a) Execute the request immediately (such as for register reads and writes)
/// b) Put a corresponding command object in a lock-free thread-safe queue (such as for publish start/stop commands),
///    the mCommandQueue. The main thread of ProgramAliceLowlevelFrontendServer periodically takes commands from this
///    queue and handles them.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Common/Program.h"
#include "CommandLineUtilities/Common.h"
#include <chrono>
#include <iostream>
#include <functional>
#include <map>
#include <thread>
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
#include "ReadoutCard/ChannelFactory.h"
#include "RocPciDevice.h"
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
using BarSharedPtr = std::shared_ptr<BarInterface>;

AliceO2::InfoLogger::InfoLogger& getLogger()
{
  static AliceO2::InfoLogger::InfoLogger logger;
  return logger;
}

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

struct LinkInfo
{
    int serial;
    int link;
};

/// Struct describing a DIM publishing service
struct ServiceDescription
{
    /// Struct for register read service
    struct Register
    {
        std::vector<uintptr_t> addresses;
    };

    /// Struct for SCA sequence service
    struct ScaSequence
    {
        std::vector<Sca::CommandData> commandDataPairs;
    };

    std::string dnsName;
    std::chrono::milliseconds interval;
    boost::variant<Register, ScaSequence> type;
    LinkInfo linkInfo;
};

/// Struct for DIM publishing service data
struct Service
{
  public:
    void advanceUpdateTime()
    {
      nextUpdate = nextUpdate + description.interval;
    }

    ServiceDescription description;
    std::chrono::steady_clock::time_point nextUpdate;
    std::vector<uint32_t> registerValues;
    std::unique_ptr<DimService> dimService;
};

///
class CommandQueue
{
  public:
    struct CommandPublishStart
    {
        ServiceDescription description;
    };

    struct CommandPublishStop
    {
        ServiceDescription description;
    };

    using CommandVariant = b::variant<CommandPublishStart, CommandPublishStop>;


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
    ProgramAliceLowlevelFrontendServer() : mCommandQueue(), mRpcServers(), mBars(), mServices()
    {
    }

    virtual Description getDescription() override
    {
      return {"ALF DIM Server", "ALICE low-level front-end DIM Server", "roc-alf-server --serial=12345"};
    }

    virtual void addOptions(b::program_options::options_description& options) override
    {
      options.add_options()("serial", b::program_options::value<int>(), "Card serial number");
    }

    virtual void run(const b::program_options::variables_map&) override
    {
      // Get DIM DNS node from environment
      if (getenv(std::string("DIM_DNS_NODE").c_str()) == nullptr) {
        BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Environment variable 'DIM_DNS_NODE' not set"));
      }

      DimServer::start("ALF");

      getLogger() << "Finding cards" << endm;

      std::vector<int> serialNumbers;

      auto cardsFound = AliceO2::roc::RocPciDevice::findSystemDevices();
      for (auto card : cardsFound) {
        int serial;
        if (auto serialMaybe = card.serialNumber) {
          serial = serialMaybe.get();
          getLogger() << "Initializing server for card " << card.pciAddress.toString() << " with serial "
            << serial << "" << endm;
        } else {
          getLogger() << "Card " << card.pciAddress.toString() << " has no serial number, skipping..." << endm;
          continue;
        }

        BarSharedPtr bar0 = ChannelFactory().getBar(Parameters::makeParameters(serial, 0));
        BarSharedPtr bar2 = ChannelFactory().getBar(Parameters::makeParameters(serial, 2));
        mBars[serial][0] = bar0;
        mBars[serial][2] = bar2;

        std::vector<int> links;
        if (card.cardType == CardType::Cru) {
          links = {0, 1, 2, 3, 4, 5, 6};
        } else if (card.cardType == CardType::Crorc) {
          links = {0};
        }

        for (auto link : links) {
          getLogger() << "Initializing link " << link << endm;
          LinkInfo linkInfo {serial, link};
          // Initialize SCA
          try {
            Sca(*bar2, bar2->getCardType(), link).initialize();
          }
          catch (const ScaException &e) {
            getLogger() << "ScaException: " << e.what() << endm;
          }

          // Object for generating DNS names
          Alf::ServiceNames names(serial, link);

          // Function to create RPC server
          auto makeServer = [&](std::string name, auto callback) {
            getLogger() << "Starting RPC server '" << name << "'" << endm;
            return std::make_unique<Alf::StringRpcServer>(name, callback);
          };

          // Start RPC servers
          // Note that we capture the bar0, bar2, and mCommandQueue objects by value, so the shared_ptrs can do their
          // job
          auto& servers = mRpcServers[serial][link];
          auto commandQueue = mCommandQueue; // Copy for lambda capture
          // Register RPCs
          servers.push_back(makeServer(names.registerReadRpc(),
            [bar0](auto parameter){
              return registerRead(parameter, bar0);}));
          servers.push_back(makeServer(names.registerWriteRpc(),
            [bar0](auto parameter){
              return registerWrite(parameter, bar0);}));
          // SCA RPCs
          servers.push_back(makeServer(names.scaRead(),
            [bar2, linkInfo](auto parameter){
              return scaRead(parameter, bar2, linkInfo);}));
          servers.push_back(makeServer(names.scaWrite(),
            [bar2, linkInfo](auto parameter){
              return scaWrite(parameter, bar2, linkInfo);}));
          servers.push_back(makeServer(names.scaWriteSequence(),
            [bar2, linkInfo](auto parameter){
              return scaBlobWrite(parameter, bar2, linkInfo);}));
          servers.push_back(makeServer(names.scaGpioRead(),
            [bar2, linkInfo](auto parameter){
              return scaGpioRead(parameter, bar2, linkInfo);}));
          servers.push_back(makeServer(names.scaGpioWrite(),
            [bar2, linkInfo](auto parameter){
              return scaGpioWrite(parameter, bar2, linkInfo);}));
          // Publish RPCs
          servers.push_back(makeServer(names.publishStartCommandRpc(),
            [commandQueue, linkInfo](auto parameter){
              return publishStartCommand(parameter, commandQueue, linkInfo);}));
          servers.push_back(makeServer(names.publishScaStartCommandRpc(),
            [commandQueue, linkInfo](auto parameter){
              return publishScaStartCommand(parameter, commandQueue, linkInfo);}));
          servers.push_back(makeServer(names.publishStopCommandRpc(),
            [commandQueue, linkInfo](auto parameter){
              return publishStopCommand(parameter, commandQueue, linkInfo);}));
        }
      }

      while (!isSigInt())
      {
        {
          // Take care of publishing commands from the queue
          CommandQueue::CommandVariant command;
          while (mCommandQueue->read(command)) {
            Visitor::apply(command,
                [&](const CommandQueue::CommandPublishStart& command){
                  serviceAdd(command.description);
                },
                [&](const CommandQueue::CommandPublishStop& command){
                  serviceRemove(command.description.dnsName);
                }
            );
          }
        }

        // Update service(s) and sleep until next update is needed

        auto now = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point next = now + std::chrono::seconds(1); // We wait a max of 1 second

        for (auto& kv: mServices) {
          auto& service = *kv.second;
          if (service.nextUpdate < now) {
            serviceUpdate(service);
            service.advanceUpdateTime();
            next = std::min(next, service.nextUpdate);
          }
        }
        std::this_thread::sleep_until(next);
      }
    }

  private:

    const std::map<int, BarSharedPtr>& getBars(int serial)
    {
      return mBars.at(serial);
    }

    /// Add service
    void serviceAdd(const ServiceDescription& serviceDescription)
    {
      if (mServices.count(serviceDescription.dnsName)) {
        // If the service is already present, remove the old one first
        serviceRemove(serviceDescription.dnsName);
      }

      auto service = std::make_unique<Service>();
      service->description = serviceDescription;
      service->nextUpdate = std::chrono::steady_clock::now();
      Visitor::apply(service->description.type,
        [&](const ServiceDescription::Register& type){
          service->registerValues.resize(type.addresses.size());

          getLogger() << "Starting publisher '" << service->description.dnsName << "' with "
            << type.addresses.size() << " address(es) at interval "
            << service->description.interval.count() << "ms" << endm;
        },
        [&](const ServiceDescription::ScaSequence& type){
          service->registerValues.resize(type.commandDataPairs.size() * 2); // Two result 32-bit integers per pair

          getLogger() << "Starting SCA publisher '" << service->description.dnsName << "' with "
            << type.commandDataPairs.size() << " commands(s) at interval "
            << service->description.interval.count() << "ms" << endm;
        }
      );

      auto format = (b::format("I:%1%") % service->registerValues.size()).str();
      service->dimService = std::make_unique<DimService>(service->description.dnsName.c_str(), format.c_str(),
        service->registerValues.data(), service->registerValues.size());

      mServices.insert(std::make_pair(serviceDescription.dnsName, std::move(service)));
    }

    /// Remove service
    void serviceRemove(std::string dnsName)
    {
      getLogger() << "Removing publisher '" << dnsName << endm;
      mServices.erase(dnsName);
    }

    /// Publish updated values
    void serviceUpdate(Service& service)
    {
      getLogger() << "Updating '" << service.description.dnsName << "':" << endm;

      Visitor::apply(service.description.type,
        [&](const ServiceDescription::Register& type){
          auto& bar0 = *(mBars.at(service.description.linkInfo.serial).at(0));
          for (size_t i = 0; i < type.addresses.size(); ++i) {
            auto index = type.addresses[i] / 4;
            auto value = bar0.readRegister(index);
            getLogger() << "  " << type.addresses[i] << " = " << value << endm;
            service.registerValues.at(i) = bar0.readRegister(index);
          }
        },
        [&](const ServiceDescription::ScaSequence& type){
          auto& bar2 = *(mBars.at(service.description.linkInfo.serial).at(2));
          std::fill(service.registerValues.begin(), service.registerValues.end(), 0); // Reset array in case of aborts
          auto sca = Sca(bar2, bar2.getCardType(), service.description.linkInfo.link);
          for (size_t i = 0; i < type.commandDataPairs.size(); ++i) {
            try {
              const auto& pair = type.commandDataPairs[i];
              sca.write(pair);
              auto result = sca.read();
              service.registerValues[i*2] = result.command;
              service.registerValues[i*2 + 1] = result.data;
            } catch (const ScaException &e) {
              // If an SCA error occurs, we stop executing the sequence of commands and set the error value
              service.registerValues[i*2] = 0xffffffff;
              service.registerValues[i*2 + 1] = 0xffffffff;
              break;
            }
          }
        }
      );

      service.dimService->updateService();
    }

    /// Checks if the address is in range
    static void checkAddress(uint64_t address)
    {
      if (address < 0x1e8 || address > 0x1fc) {
        BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Address out of range"));
      }
    }

    /// Try to add a command to the queue
    static bool tryAddToQueue(CommandQueue& queue, const CommandQueue::CommandVariant& command)
    {
      if (!queue.write(command)) {
        getLogger() << "  command queue was full!" << endm;
        return false;
      }
      return true;
    }

    /// RPC handler for register reads
    static std::string registerRead(const std::string& parameter, BarSharedPtr channel)
    {
      auto address = convertHexString(parameter);
      checkAddress(address);

      uint32_t value = channel->readRegister(address / 4);

      getLogger() << "READ   " << Common::makeRegisterString(address, value) << endm;

      return (b::format("0x%x") % value).str();
    }

    /// RPC handler for register writes
    static std::string registerWrite(const std::string& parameter, BarSharedPtr channel)
    {
      std::vector<std::string> params = split(parameter, ",");

      if (params.size() != 2) {
        BOOST_THROW_EXCEPTION(AlfException() << ErrorInfo::Message("Write RPC call did not have 2 parameters"));
      }

      auto address = convertHexString(params[0]);;
      auto value = convertHexString(params[1]);;
      checkAddress(address);

      getLogger() << "WRITE  " << Common::makeRegisterString(address, value) << endm;

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
    static std::string publishStartCommand(const std::string& parameter, std::shared_ptr<CommandQueue> queue,
      LinkInfo linkInfo)
    {
      getLogger() << "Received publish command: '" << parameter << "'" << endm;
      auto params = split(parameter, ";");

      ServiceDescription description;
      description.dnsName = params.at(0);
      description.interval = std::chrono::milliseconds(int64_t(b::lexical_cast<double>(params.at(2)) * 1000.0));
      description.type = ServiceDescription::Register{lexicalCastVector<uintptr_t >(split(params.at(1), ","))};
      description.linkInfo = linkInfo;

      tryAddToQueue(*queue, CommandQueue::CommandPublishStart{description});
      return "";
    }

    /// RPC handler for SCA publish commands
    static std::string publishScaStartCommand(const std::string& parameter, std::shared_ptr<CommandQueue> queue,
      LinkInfo linkInfo)
    {
      getLogger() << "Received SCA publish command: '" << parameter << "'" << endm;

      auto params = split(parameter, ";");

      // Convert command-data pair string sequence to binary format
      ServiceDescription::ScaSequence sca;
      auto commandDataPairStrings = split(params.at(1), "\n");
      sca.commandDataPairs.resize(commandDataPairStrings.size());
      for (size_t i = 0; i < commandDataPairStrings.size(); ++i) {
        sca.commandDataPairs[i] = stringToScaCommandDataPair(commandDataPairStrings[i]);
      }

      ServiceDescription description;
      description.type = sca;
      description.dnsName = params.at(0);
      description.interval = std::chrono::milliseconds(int64_t(b::lexical_cast<double>(params.at(2)) * 1000.0));
      description.linkInfo = linkInfo;

      tryAddToQueue(*queue, CommandQueue::CommandPublishStart{description});
      return "";
    }

    /// RPC handler for publish stop commands
    static std::string publishStopCommand(const std::string& parameter, std::shared_ptr<CommandQueue> queue,
      LinkInfo linkInfo)
    {
      getLogger() << "Received stop command: '" << parameter << "'" << endm;

      ServiceDescription description;
      description.type = ServiceDescription::Register();
      description.dnsName = parameter;
      description.interval = std::chrono::milliseconds(0);
      description.linkInfo = linkInfo;

      tryAddToQueue(*queue, CommandQueue::CommandPublishStop{description});
      return "";
    }

    /// RPC handler for SCA read commands
    static std::string scaRead(const std::string&, BarSharedPtr bar2, LinkInfo linkInfo)
    {
      getLogger() << "SCA_READ" << endm;
      auto result = Sca(*bar2, bar2->getCardType(), linkInfo.link).read();
      return (b::format("0x%x,0x%x") % result.command % result.data).str();
    }

    /// RPC handler for SCA write commands
    static std::string scaWrite(const std::string& parameter, BarSharedPtr bar2, LinkInfo linkInfo)
    {
      getLogger() << "SCA_WRITE: '" << parameter << "'" << endm;
      auto params = split(parameter, ",");
      auto command = convertHexString(params.at(0));
      auto data = convertHexString(params.at(1));
      Sca(*bar2, bar2->getCardType(), linkInfo.link).write(command, data);
      return "";
    }

    /// RPC handler for SCA GPIO read commands
    static std::string scaGpioRead(const std::string&, BarSharedPtr bar2, LinkInfo linkInfo)
    {
      getLogger() << "SCA_GPIO_READ" << endm;
      auto result = Sca(*bar2, bar2->getCardType(), linkInfo.link).gpioRead();
      return (b::format("0x%x") % result.data).str();
    }

    /// RPC handler for SCA GPIO write commands
    static std::string scaGpioWrite(const std::string& parameter, BarSharedPtr bar2, LinkInfo linkInfo)
    {
      getLogger() << "SCA_GPIO_WRITE: '" << parameter << "'" << endm;
      auto data = convertHexString(parameter);
      Sca(*bar2, bar2->getCardType(), linkInfo.link).gpioWrite(data);
      return "";
    }

    /// RPC handler for SCA blob write commands (sequence of commands)
    static std::string scaBlobWrite(const std::string& parameter, BarSharedPtr bar2, LinkInfo linkInfo)
    {
      getLogger() << "SCA_BLOB_WRITE size=" << parameter.size() << " bytes" << endm;

      // We first split on \n to get the pairs of SCA command and SCA data
      // Since this can be an enormous list of pairs, we walk through it using the tokenizer
      using tokenizer = boost::tokenizer<boost::char_separator<char>>;
      boost::char_separator<char> sep("\n", "", boost::drop_empty_tokens); // Drop '\n' delimiters, keep none
      std::stringstream resultBuffer;
      auto sca = Sca(*bar2, bar2->getCardType(), linkInfo.link);

      for (const std::string& token : tokenizer(parameter, sep)) {
        // Walk through the tokens, these should be the pairs (or comments).
        if (token.find('#') == 0) {
          // We have a comment, skip this token
          continue;
        } else {
          auto commandData = stringToScaCommandDataPair(token);
          try {
            sca.write(commandData);
            auto result = sca.read();
            getLogger() << (b::format("cmd=0x%x data=0x%x result=0x%x") % commandData.command % commandData.data %
              result.data).str() << endm;
            resultBuffer << std::hex << result.data << '\n';
          } catch (const ScaException &e) {
            // If an SCA error occurs, we stop executing the sequence of commands and return the results as far as we got
            // them, plus the error message.
            resultBuffer << e.what();
            break;
          }
        }
      }

      return resultBuffer.str();
    }

    static Sca::CommandData stringToScaCommandDataPair(const std::string& string)
    {
      // The pairs are comma-separated, so we split them.
      std::vector<std::string> pair = split(string, ",");
      if (pair.size() != 2) {
        BOOST_THROW_EXCEPTION(
          AlfException() << ErrorInfo::Message("SCA command-data pair not formatted correctly"));
      }
      Sca::CommandData commandData;
      commandData.command = convertHexString(pair[0]);
      commandData.data = convertHexString(pair[1]);
      return commandData;
    };

    /// Command queue for passing commands from DIM RPC calls (which are in separate threads) to the main program loop
    std::shared_ptr<CommandQueue> mCommandQueue;
    /// serial -> link -> vector of RPC servers
    std::map<int, std::map<int, std::vector<std::unique_ptr<Alf::StringRpcServer>>>> mRpcServers;
    /// serial -> BAR number -> BAR
    std::map<int, std::map<int, BarSharedPtr>> mBars;
    /// Object representing a publishing DIM service
    std::map<std::string, std::unique_ptr<Service>> mServices;
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
