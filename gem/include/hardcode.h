#ifndef HARD_CODE_H
#define HARD_CODE_H

/*
 * hard-coded define macros, when hardware development is finalized, this file 
 * will be removed
 */

#define SORTING_ALGORITHM
//#define DANNING_ALGORITHM

// leave this this prameter uncommented
#define DANNING_ALGORITHM_RMS_THRESHOLD 5.0 // Ben's firmware is using 5.0

//#define USE_VME
#define USE_SSP
//#define USE_SRS

//#define INVERSE_POLARITY_VALID

// the UVA type APV hybrid card and INFN type APV hybrid card have different strip conversion
// here we add an option to choose the corresponding types

#define UVA_TYPE_APV_HYBRID
//#define INFN_TYPE_APV_HYBRID

#ifdef UVA_TYPE_APV_HYBRID
constexpr int g_skip_channel[] = {0, 1, 2, 3, 4, 5, 6};
#elif defined(INFN_TYPE_APV_HYBRID)
constexpr int g_skip_channel[] = {16, 17, 18, 19, 20, 21, 22};
#else
constexpr int g_skip_channel[] = {};
#endif

#endif
