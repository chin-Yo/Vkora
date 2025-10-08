#ifndef MATH_CONSTANTS_HPP
#define MATH_CONSTANTS_HPP

#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923f // π/2
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962f // π/4
#endif

#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154f // 1/π
#endif

#ifndef M_2_PI
#define M_2_PI 0.63661977236758134308f // 2/π
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390f // 2/√π
#endif

// e
#ifndef M_E
#define M_E 2.71828182845904523536f
#endif

// log₂(e)
#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340736f
#endif

// log₁₀(e)
#ifndef M_LOG10E
#define M_LOG10E 0.43429448190325182765f
#endif

// ln(2)
#ifndef M_LN2
#define M_LN2 0.69314718055994530942f
#endif

// ln(10)
#ifndef M_LN10
#define M_LN10 2.30258509299404568402f
#endif

// √2
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880f
#endif

// √3
#ifndef M_SQRT3
#define M_SQRT3 1.73205080756887729353f
#endif

// 1/√2
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440f
#endif

// φ = (1 + √5) / 2
#ifndef M_PHI
#define M_PHI 1.61803398874989484820f
#endif

#define DEG2RAD (M_PI / 180.0f) // Degree to radian
#define RAD2DEG (180.0f / M_PI) // Radian to degree

#define FLT_EPS std::numeric_limits<float>::epsilon()
#define DBL_EPS std::numeric_limits<double>::epsilon()

// infinity and NaN , If the compiler does not support
#ifndef INFINITY
#define INFINITY (std::numeric_limits<float>::infinity())
#endif

#ifndef NAN
#define NAN (std::numeric_limits<float>::quiet_NaN())
#endif

#define GRAVITY 9.80665f            // (m/s²)
#define SPEED_OF_LIGHT 299792458.0f // (m/s)

#define COLOR_RED 1.0f, 0.0f, 0.0f, 1.0f
#define COLOR_GREEN 0.0f, 1.0f, 0.0f, 1.0f
#define COLOR_BLUE 0.0f, 0.0f, 1.0f, 1.0f
#define COLOR_WHITE 1.0f, 1.0f, 1.0f, 1.0f
#define COLOR_BLACK 0.0f, 0.0f, 0.0f, 1.0f

#endif // MATH_CONSTANTS_HPP
