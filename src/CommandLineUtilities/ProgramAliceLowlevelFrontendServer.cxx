/// \file ProgramAliceLowlevelFrontendServer.cxx
/// \brief Utility that starts the ALICE Lowlevel Frontend (ALF) DIM server
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <chrono>
#include <iostream>
#include <functional>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include <dim/dis.hxx>
#include "AliceLowlevelFrontend.h"
#include "Common/BasicThread.h"
#include "Common/GuardFunction.h"
#include "ExceptionInternal.h"
#include "folly/ProducerConsumerQueue.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ChannelFactory.h"
#include "Visitor.h"

namespace {
using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;

InfoLogger& getInfoLogger()
{
  static InfoLogger logger;
  return logger;
}

using ChannelSharedPtr = std::shared_ptr<BarInterface>;

/// Splits a string
static std::vector<std::string> split(const std::string& string, const char* separators = ",")
{
  std::vector<std::string> split;
  boost::split(split, string, boost::is_any_of(separators));
  return split;
}

/// Lexical casts a vector
template<typename T, typename U>
static std::vector<T> lexicalCastVector(const std::vector<U>& items)
{
  std::vector<T> result;
  for (const auto& i : items) {
    result.push_back(boost::lexical_cast<T>(i));
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

          auto format = boost::str(boost::format("I:%d") % registerValues.size());
          dimService = std::make_unique<DimService>(mDescription.dnsName.c_str(), format.c_str(), registerValues.data(),
            registerValues.size());

          getInfoLogger() << "Starting publisher '" << mDescription.dnsName << "' with "
            << mDescription.addresses.size() << " address(es) at interval "
            << mDescription.interval.count() << "ms" << InfoLogger::endm;
        }

        ~Service()
        {
          getInfoLogger() << "Stopping publisher '" << mDescription.dnsName << "'" << InfoLogger::endm;
        }

        void updateRegisterValues(RegisterReadWriteInterface& channel)
        {
          getInfoLogger() << "Updating '" << mDescription.dnsName << "':" << InfoLogger::endm;
          for (size_t i = 0; i < mDescription.addresses.size(); ++i) {
            auto index = mDescription.addresses[i] / 4;
            auto value = channel.readRegister(index);
            getInfoLogger() << "  " << mDescription.addresses[i] << " = " << value << InfoLogger::endm;
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

using CommandVariant = boost::variant<CommandPublishStart, CommandPublishStop>;

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

class ProgramAliceLowlevelFrontendServer: public Program
{
  public:

    virtual Description getDescription() override
    {
      return {"ALF DIM Server", "ALICE low-level front-end DIM Server", "roc-alf-server --serial=12345 --channel=0"};
    }

    virtual void addOptions(boost::program_options::options_description& options) override
    {
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
    }

    virtual void run(const boost::program_options::variables_map& map) override
    {
      // Get DIM DNS node from environment
      if (getenv(std::string("DIM_DNS_NODE").c_str()) == nullptr) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Environment variable 'DIM_DNS_NODE' not set"));
      }

      // Get card channel for register access
      mSerialNumber = Options::getOptionSerialNumber(map);
      mChannelNumber = Options::getOptionChannel(map);
      auto params = AliceO2::roc::Parameters::makeParameters(mSerialNumber, mChannelNumber);
      auto channel = AliceO2::roc::ChannelFactory().getBar(params);

      DimServer::start("ALF");

      // Object for service DNS names
      Alf::ServiceNames names(mSerialNumber, mChannelNumber);

      // Start RPC server for reading registers
      getInfoLogger() << "Starting service " << names.registerReadRpc() << InfoLogger::endm;
      Alf::StringRpcServer registerReadRpcServer(names.registerReadRpc(),
          [&](const std::string& parameter) {return registerRead(parameter, channel);});

      // Start RPC server for writing registers
      getInfoLogger() << "Starting service " << names.registerWriteRpc() << InfoLogger::endm;
      Alf::StringRpcServer registerWriteRpcServer(names.registerWriteRpc(),
          [&](const std::string& parameter) {return registerWrite(parameter, channel);});

      //PublisherRegistry publisherRegistry;
      PublisherRegistry publisherRegistry {channel};

      // Start RPC server for publish commands
      getInfoLogger() << "Starting service " << names.publishStartCommandRpc() << InfoLogger::endm;
      Alf::StringRpcServer publishStartCommandRpcServer(names.publishStartCommandRpc(),
          [&](const std::string& parameter) {return publishStartCommand(parameter, mCommandQueue);});

      // Start RPC server for stop publish commands
      getInfoLogger() << "Starting service " << names.publishStopCommandRpc() << InfoLogger::endm;
      Alf::StringRpcServer publishStopCommandRpcServer(names.publishStopCommandRpc(),
          [&](const std::string& parameter) {return publishStopCommand(parameter, mCommandQueue);});

      // Start temperature service
      DimService temperatureService(names.temperature().c_str(), mTemperature);

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
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Address out of range"));
      }
    }

    /// RPC handler for register reads
    static std::string registerRead(const std::string& parameter, ChannelSharedPtr channel)
    {
      auto address = boost::lexical_cast<uint64_t>(parameter);
      checkAddress(address);

      uint32_t value = channel->readRegister(address / 4);

      getInfoLogger() << "READ   " << Common::makeRegisterString(address, value) << InfoLogger::endm;
      return std::to_string(value);
    }

    /// RPC handler for register writes
    static std::string registerWrite(const std::string& parameter, ChannelSharedPtr channel)
    {
      std::vector<std::string> params = split(parameter);

      if (params.size() != 2) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Write RPC call did not have 2 parameters"));
      }

      auto address = boost::lexical_cast<uint64_t>(params[0]);
      auto value = boost::lexical_cast<uint32_t>(params[1]);
      checkAddress(address);

      getInfoLogger() << "WRITE  " << Common::makeRegisterString(address, value) << InfoLogger::endm;

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
      getInfoLogger() << "Received publish command: '" << parameter << "'" << InfoLogger::endm;
      auto params = split(parameter, ";");

      ServiceDescription description;
      description.addresses = lexicalCastVector<uintptr_t >(split(params.at(1), ","));
      description.dnsName = params.at(0);
      description.interval = std::chrono::milliseconds(int64_t(boost::lexical_cast<double>(params.at(2)) * 1000.0));

      if (!commandQueue.write(CommandPublishStart{std::move(description)})) {
        getInfoLogger() << "  command queue was full!" << InfoLogger::endm;
      }
      return "";
    }

    /// RPC handler for publish stop commands
    static std::string publishStopCommand(const std::string& parameter, CommandQueue& commandQueue)
    {
      getInfoLogger() << "Received stop command: '" << parameter << "'" << InfoLogger::endm;
      if (!commandQueue.write(CommandPublishStop{parameter})) {
        getInfoLogger() << "  command queue was full!" << InfoLogger::endm;
      }
      return "";
    }

    int mSerialNumber = 0;
    int mChannelNumber = 0;
    CommandQueue mCommandQueue;
    double mTemperature = 40;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramAliceLowlevelFrontendServer().execute(argc, argv);
}
