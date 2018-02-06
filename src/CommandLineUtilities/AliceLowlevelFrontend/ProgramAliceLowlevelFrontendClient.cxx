/// \file ProgramAliceLowlevelFrontendClient.cxx
/// \brief Utility that starts an example ALICE Lowlevel Frontend (ALF) DIM client
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include "Common/GuardFunction.h"
#include <iostream>
#include <thread>
#include <dim/dic.hxx>
#include "AliceLowlevelFrontend.h"
#include "AlfException.h"
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
      return {"ALF DIM Client example", "ALICE low-level front-end DIM Client example",
        "roc-alf-client --serial=12345 --link=42"};
    }

    virtual void addOptions(boost::program_options::options_description& options) override
    {
      options.add_options()
        ("serial", boost::program_options::value<int>(&mSerialNumber), "Card serial number")
        ("link", boost::program_options::value<int>(&mLink), "Link");
    }

    virtual void run(const boost::program_options::variables_map&) override
    {
      // Get DIM DNS node from environment
      if (getenv(std::string("DIM_DNS_NODE").c_str()) == nullptr) {
        BOOST_THROW_EXCEPTION(
          Alf::AlfException() << Alf::ErrorInfo::Message("Environment variable 'DIM_DNS_NODE' not set"));
      }

      cout << "Using serial=" << mSerialNumber << " link=" << mLink << '\n';

      // Initialize DIM objects
      Alf::ServiceNames names(mSerialNumber, mLink);
      TemperatureInfo alfTestInt(names.temperature());
      Alf::RegisterReadRpc readRpc(names.registerReadRpc());
      Alf::RegisterWriteRpc writeRpc(names.registerWriteRpc());
      Alf::ScaReadRpc scaReadRpc(names.scaRead());
      Alf::ScaWriteRpc scaWriteRpc(names.scaWrite());
      Alf::ScaGpioReadRpc scaGpioReadRpc(names.scaGpioRead());
      Alf::ScaGpioWriteRpc scaGpioWriteRpc(names.scaGpioWrite());
      Alf::ScaWriteSequence scaWriteSequence(names.scaWriteSequence());
      Alf::PublishRegistersStartRpc publishRegistersStartRpc(names.publishRegistersStart());
      Alf::PublishRegistersStopRpc publishRegistersStopRpc(names.publishRegistersStop());
      Alf::PublishScaSequenceStartRpc publishScaSequenceStartRpc(names.publishScaSequenceStart());
      Alf::PublishScaSequenceStopRpc publishScaSequenceStopRpc(names.publishScaSequenceStop());

      publishRegistersStartRpc.publish("TEST_1", 5.0, {0x1fc});
      publishRegistersStartRpc.publish("TEST_2", 5.0, {0x100, 0x104});
      publishScaSequenceStartRpc.publish("TEST_3", 2.5, {{0x0, 0x1}, {0x10, 0x11}});

      AliceO2::Common::GuardFunction publishStopper{
        [&]() {
          publishRegistersStopRpc.stop("TEST_1");
          publishRegistersStopRpc.stop("TEST_2");
          publishScaSequenceStopRpc.stop("TEST_3");
        }
      };

      for (int i = 0; i < 3; ++i) {
        cout << "SCA write '" << i << "'" << endl;
        cout << "  result: " << scaWriteRpc.write(0xabcdabcd, i) << endl;
        cout << "SCA read" << endl;
        cout << "  result: " << scaReadRpc.read() << endl;
      }

      for (int i = 0; i < 3; ++i) {
        cout << "SCA GPIO write '" << i << "'" << endl;
        cout << "  result: " << scaGpioWriteRpc.write(i) << endl;
        cout << "SCA GPIO read" << endl;
        cout << "  result: " << scaGpioReadRpc.read() << endl;
      }

      {
        cout << "Reads & writes to 0x1fc..." << endl;
        for (int i = 0; i < 3; ++i) {
          writeRpc.writeRegister(0x1fc, 0x123);
          readRpc.readRegister(0x1fc);
        }
        cout << "Done!" << endl;
      }

      {
        size_t numInts = 4;
        cout << "Writing blob of " << numInts << " pairs of 32-bit ints..." << endl;
        std::vector<std::pair<uint32_t, uint32_t>> buffer(numInts);
        for (size_t i = 0; i < buffer.size(); ++i) {
          buffer[i] = {0xabcdab + i * 2, 0xabcdab + i * 2 + 1};
        }

        std::string result = scaWriteSequence.write(buffer);
        cout << "Done!" << endl;
        cout << "Got result: \n";
        cout << "  " << result << '\n';
      }

      {
        cout << "Writing blob with comments..." << endl;
        std::string result = scaWriteSequence.write("# Hello!\nabcdab11,22\nabcdab33,44\n# Bye!");
        cout << "Done!" << endl;
        cout << "Got result: \n";
        cout << "  " << result << '\n';
      }

      try {
        cout << "Writing bad blob..." << endl;
        std::string result = scaWriteSequence.write("I AM BAD\n11,22\n33,44\nAAAAAAAAaaaaa");
      } catch (const Alf::AlfException& e) {
        cout << "Successfully broke the server!\n";
      }

      while (!isSigInt())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }

    int mSerialNumber;
    int mLink;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramAliceLowlevelFrontendClient().execute(argc, argv);
}
