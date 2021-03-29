// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file NamespaceAlias.h
/// \brief Retain backwards compatibility with the AliceO2 namespace
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_NAMESPACE_ALIAS_H_
#define O2_READOUTCARD_NAMESPACE_ALIAS_H_

// "fake" namespace forward decalaration so that
// the `using` directive can find the namespace
namespace o2
{
}

namespace AliceO2
{
using namespace o2;
}

#endif // O2_READOUTCARD_NAMESPACE_ALIAS_H_
