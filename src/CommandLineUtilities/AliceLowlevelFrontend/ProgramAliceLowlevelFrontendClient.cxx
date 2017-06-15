/// \file ProgramAliceLowlevelFrontendClient.cxx
/// \brief Utility that starts an example ALICE Lowlevel Frontend (ALF) DIM client
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include <thread>
#include <dim/dic.hxx>
#include "AliceLowlevelFrontend.h"
#include "ExceptionInternal.h"
#include "ServiceNames.h"

using std::cout;
using std::endl;

namespace {
using namespace AliceO2::roc::CommandLineUtilities;

double gTemperature = 0;

class TemperatureInfo: public DimInfo
{
  public:

    TemperatureInfo(const std::string& serviceName)
        : DimInfo(serviceName.c_str(), std::nan(""))
    {
    }

  private:

    void infoHandler() override
    {
      gTemperature = getDouble();
    }
};

class ProgramAliceLowlevelFrontendClient: public Program
{
  public:

    virtual Description getDescription() override
    {
      return {"ALF DIM Client example", "ALICE low-level front-end DIM Client example", "roc-alf-client"};
    }

    virtual void addOptions(boost::program_options::options_description& options) override
    {
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionCardId(options);
    }

    virtual void run(const boost::program_options::variables_map& map) override
    {
      // Get DIM DNS node from environment
      if (getenv(std::string("DIM_DNS_NODE").c_str()) == nullptr) {
        BOOST_THROW_EXCEPTION(
            AliceO2::roc::Exception()
                << AliceO2::roc::ErrorInfo::Message("Environment variable 'DIM_DNS_NODE' not set"));
      }

      // Get program options
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);

      // Initialize DIM objects
      Alf::ServiceNames names(serialNumber, channelNumber);
      TemperatureInfo alfTestInt(names.temperature());
      Alf::RegisterReadRpc readRpc(names.registerReadRpc());
      Alf::RegisterWriteRpc writeRpc(names.registerWriteRpc());
      Alf::ScaReadRpc scaReadRpc(names.scaRead());
      Alf::ScaWriteRpc scaWriteRpc(names.scaWrite());
      Alf::PublishRpc publishRpc(names.publishStartCommandRpc());

      publishRpc.publish("ALF/TEST/1", 1.0, {0x1fc});
      publishRpc.publish("ALF/TEST/2", 3.0, {0x100, 0x104, 0x108});

      cout << "1k writes to 0x1fc..." << endl;
      for (int i = 0; i < 1000; ++i) {
        readRpc.readRegister(0x1fc);
      }
      cout << "Done!" << endl;

      cout << "SCA read" << endl;
      cout << "  result: " << scaReadRpc.read() << endl;
      cout << "SCA write" << endl;
      cout << "  result: " << scaWriteRpc.write(0x0, 0x0) << endl;

      while (!isSigInt())
      {
        cout << "-------------------------------------\n";
        cout << "Temperature   = " << gTemperature << endl;

        int writes = 10; //std::rand() % 50;
        cout << "Write   0x1f8 = 0x1 times " << writes << endl;
        for (int i = 0; i < writes; ++i) {
          writeRpc.writeRegister(0x1f8, 0x1);
        }

        cout << "Read    0x1fc = " << readRpc.readRegister(0x1fc) << endl;
        cout << "Read    0x1ec = " << readRpc.readRegister(0x1ec) << endl;
        cout << "Cmd     0x1f4 = 0x1" << endl;
        writeRpc.writeRegister(0x1f4, 0x1);
        cout << "Cmd     0x1f4 = 0x2" << endl;
        writeRpc.writeRegister(0x1f4, 0x1);
        cout << "Cmd     0x1f4 = 0x3" << endl;
        writeRpc.writeRegister(0x1f4, 0x1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }

      Alf::PublishStopRpc publishStopRpc(names.publishStopCommandRpc());
      publishStopRpc.stop("ALF/TEST/1");
      publishStopRpc.stop("ALF/TEST/2");
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramAliceLowlevelFrontendClient().execute(argc, argv);
}
