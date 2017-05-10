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
#include <dim/dis.hxx>
#include "AliceLowlevelFrontend.h"
#include "Common/BasicThread.h"
#include "Common/GuardFunction.h"
#include "ExceptionInternal.h"
#include "RORC/Parameters.h"
#include "RORC/ChannelFactory.h"

namespace {
using namespace AliceO2::Rorc::CommandLineUtilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

using ChannelSharedPtr = std::shared_ptr<ChannelSlaveInterface>;

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

/// Class that handles adding/removing publishers
class PublisherRegistry
{
  public:
    struct ServiceDescription
    {
        std::string dnsName;
        double interval; // in seconds
        std::vector<size_t> addresses;
    };

    using PublishFunction = std::function<void(ChannelSharedPtr)>;

    void add(ChannelSharedPtr channel, ServiceDescription serviceDescription)
    {
      LockGuard lockGuard(mMutex);
      // Remove previous service with this name in case it existed
      removeImpl(serviceDescription.dnsName);

      // Add new service
      cout << "Adding publisher: " << serviceDescription.dnsName << '\n';
      auto iter = mPublishers.insert(
          std::make_pair(serviceDescription.dnsName, std::make_unique<Publisher>(channel, serviceDescription)));
      iter.first->second->start();
    }

    void remove(std::string dnsName)
    {
      LockGuard lockGuard(mMutex);
      removeImpl(dnsName);
    }

  private:
    using Mutex = std::mutex;
    using LockGuard = std::lock_guard<Mutex>;

    void removeImpl(std::string dnsName)
    {
      // Remove if it exists
      auto iter = mPublishers.find(dnsName);
      if (iter != mPublishers.end()) {
        cout << "Removing publisher: " << dnsName << '\n';
        iter->second->stop();
        mPublishers.erase(iter);
      }
    }

    /// Class that publishes values at an interval.
    /// TODO Use condition variables, so we can wake up the thread and stop it immediately, not having to wait for its
    ///   next iteration
    class Publisher: public AliceO2::Common::BasicThread
    {
      public:

        Publisher(ChannelSharedPtr channel, ServiceDescription serviceDescription)
            : mServiceDescription(serviceDescription), mChannel(channel)
        {
        }

        void start()
        {
          BasicThread::start([&](std::atomic<bool>* stopFlag) {
            auto startTime = std::chrono::high_resolution_clock::now();
            int iteration = 0;

            cout << "Starting publisher '" << mServiceDescription.dnsName << "' with "
            << mServiceDescription.addresses.size() << " addresses at interval "
            << mServiceDescription.interval << "s \n";
            AliceO2::Common::GuardFunction logGuard([&]{
              cout << "Stopping publisher '" << mServiceDescription.dnsName << "'\n";});

            // Prepare the service and its variable
            std::vector<uint32_t> registerValues(mServiceDescription.addresses.size());
            auto registerCount = registerValues.size();
            auto size = registerCount * sizeof(decltype(registerValues)::value_type);
            auto format = boost::str(boost::format("I:%d") % registerCount);
            DimService service(mServiceDescription.dnsName.c_str(), format.c_str(), registerValues.data(), size);

            while(!stopFlag->load(std::memory_order_relaxed)) {

              std::ostringstream stream;
              stream << "Publisher '" << mServiceDescription.dnsName << "' publishing:\n";

              for (size_t i = 0; i < mServiceDescription.addresses.size(); ++i) {
                auto index = mServiceDescription.addresses[i] / 4;
                auto value = mChannel->readRegister(index);
                stream << "  " << mServiceDescription.addresses[i] << " = " << value << '\n';
                registerValues[i] = mChannel->readRegister(index);
              }

              cout << stream.str();

              service.updateService();

              auto nextTime = startTime + std::chrono::duration<double>(iteration * mServiceDescription.interval);
              std::this_thread::sleep_until(nextTime);
              iteration++;
            }
          });
        }

        ServiceDescription mServiceDescription;
        ChannelSharedPtr mChannel;
    };

    std::unordered_map<std::string, std::unique_ptr<Publisher>> mPublishers;
    Mutex mMutex;
};

class ProgramAliceLowlevelFrontendServer: public Program
{
  public:

    virtual Description getDescription() override
    {
      return {"ALF DIM Server", "ALICE low-level front-end DIM Server", "./rorc-alf-server --serial=12345 --channel=0"};
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
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);
      auto params = AliceO2::Rorc::Parameters::makeParameters(serialNumber, channelNumber);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(params);

      DimServer::start("ALF");
      // Object that stops the DIM service when destroyed
      AliceO2::Common::GuardFunction dimStopper([]{DimServer::stop();});

      Alf::ServiceNames names(serialNumber, channelNumber);

      // Start RPC server for reading registers
      Alf::StringRpcServer registerReadRpcServer(names.registerReadRpc(),
          [&](const std::string& parameter) {return registerRead(parameter, channel);});

      // Start RPC server for writing registers
      Alf::StringRpcServer registerWriteRpcServer(names.registerWriteRpc(),
          [&](const std::string& parameter) {return registerWrite(parameter, channel);});

      PublisherRegistry publisherRegistry;

      // Start RPC server for publish commands
      Alf::StringRpcServer publishCommandRpcServer(names.publishCommandRpc(),
          [&](const std::string& parameter) {return publishCommand(parameter, channel, publisherRegistry);});

      // Start RPC server for stop publish commands
      Alf::StringRpcServer publishStopCommandRpcServer(names.publishStopCommandRpc(),
          [&](const std::string& parameter) {return publishStopCommand(parameter, publisherRegistry);});

//      publishCommand("/ALF/1;20,40;2.0", channel, publisherRegistry);
//      publishCommand("/ALF/2;504;1.0", channel, publisherRegistry);

      // Start temperature service
      DimService temperatureService(names.temperature().c_str(), mTemperature);
      while (!isSigInt()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
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

      cout << "READ   " << Common::makeRegisterString(address, value);
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

      cout << "WRITE  " << Common::makeRegisterString(address, value);

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
    static std::string publishCommand(const std::string& parameter, ChannelSharedPtr channel,
        PublisherRegistry& registry)
    {
      auto params = split(parameter, ";");
      auto serviceName = params.at(0);
      auto registerAddresses = lexicalCastVector<size_t>(split(params.at(1), ","));
      auto interval = boost::lexical_cast<double>(params.at(2));

      registry.add(channel, { serviceName, interval, registerAddresses });
      return "";
    }

    /// RPC handler for publish stop commands
    static std::string publishStopCommand(const std::string& parameter, PublisherRegistry& registry)
    {
      registry.remove(parameter);
      return "";
    }

    using DimServicePtr = std::unique_ptr<DimService>;

    double mTemperature = 45;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramAliceLowlevelFrontendServer().execute(argc, argv);
}
