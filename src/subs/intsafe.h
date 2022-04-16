//==============================================================================
//
//  intsafe.h
//
#ifdef OS_WIN
#ifndef INTSAFE_H_INCLUDED
#define INTSAFE_H_INCLUDED

#include "windows.h"

HRESULT SIZETMult(SIZE_T multiplicand, SIZE_T multiplier, SIZE_T* result);

#endif
#endif
