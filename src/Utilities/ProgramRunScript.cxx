/// \file ProgramRunScript.cxx
/// \brief Utility that runs a Python script to perform actions on a channel
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/Parameters.h"
#include "Utilities/Program.h"
#include <iostream>
#include <boost/python.hpp>
#include <python2.7/Python.h>
#include "RORC/ChannelFactory.h"
#include "Util.h"

namespace {
using namespace AliceO2::Rorc::Utilities;
namespace bpy = boost::python;

/// Simple example script
auto sExampleScript =
  "# Run this with:\n"
  "# rorc-run-script --script=file_with_this_script.py --serial=-1 --channel=0\n"
  "\n"
  "print 'Hello RORC Python script!'\n"
  "\n"
  "print rorc_channel.register_read.__doc__\n"
  "print rorc_channel.register_write.__doc__\n"
  "print '\\n'\n"
  "\n"
  "rorc_channel.register_read(0)\n"
  "rorc_channel.register_write(0, 123)";

/// Pointer for Python interface
AliceO2::Rorc::ChannelFactory::SlaveSharedPtr sChannel = nullptr;

struct PythonWrapper
{
    static int registerRead(int index)
    {
      auto channel = sChannel;
      if (channel) {
        return channel->readRegister(index);
      } else {
        // TODO throw exception
        printf("Invalid channel state\n");
        return -1;
      }
    }

    static void registerWrite(int index, int value)
    {
      auto channel = sChannel;
      if (channel) {
        return channel->writeRegister(index, value);
      } else {
        // TODO throw exception
        printf("Invalid channel state\n");
      }
    }

    /// Puts this class into the given Python namespace
    static void putClass(bpy::object& mainNamespace)
    {
      auto className = "rorc_channel";
      auto readName = "register_read";
      auto writeName = "register_write";
      auto readDoc =
          "Read the 32-bit value at given 32-bit index\n"
          "\n"
          "Args:\n"
          "    index: 32-bit based index of the register\n"
          "Returns:\n"
          "    The 32-bit value of the register";
      auto writeDoc =
          "Write a 32-bit value at given 32-bit index\n"
          "\n"
          "Args:\n"
          "    index: 32-bit based index of the register\n"
          "    value: 32-bit value";

      mainNamespace[className] = bpy::class_<PythonWrapper>(className)
          .def(readName, &registerRead, readDoc).staticmethod(readName)
          .def(writeName, &registerWrite, writeDoc).staticmethod(writeName);
    }
};

class ProgramRunScript : public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"Run script", "Runs a Python script to perform actions on a channel",
          "./rorc-run-script --serial=12345 --channel=0 --script=myscript.py"};
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
        BOOST_THROW_EXCEPTION(AliceO2::Rorc::Exception()
        << AliceO2::Rorc::errinfo_rorc_error_message("Empty script path"));
      }

      auto cardId = Options::getOptionCardId(map);
      int channelNumber = Options::getOptionChannel(map);
      auto params = AliceO2::Rorc::Parameters::makeParameters(cardId, channelNumber);
      auto channel = AliceO2::Rorc::ChannelFactory().getSlave(params);

      try {
        // Initialize Python environment
        Py_Initialize();
        bpy::object mainModule = bpy::import("__main__");
        bpy::object mainNamespace = mainModule.attr("__dict__");

        // Initialize interface wrapper
        PythonWrapper::putClass(mainNamespace);

        // Set channel object for PythonWrapper
        AliceO2::Rorc::Util::GuardFunction guard = {
            [&]{ sChannel = channel; },
            [&]{ sChannel.reset(); }};

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
