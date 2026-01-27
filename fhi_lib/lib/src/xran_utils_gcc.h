#ifndef _XRAN_UTILS_GCC_H_
#define _XRAN_UTILS_GCC_H_

#ifndef __INTEL_COMPILER
#include <immintrin.h>

/* Wrapper for _may_i_use_cpu_feature */
#define _FEATURE_AVX512IFMA52 "avx512ifma"
#define _FEATURE_F16C "f16c"

#define _may_i_use_cpu_feature(feature) __builtin_cpu_supports(feature)

#endif /* __INTEL_COMPILER */

#endif /* _XRAN_UTILS_GCC_H_ */
