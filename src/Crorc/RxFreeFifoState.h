/// \file RxFreeFifoState.h
/// \brief Definition of the RxFreeFifoState enum
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_RORC_CRORC_RXFREEFIFOSTATE_H_
#define ALICEO2_SRC_RORC_CRORC_RXFREEFIFOSTATE_H_

enum class RxFreeFifoState
{
	EMPTY, NOT_EMPTY, FULL
};

#endif // ALICEO2_SRC_RORC_CRORC_RXFREEFIFOSTATE_H_