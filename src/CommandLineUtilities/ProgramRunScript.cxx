/// \file ProgramRunScript.cxx
/// \brief Utility that runs a Python script to perform actions on a channel
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
#include <map>
#include <vector>
#include <boost/python.hpp>
#include <python2.7/Python.h>
#include "ExceptionInternal.h"
#include "RORC/ChannelFactory.h"
#include "RORC/Parameters.h"
#include "Utilities/GuardFunction.h"

namespace {
using namespace AliceO2::Rorc::CommandLineUtilities;
namespace bpy = boost::python;

/// Simple example script
auto sExampleScript = R"(
# Run this with:
# rorc-run-script --example > example.py
# rorc-run-script --script=example.py --id=-1

print 'Hello RORC Python script!'

print '\nPrinting function docs'
print rorc.register_read_32.__doc__
print rorc.register_write_32.__doc__

print '\nReading and writing registers'
channel = 0
rorc.register_read_32(channel, 0x40)
rorc.register_write_32(channel, 0x40, 123)
)";


AliceO2::Rorc::Parameters::CardIdType sCardId;

/// Channels for Python interface
std::map<int, AliceO2::Rorc::ChannelFactory::SlaveSharedPtr> sChannelMap;

struct PythonWrapper
{
    static AliceO2::Rorc::ChannelSlaveInterface* getChannel(int channelNumber)
    {
      auto found = sChannelMap.find(channelNumber);
      if (found != sChannelMap.end()) {
        // Channel is already opened
        return found->second.get();
      } else {
        // Channel is not yet opened, so we open it, insert it into the map, and return it
        auto params = AliceO2::Rorc::Parameters::makeParameters(sCardId, channelNumber);
        auto inserted =
            sChannelMap.insert(std::make_pair(channelNumber, AliceO2::Rorc::ChannelFactory().getSlave(params))).first;
        return inserted->second.get();
      }
    }

    static int registerRead(int channelNumber, uint32_t address)
    {
      return getChannel(channelNumber)->readRegister(address / 4);
    }

    static void registerWrite(int channelNumber, uint32_t address, uint32_t value)
    {
      return getChannel(channelNumber)->writeRegister(address / 4, value);
    }

    /// Puts this class into the given Python namespace
    static void putClass(bpy::object& mainNamespace)
    {
      auto className = "rorc";
      auto readName = "register_read_32";
      auto writeName = "register_write_32";
      auto readDoc =
          "Read the 32-bit value at given 32-bit aligned address\n"
          "\n"
          "Args:\n"
          "    channel: Number of the channel\n"
          "    index: 32-bit aligned address of the register\n"
          "Returns:\n"
          "    The 32-bit value of the register";
      auto writeDoc =
          "Write a 32-bit value at given 32-bit aligned address\n"
          "\n"
          "Args:\n"
          "    channel: Number of the channel\n"
          "    index: 32-bit aligned address of the register\n"
          "    value: 32-bit value to write to the register";

      mainNamespace[className] = bpy::class_<PythonWrapper>(className)
          .def(readName, &registerRead, readDoc).staticmethod(readName)
          .def(writeName, &registerWrite, writeDoc).staticmethod(writeName);
    }
};

class ProgramRunScript : public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Run script", "Runs a Python script to perform actions on a channel",
          "./rorc-run-script --id=12345 --script=myscript.py"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionCardId(options);
      options.add_options()
          ("script", boost::program_options::value<std::string>(&mScriptFilename), "Python script path")
          ("example", boost::program_options::bool_switch(&mPrintExample), "Print example script");
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      if (mPrintExample) {
        std::cout << sExampleScript << '\n';
        return;
      }

      if (mScriptFilename.empty()) {
        BOOST_THROW_EXCEPTION(AliceO2::Rorc::ProgramOptionException()
            << AliceO2::Rorc::ErrorInfo::Message("Empty script path"));
      }

      sCardId = Options::getOptionCardId(map);

      try {
        // Initialize Python environment
        Py_Initialize();
        bpy::object mainModule = bpy::import("__main__");
        bpy::object mainNamespace = mainModule.attr("__dict__");

        // Initialize interface wrapper
        PythonWrapper::putClass(mainNamespace);

        // Close channels on scope exit
//        AliceO2::Rorc::Utilities::GuardFunction guard = {[&]{ sChannelMap.clear(); }};

        // Execute the script
        exec_file(mScriptFilename.c_str(), mainNamespace);
      }
      catch (const bpy::error_already_set& e) {
        std::cout << "Error in Python: " << makePythonExceptionMessage() << std::endl;
      }
    }

    /// Extracts information from the current Python error and creates a string representation
    /// Based on:
    /// http://thejosephturner.com/blog/post/embedding-python-in-c-applications-with-boostpython-part-2/
    std::string makePythonExceptionMessage()
    {
      PyObject* typePointer = nullptr;
      PyObject* valuePointer = nullptr;
      PyObject* tracebackPointer = nullptr;
      PyErr_Fetch(&typePointer, &valuePointer, &tracebackPointer);

      std::string returnString("Unfetchable Python error");

      auto addString = [&](PyObject* object, std::string errorMessage) {
        if (object != nullptr) {
          bpy::handle<> handle(object);
          bpy::str string(handle);
          bpy::extract<std::string> extractedString(string);
          returnString += extractedString.check()
              ? extractedString()
              : errorMessage;
        }
      };

      addString(typePointer, "Unknown exception type");
      addString(valuePointer, ": Unparseable Python error");

      if (tracebackPointer != nullptr) {
        bpy::object traceback(bpy::import("traceback"));
        bpy::object format(traceback.attr("format_tb"));
        bpy::object list(format(bpy::handle<>(tracebackPointer)));
        bpy::object string(bpy::str("\n").join(list));
        bpy::extract<std::string> extractedString(string);
        returnString += extractedString.check()
            ? extractedString()
            : ": Unparseable Python traceback";
      }

      return returnString;
    }

    std::string mScriptFilename;
    bool mPrintExample = false;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramRunScript().execute(argc, argv);
}
