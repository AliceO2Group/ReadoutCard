/// \file ProgramAliceLowlevelFrontendClient.cxx
/// \brief Utility that starts an example ALICE Lowlevel Frontend (ALF) DIM client
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>
#include <thread>
#include <dim/dic.hxx>
#include "AliceLowlevelFrontend.h"
#include "AlfException.h"
#include "ServiceNames.h"

using std::cout;
using std::endl;

namespace {
using namespace AliceO2::roc::CommandLineUtilities;

class ProgramAlfScaWriteSequence: public Program
{
  public:

    virtual Description getDescription() override
    {
      return {"ALF SCA write sequence utility", "Writes a sequence of SCA commands from a file, with an optional output"
        "file for returned results",
        "roc-alf-write-seq --serial=12345 --file=/tmp/tpc-config.txt --out=/tmp/result.txt"};
    }

    virtual void addOptions(boost::program_options::options_description& options) override
    {
      options.add_options()
        ("serial", boost::program_options::value<int>(&mSerialNumber),
          "Card serial number")
        ("file", boost::program_options::value<std::string>(&mFilePath)->required(),
          "Path to command sequence file")
        ("out", boost::program_options::value<std::string>(&mOutFilePath),
          "Path to output file. If not specified, will output to stdout");
    }

    virtual void run(const boost::program_options::variables_map&) override
    {
      // Get DIM DNS node from environment
      if (getenv(std::string("DIM_DNS_NODE").c_str()) == nullptr) {
        BOOST_THROW_EXCEPTION(
          Alf::AlfException() << Alf::ErrorInfo::Message("Environment variable 'DIM_DNS_NODE' not set"));
      }

      // Initialize DIM objects
      Alf::ServiceNames names(mSerialNumber);
      Alf::ScaWriteSequence scaWriteSequence(names.scaWriteSequence());

      // Read file
      std::ifstream file(mFilePath);
      std::string commandSequence((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

      // Send commands to ALF
      cout << "Writing command sequence of size " << commandSequence.size() << "...\n";
      std::string result = scaWriteSequence.write(commandSequence);
      cout << "Done!" << endl;

      if (mOutFilePath.empty()) {
        cout << "Received result: \n";
        cout << "" << result << '\n';
      } else {
        std::ofstream outFile(mOutFilePath);
        outFile << result;
      }
    }

  private:
    std::string mFilePath;
    std::string mOutFilePath;
    int mSerialNumber;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramAlfScaWriteSequence().execute(argc, argv);
}
