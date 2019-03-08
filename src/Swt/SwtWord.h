// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file SwtWord.h
/// \brief Definition of the SWTWORD class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include <cstdint>
#include <cstddef>
#include <iomanip>

namespace AliceO2{
namespace roc{

class SwtWord
{
  public:
    SwtWord():
      mLow(0),
      mMed(0),
      mHigh(0)
    {
    }

    SwtWord(uint32_t low, uint32_t med, uint16_t high):
      mLow(low),
      mMed(med),
      mHigh(high)
    {
    }

    SwtWord(uint64_t swtInt)
    {
      mLow = swtInt & 0xffffffff;
      mMed = (swtInt >> 32) & 0xffffffff;
      mHigh = 0;
      //mHigh = (swtInt >> 64) & 0xffffffff;
    }

    bool operator==(const SwtWord& swtWord){
      return (mLow == swtWord.mLow) && (mMed == swtWord.mMed) && ((mHigh & 0xff) == (swtWord.mHigh & 0xff));
    }

    bool operator!=(const SwtWord& swtWord){
      return (mLow != swtWord.mLow) || (mMed != swtWord.mMed) || ((mHigh & 0xff) != (swtWord.mHigh & 0xff));
    }

    void setLow(uint32_t low){
      mLow = low;
    }

    void setMed(uint32_t med){
      mMed = med;
    }

    void setHigh(uint16_t high){
      mHigh = high;
    }

    uint64_t getLow() const{
      return mLow;
    }
    
    uint64_t getMed() const{
      return mMed;
    }

    uint16_t getHigh() const{
      return mHigh;
    }
   
  private:
      uint64_t mLow;
      uint64_t mMed;
      uint16_t mHigh;
};
 
std::ostream& operator<< (std::ostream& stream, const SwtWord &swtWord){
  stream << "0x"  << std::setfill('0')<< std::hex << std::setw(4) << swtWord.getHigh()
     << std::setfill('0') << std::setw(8) << swtWord.getMed() << std::setfill('0') << std::setw(8) << swtWord.getLow();
  return stream;
}

} // namespace roc
} // namespace AliceO2
