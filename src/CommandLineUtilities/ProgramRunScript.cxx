/// \file ProgramRunScript.cxx
/// \brief Utility that runs a Python script to perform actions on a channel
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iostream>
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
# rorc-run-script --script=example.py --id=-1 --channel=0

print 'Hello RORC Python script!'

# Printing function docs
print rorc_channel.register_read_32.__doc__
print rorc_channel.register_write_32.__doc__

# Reading and writing registers
rorc_channel.register_read_32(0x40)
rorc_channel.register_write_32(0x40, 123)
)";

/// Pointer for Python interface
AliceO2::Rorc::ChannelFactory::SlaveSharedPtr sChannel = nullptr;

struct PythonWrapper
{
    static int registerRead(int address)
    {
      auto channel = sChannel;
      if (channel) {
        return channel->readRegister(address / 4);
      } else {
        // TODO throw exception
        printf("Invalid channel state\n");
        return -1;
      }
    }

    static void registerWrite(int address, int value)
    {
      auto channel = sChannel;
      if (channel) {
        return channel->writeRegister(address / 4, value);
      } else {
        // TODO throw exception
        printf("Invalid channel state\n");
      }
    }

    /// Puts this class into the given Python namespace
    static void putClass(bpy::object& mainNamespace)
    {
      auto className = "rorc_channel";
      auto readName = "register_read_32";
      auto writeName = "register_write_32";
      auto readDoc =
          "Read the 32-bit value at given 32-bit aligned address\n"
          "\n"
          "Args:\n"
          "    index: 32-bit aligned address of the register\n"
          "Returns:\n"
          "    The 32-bit value of the register";
      auto writeDoc =
          "Write a 32-bit value at given 32-bit aligned address\n"
          "\n"
          "Args:\n"
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
          "./rorc-run-script --id=12345 --channel=0 --script=myscript.py"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionChannel(options);
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

      auto cardId = Options::getOptionCardId(map);
      int channelNumber = Options::getOptionChannel(map);

      try {
        // Initialize Python environment
        Py_Initialize();
        bpy::object mainModule = bpy::import("__main__");
        bpy::object mainNamespace = mainModule.attr("__dict__");

        // Initialize interface wrapper
        PythonWrapper::putClass(mainNamespace);

        // Set channel object for PythonWrapper
        auto params = AliceO2::Rorc::Parameters::makeParameters(cardId, channelNumber);
        sChannel = AliceO2::Rorc::ChannelFactory().getSlave(params);
        AliceO2::Rorc::Utilities::GuardFunction guard = {[&]{ sChannel.reset(); }};

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
