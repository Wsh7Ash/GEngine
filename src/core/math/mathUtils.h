#pragma once

#include <type_traits>
#include <cmath>
#include <limits>

// ============================================================
//  MathUtils.h
//  Zero-overhead math constants and utility functions.
//  All symbols live in the Math:: namespace.
//  Requires C++17 or later.
// ============================================================

namespace Math
{

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

template <typename T = double>
struct Constants
{
    static_assert(std::is_floating_point_v<T>,
                  "Math::Constants<T> requires a floating-point type.");

    /// π
    static constexpr T PI           = static_cast<T>(3.14159265358979323846264338327950288L);
    /// 2π
    static constexpr T TWO_PI       = static_cast<T>(6.28318530717958647692528676655900576L);
    /// π / 2
    static constexpr T HALF_PI      = static_cast<T>(1.57079632679489661923132169163975144L);
    /// π / 4
    static constexpr T QUARTER_PI   = static_cast<T>(0.78539816339744830961566084581987572L);
    /// 1 / π
    static constexpr T INV_PI       = static_cast<T>(0.31830988618379067153776716941615590L);
    /// 1 / (2π)
    static constexpr T INV_TWO_PI   = static_cast<T>(0.15915494309189533576888376210807796L);
    /// √2
    static constexpr T SQRT2        = static_cast<T>(1.41421356237309504880168872420969808L);
    /// 1 / √2
    static constexpr T INV_SQRT2    = static_cast<T>(0.70710678118654752440084436210484904L);
    /// √3
    static constexpr T SQRT3        = static_cast<T>(1.73205080756887729352744634150587237L);
    /// Euler's number  e
    static constexpr T E            = static_cast<T>(2.71828182845904523536028747135266250L);
    /// Natural log of 2
    static constexpr T LN2          = static_cast<T>(0.69314718055994530941723212145817657L);
    /// Natural log of 10
    static constexpr T LN10         = static_cast<T>(2.30258509299404568401799145468436421L);
    /// log₂(e)
    static constexpr T LOG2E        = static_cast<T>(1.44269504088896340735992468100189214L);
    /// log₁₀(e)
    static constexpr T LOG10E       = static_cast<T>(0.43429448190325182765112891891660508L);
    /// Golden ratio  φ
    static constexpr T PHI          = static_cast<T>(1.61803398874989484820458683436563812L);

    /// A sensible general-purpose epsilon for the given type.
    /// For most algorithms you'll want to tune this per-use-case.
    static constexpr T EPSILON      = static_cast<T>(1e-6);
    /// The smallest positive value representable by T
    static constexpr T MIN_VALUE    = std::numeric_limits<T>::min();
    /// The largest finite value representable by T
    static constexpr T MAX_VALUE    = std::numeric_limits<T>::max();
    /// Positive infinity
    static constexpr T INFINITY_VAL = std::numeric_limits<T>::infinity();
};

// Convenient aliases for the most common types
using Constantsf = Constants<float>;
using Constantsd = Constants<double>;

// Legacy-style flat constants (double precision) for convenience
constexpr double PI          = Constants<double>::PI;
constexpr double TWO_PI      = Constants<double>::TWO_PI;
constexpr double HALF_PI     = Constants<double>::HALF_PI;
constexpr double QUARTER_PI  = Constants<double>::QUARTER_PI;
constexpr double INV_PI      = Constants<double>::INV_PI;
constexpr double INV_TWO_PI  = Constants<double>::INV_TWO_PI;
constexpr double SQRT2       = Constants<double>::SQRT2;
constexpr double INV_SQRT2   = Constants<double>::INV_SQRT2;
constexpr double SQRT3       = Constants<double>::SQRT3;
constexpr double E           = Constants<double>::E;
constexpr double LN2         = Constants<double>::LN2;
constexpr double LN10        = Constants<double>::LN10;
constexpr double LOG2E       = Constants<double>::LOG2E;
constexpr double LOG10E      = Constants<double>::LOG10E;
constexpr double PHI         = Constants<double>::PHI;
constexpr double EPSILON     = Constants<double>::EPSILON;

// ============================================================
// Utility Functions
// ============================================================

// ------------------------------------------------------------
// Clamp
//   Restricts `value` to the closed interval [lo, hi].
// ------------------------------------------------------------

/// Clamp for arithmetic types — fully constexpr.
template <typename T>
[[nodiscard]] constexpr T Clamp(T value, T lo, T hi) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Clamp requires an arithmetic type.");
    return (value < lo) ? lo
         : (value > hi) ? hi
         : value;
}

/// Convenience overload: Clamp01 — clamps to [0, 1].
template <typename T>
[[nodiscard]] constexpr T Clamp01(T value) noexcept
{
    return Clamp(value, T{0}, T{1});
}

// ------------------------------------------------------------
// Lerp  (linear interpolation / extrapolation)
//   Returns  a + t * (b - a)
//   t == 0 → a,  t == 1 → b.  Extrapolates outside [0,1].
// ------------------------------------------------------------

template <typename T, typename U = T>
[[nodiscard]] constexpr T Lerp(T a, T b, U t) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Lerp requires arithmetic value types.");
    static_assert(std::is_floating_point_v<U>, "Lerp requires a floating-point t parameter.");
    // FMA-friendly form; precise at t==0 and t==1.
    return static_cast<T>(a * (U{1} - t) + b * t);
}

/// Inverse lerp — returns the t in [0,1] such that Lerp(a,b,t) == value.
/// Returns 0 when a == b (avoids division by zero).
template <typename T>
[[nodiscard]] constexpr T InverseLerp(T a, T b, T value) noexcept
{
    static_assert(std::is_floating_point_v<T>, "InverseLerp requires a floating-point type.");
    return (a == b) ? T{0} : (value - a) / (b - a);
}

