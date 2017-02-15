/*
 * HexFileImage.h
 */

#ifndef HEXFILEIMAGE_H_
#define HEXFILEIMAGE_H_

// Uncomment one of the following, and one only, not more, not less.
//#define HEXFILE_RN2483_101
//#define HEXFILE_RN2903AU_097rc7

#if defined(HEXFILE_RN2483_101)
#include "HexFileImage2483_101.h"
#elif defined(HEXFILE_RN2903AU_097rc7)
#include "HexFileImage2903AU_097rc7.h"
#else
#error "Please define one the the following: HEXFILE_RN2483, HEXFILE_RN2903AU_097rc7"
#endif


#endif /* HEXFILEIMAGE_H_ */
