/*
 * HexFileImage.h
 */

#ifndef HEXFILEIMAGE_H_
#define HEXFILEIMAGE_H_

// Uncomment one of the following, and one only, not more, not less.
//#define HEXFILE_RN2483_101
//#define HEXFILE_RN2483_103
//#define HEXFILE_RN2483_104
//#define HEXFILE_RN2903AU_097rc7
//#define HEXFILE_RN2903_098
//#define HEXFILE_RN2903_103

#if defined(HEXFILE_RN2483_101)
#include "HexFileImage2483_101.h"
#elif defined(HEXFILE_RN2483_103)
#include "HexFileImage2483_103.h"
#elif defined(HEXFILE_RN2483_104)
#include "HexFileImage2483_104.h"
#elif defined(HEXFILE_RN2903AU_097rc7)
#include "HexFileImage2903AU_097rc7.h"
#elif defined(HEXFILE_RN2903_098)
#include "HexFileImage2903_098.h"
#elif defined(HEXFILE_RN2903_103)
#include "HexFileImage2903_103.h"
#else
#error "Please define one the the following: HEXFILE_RN2483_101, HEXFILE_RN2483_103, HEXFILE_RN2903AU_097rc7, HEXFILE_RN2903_098"
#endif


#endif /* HEXFILEIMAGE_H_ */
