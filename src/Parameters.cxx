/// \file Parameters.cxx
/// \brief Implementation of the RORC Parameters and associated functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/Parameters.h"
#include <map>
#include <set>
#include <string>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace roc {

/// Variant used for internal storage of parameters
using Variant = boost::variant<size_t, int32_t, bool, Parameters::BufferParametersType, Parameters::CardIdType,
  Parameters::GeneratorLoopbackType, Parameters::GeneratorPatternType, Parameters::ReadoutModeType,
  Parameters::LinkMaskType>;

using KeyType = const char*;

/// Map used for internal storage of parameters
using Map = std::map<KeyType, Variant>;

/// PIMPL for hiding Parameters implementation
struct ParametersPimpl
{
    /// Map for storage of parameters
    Map map;
};

namespace {

/// Set a parameter from the map
/// \tparam T The parameter type to get
/// \tparam Map The map type to extract from
template <class T>
void setParam(Map& map, KeyType key, const T& value)
{
    map[key] = Variant { value };
}

/// Get a parameter from the map
/// \tparam T The parameter type to get
/// \tparam Map The map type to extract from
/// \return The value wrapped in an optional if it was found, or boost::none if it was not
template <typename T>
auto getParam(const Map& map, KeyType key) -> boost::optional<T>
{
  auto iterator = map.find(key);
  if (iterator != map.end()) {
    auto variant = iterator->second;
    auto value = boost::get<T>(variant);
    return value;
  } else {
    return boost::none;
  }
}

/// Get a parameter from the map, throwing an exception if it is not present
/// \tparam T The parameter type to get
/// \tparam Map The map type to extract from
/// \return The value
template <typename T>
auto getParamRequired(const Map& map, KeyType key) -> T
{
  if (auto optional = getParam<T>(map, key)) {
    return *optional;
  } else {
    BOOST_THROW_EXCEPTION(ParameterException()
        << ErrorInfo::Message("Parameter was not set")
        << ErrorInfo::ParameterKey(key));
  }
}
} // Anonymous namespace

/// Helper macro to implement getters/setters
/// \param _param_name The name of the parameter
/// \param _key_string The string for the parameter's map key
#define _PARAMETER_FUNCTIONS(_param_name, _key_string)\
auto Parameters::set##_param_name(_param_name##Type value) -> Parameters&\
{\
  setParam(mPimpl->map, _key_string, value);\
  return *this;\
}\
\
auto Parameters::get##_param_name() const -> boost::optional<_param_name##Type>\
{\
  return getParam<_param_name##Type>(mPimpl->map, _key_string);\
}\
\
auto Parameters::get##_param_name##Required() const -> _param_name##Type\
{\
  return getParamRequired<_param_name##Type>(mPimpl->map, _key_string);\
}

_PARAMETER_FUNCTIONS(BufferParameters, "buffer_parameters")
_PARAMETER_FUNCTIONS(CardId, "card_id")
_PARAMETER_FUNCTIONS(ChannelNumber, "channel_number")
_PARAMETER_FUNCTIONS(DmaPageSize, "dma_page_size")
_PARAMETER_FUNCTIONS(GeneratorEnabled, "generator_enabled")
_PARAMETER_FUNCTIONS(GeneratorLoopback, "generator_loopback")
_PARAMETER_FUNCTIONS(GeneratorDataSize, "generator_data_size")
_PARAMETER_FUNCTIONS(GeneratorPattern, "generator_pattern")
_PARAMETER_FUNCTIONS(ReadoutMode, "readout_mode")
_PARAMETER_FUNCTIONS(ForcedUnlockEnabled, "forced_unlock")
_PARAMETER_FUNCTIONS(LinkMask, "link_mask")
#undef _PARAMETER_FUNCTIONS

Parameters::Parameters() : mPimpl(std::make_unique<ParametersPimpl>())
{
}

Parameters::~Parameters()
{
}

Parameters::Parameters(const Parameters& other)
{
  mPimpl = std::make_unique<ParametersPimpl>();
  mPimpl->map = other.mPimpl->map;
}

Parameters::Parameters(Parameters&& other)
{
  mPimpl = std::move(other.mPimpl);
}

Parameters& Parameters::operator=(const Parameters& other)
{
  if (&other == this) {
    return *this;
  }
  if (!mPimpl) {
    mPimpl = std::make_unique<ParametersPimpl>();
  }
  mPimpl->map = other.mPimpl->map;
  return *this;
}

Parameters& Parameters::operator=(Parameters&& other)
{
  if (&other == this) {
    return *this;
  }
  mPimpl = std::move(other.mPimpl);
  return *this;
}

} // namespace roc
} // namespace AliceO2