/// Remap — maps `value` from [inMin, inMax] into [outMin, outMax].
template <typename T>
[[nodiscard]] constexpr T Remap(T value, T inMin, T inMax, T outMin, T outMax) noexcept
{
    static_assert(std::is_floating_point_v<T>, "Remap requires a floating-point type.");
    return Lerp(outMin, outMax, InverseLerp(inMin, inMax, value));
}

// ------------------------------------------------------------
// ApproxEqual
//   Compares two floating-point values with a configurable
//   absolute-or-relative tolerance.
// ------------------------------------------------------------

/// Absolute epsilon comparison.
template <typename T>
[[nodiscard]] constexpr bool ApproxEqual(T a, T b,
                                          T epsilon = Constants<T>::EPSILON) noexcept
{
    static_assert(std::is_floating_point_v<T>, "ApproxEqual requires a floating-point type.");
    const T diff = (a > b) ? (a - b) : (b - a);   // constexpr-safe abs
    return diff <= epsilon;
}

/// Relative + absolute (ULP-agnostic) epsilon comparison.
/// Uses  |a-b| ≤ epsilon * max(|a|, |b|, 1)  — robust near zero.
template <typename T>
[[nodiscard]] constexpr bool ApproxEqualRelative(T a, T b,
                                                   T relEpsilon = Constants<T>::EPSILON) noexcept
{
    static_assert(std::is_floating_point_v<T>, "ApproxEqualRelative requires a floating-point type.");
    const T absA    = (a < T{0}) ? -a : a;
    const T absB    = (b < T{0}) ? -b : b;
    const T largest = (absA > absB) ? absA : absB;
    const T scale   = (largest > T{1}) ? largest : T{1};
    const T diff    = (a > b) ? (a - b) : (b - a);
    return diff <= relEpsilon * scale;
}

// ------------------------------------------------------------
// Angle Conversions
// ------------------------------------------------------------

/// Converts degrees to radians.
template <typename T = double>
[[nodiscard]] constexpr T DegreesToRadians(T degrees) noexcept
{
    static_assert(std::is_floating_point_v<T>, "DegreesToRadians requires a floating-point type.");
    return degrees * (Constants<T>::PI / static_cast<T>(180));
}

/// Converts radians to degrees.
template <typename T = double>
[[nodiscard]] constexpr T RadiansToDegrees(T radians) noexcept
{
    static_assert(std::is_floating_point_v<T>, "RadiansToDegrees requires a floating-point type.");
    return radians * (static_cast<T>(180) / Constants<T>::PI);
}

/// Wraps an angle in radians to (-π, π].
template <typename T>
[[nodiscard]] inline T WrapAngle(T radians) noexcept
{
    static_assert(std::is_floating_point_v<T>, "WrapAngle requires a floating-point type.");
    // std::fmod is not constexpr in C++17, so this is inline-only.
    const T wrapped = std::fmod(radians + Constants<T>::PI, Constants<T>::TWO_PI);
    return (wrapped < T{0} ? wrapped + Constants<T>::TWO_PI : wrapped) - Constants<T>::PI;
}

// ------------------------------------------------------------
// Miscellaneous helpers
// ------------------------------------------------------------

/// Integer / floating-point sign: returns -1, 0, or +1.
template <typename T>
[[nodiscard]] constexpr int Sign(T value) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Sign requires an arithmetic type.");
    return (T{0} < value) - (value < T{0});
}

/// Returns the absolute value — constexpr-safe (avoids std::abs which isn't
/// always constexpr in C++17).
template <typename T>
[[nodiscard]] constexpr T Abs(T value) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Abs requires an arithmetic type.");
    return (value < T{0}) ? -value : value;
}

/// Squares a value.
template <typename T>
[[nodiscard]] constexpr T Square(T value) noexcept { return value * value; }

/// Cubes a value.
template <typename T>
[[nodiscard]] constexpr T Cube(T value) noexcept { return value * value * value; }

/// Smoothstep — Hermite interpolation, t clamped to [0,1].
template <typename T>
[[nodiscard]] constexpr T Smoothstep(T edge0, T edge1, T x) noexcept
{
    static_assert(std::is_floating_point_v<T>, "Smoothstep requires a floating-point type.");
    const T t = Clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (T{3} - T{2} * t);
}

/// Smootherstep (Ken Perlin's improved version — zero 1st and 2nd derivatives at edges).
template <typename T>
[[nodiscard]] constexpr T Smootherstep(T edge0, T edge1, T x) noexcept
{
    static_assert(std::is_floating_point_v<T>, "Smootherstep requires a floating-point type.");
    const T t = Clamp01((x - edge0) / (edge1 - edge0));
    return t * t * t * (t * (t * T{6} - T{15}) + T{10});
}

/// Checks whether an integer value is a power of two.
template <typename T>
[[nodiscard]] constexpr bool IsPowerOfTwo(T value) noexcept
{
    static_assert(std::is_integral_v<T>, "IsPowerOfTwo requires an integral type.");
    return value > T{0} && (value & (value - T{1})) == T{0};
}

/// Rounds an integer up to the next power of two.
[[nodiscard]] constexpr unsigned int NextPowerOfTwo(unsigned int value) noexcept
{
    if (value == 0u) return 1u;
    --value;
    value |= value >> 1u;
    value |= value >> 2u;
    value |= value >> 4u;
    value |= value >> 8u;
    value |= value >> 16u;
    return ++value;
}

} // namespace Math