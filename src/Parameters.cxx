// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Parameters.cxx
/// \brief Implementation of the ReadoutCard Parameters and associated functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/Parameters.h"
#include <map>
#include <regex>
#include <set>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "ExceptionInternal.h"

namespace AliceO2
{
namespace roc
{

/// Variant used for internal storage of parameters
using Variant = boost::variant<size_t, uint32_t, int32_t, bool, Parameters::BufferParametersType, Parameters::CardIdType,
                               Parameters::DataSourceType, Parameters::LinkMaskType, Parameters::AllowRejectionType,
                               Parameters::ClockType, Parameters::CruIdType, Parameters::DatapathModeType, Parameters::DownstreamDataType,
                               Parameters::GbtModeType, Parameters::GbtMuxType, Parameters::GbtMuxMapType, Parameters::PonUpstreamEnabledType,
                               Parameters::OnuAddressType, Parameters::DynamicOffsetEnabledType>;

using KeyType = const char*;

/// Map used for internal storage of parameters
using Map = std::map<KeyType, Variant>;

/// PIMPL for hiding Parameters implementation
struct ParametersPimpl {
  /// Map for storage of parameters
  Map map;
};

namespace
{

/// Set a parameter from the map
/// \tparam T The parameter type to get
/// \tparam Map The map type to extract from
template <class T>
void setParam(Map& map, KeyType key, const T& value)
{
  map[key] = Variant{ value };
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
#define _PARAMETER_FUNCTIONS(_param_name, _key_string)                          \
  auto Parameters::set##_param_name(_param_name##Type value)->Parameters&       \
  {                                                                             \
    setParam(mPimpl->map, _key_string, value);                                  \
    return *this;                                                               \
  }                                                                             \
                                                                                \
  auto Parameters::get##_param_name() const->boost::optional<_param_name##Type> \
  {                                                                             \
    return getParam<_param_name##Type>(mPimpl->map, _key_string);               \
  }                                                                             \
                                                                                \
  auto Parameters::get##_param_name##Required() const->_param_name##Type        \
  {                                                                             \
    return getParamRequired<_param_name##Type>(mPimpl->map, _key_string);       \
  }

_PARAMETER_FUNCTIONS(BufferParameters, "buffer_parameters")
_PARAMETER_FUNCTIONS(CardId, "card_id")
_PARAMETER_FUNCTIONS(ChannelNumber, "channel_number")
_PARAMETER_FUNCTIONS(DmaPageSize, "dma_page_size")
_PARAMETER_FUNCTIONS(DataSource, "data_source")
_PARAMETER_FUNCTIONS(LinkMask, "link_mask")
_PARAMETER_FUNCTIONS(AllowRejection, "allow_rejection")
_PARAMETER_FUNCTIONS(Clock, "clock")
_PARAMETER_FUNCTIONS(CruId, "cru_id")
_PARAMETER_FUNCTIONS(DatapathMode, "datapath_mode")
_PARAMETER_FUNCTIONS(DownstreamData, "downstream_data")
_PARAMETER_FUNCTIONS(GbtMode, "gbt_mode")
_PARAMETER_FUNCTIONS(GbtMux, "gbt_mux")
_PARAMETER_FUNCTIONS(GbtMuxMap, "gbt_mux_map")
_PARAMETER_FUNCTIONS(LinkLoopbackEnabled, "link_loopback_enabled")
_PARAMETER_FUNCTIONS(PonUpstreamEnabled, "pon_upstream_enabled")
_PARAMETER_FUNCTIONS(DynamicOffsetEnabled, "dynamic_offset_enabled")
_PARAMETER_FUNCTIONS(OnuAddress, "onu_address")
_PARAMETER_FUNCTIONS(StbrdEnabled, "stbrd_enabled")
_PARAMETER_FUNCTIONS(TriggerWindowSize, "trigger_window_size")
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

auto Parameters::linkMaskFromString(const std::string& string) -> LinkMaskType
{
  std::set<uint32_t> links;

  try {
    // Separate by comma
    std::vector<std::string> commaSeparateds;
    boost::split(commaSeparateds, string, boost::is_any_of(","));
    for (const auto& commaSeparated : commaSeparateds) {
      if (commaSeparated.find("-") != std::string::npos) {
        // Separate ranges by dash
        std::vector<std::string> dashSeparateds;
        boost::split(dashSeparateds, commaSeparated, boost::is_any_of("-"));
        if (dashSeparateds.size() != 2) {
          BOOST_THROW_EXCEPTION(ParseException() << ErrorInfo::Message("Invalid link string format"));
        }
        auto start = boost::lexical_cast<uint32_t>(dashSeparateds[0]);
        auto end = boost::lexical_cast<uint32_t>(dashSeparateds[1]);
        for (uint32_t i = start; i <= end; ++i) {
          links.insert(i);
        }
      } else {
        links.insert(boost::lexical_cast<uint32_t>(commaSeparated));
      }
    }
  } catch (boost::bad_lexical_cast& e) {
    BOOST_THROW_EXCEPTION(
      ParseException() << ErrorInfo::Message(std::string("Invalid link string format: ") + e.what()));
  }

  return links;
}

auto Parameters::cardIdFromString(const std::string& string) -> CardIdType
{
  if (auto pciAddressMaybe = PciAddress::fromString(string)) {
    return *pciAddressMaybe;
  } else if (auto pciSequenceNumberMaybe = PciSequenceNumber::fromString(string)) {
    return *pciSequenceNumberMaybe;
  } else if (auto serialIdMaybe = SerialId::fromString(string)) {
    return *serialIdMaybe;
  } else {
    BOOST_THROW_EXCEPTION(
      ParseException() << ErrorInfo::Message("Failed to parse card ID as PCI address, sequence number or serial ID"));
  }
}

} // namespace roc
} // namespace AliceO2
