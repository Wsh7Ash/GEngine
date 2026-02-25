#pragma once

// ================================================================
//  VecTypes.h
//  Vec2, Vec3, Vec4 — zero-overhead, constexpr-first vector math.
//
//  Depends on:  MathUtils.h  (Math::Constants, Lerp, Clamp,
//               ApproxEqual, Abs)
//
//  Design goals
//  ─────────────
//  • Every operation that doesn't call std::sqrt / std::fma is
//    fully constexpr so values can be computed at compile time.
//  • Vec3 and Vec4 use alignas(16) for SIMD-friendly layout.
//  • All types are templated on a scalar T (default float).
//  • Free-function overloads keep the interface symmetric:
//      scalar * vec  and  vec * scalar  both work.
//  • [[nodiscard]] on every pure function to catch silent drops.
//
//  Requires C++17 or later.
// ================================================================

#include "mathUtils.h"

#include <cmath>        // std::sqrt, std::fma
#include <cassert>
#include <ostream>

namespace Math
{

// ----------------------------------------------------------------
// Forward declarations
// ----------------------------------------------------------------
template <typename T> struct Vec2;
template <typename T> struct Vec3;
template <typename T> struct Vec4;

// ================================================================
//  Vec2<T>
// ================================================================
template <typename T = float>
struct Vec2
{
    static_assert(std::is_arithmetic_v<T>, "Vec2<T> requires an arithmetic scalar.");

    // ----------------------------------------------------------
    // Data
    // ----------------------------------------------------------
    T x{}, y{};

    // ----------------------------------------------------------
    // Named constants
    // ----------------------------------------------------------
    [[nodiscard]] static constexpr Vec2 Zero()    noexcept { return {T{0}, T{0}}; }
    [[nodiscard]] static constexpr Vec2 One()     noexcept { return {T{1}, T{1}}; }
    [[nodiscard]] static constexpr Vec2 UnitX()   noexcept { return {T{1}, T{0}}; }
    [[nodiscard]] static constexpr Vec2 UnitY()   noexcept { return {T{0}, T{1}}; }

    // ----------------------------------------------------------
    // Construction
    // ----------------------------------------------------------
    constexpr Vec2() noexcept = default;
    constexpr Vec2(T x_, T y_) noexcept : x(x_), y(y_) {}
    explicit constexpr Vec2(T scalar) noexcept : x(scalar), y(scalar) {}

    /// Construct from a Vec3 by dropping z.
    explicit constexpr Vec2(const Vec3<T>& v) noexcept;

    // ----------------------------------------------------------
    // Element access
    // ----------------------------------------------------------
    [[nodiscard]] constexpr T&       operator[](int i)       noexcept { return (&x)[i]; }
    [[nodiscard]] constexpr const T& operator[](int i) const noexcept { return (&x)[i]; }

    // ----------------------------------------------------------
    // Arithmetic — vector × vector
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec2 operator+(const Vec2& rhs) const noexcept { return {x + rhs.x, y + rhs.y}; }
    [[nodiscard]] constexpr Vec2 operator-(const Vec2& rhs) const noexcept { return {x - rhs.x, y - rhs.y}; }
    [[nodiscard]] constexpr Vec2 operator*(const Vec2& rhs) const noexcept { return {x * rhs.x, y * rhs.y}; }
    [[nodiscard]] constexpr Vec2 operator/(const Vec2& rhs) const noexcept { return {x / rhs.x, y / rhs.y}; }

    // ----------------------------------------------------------
    // Arithmetic — vector × scalar
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec2 operator*(T s) const noexcept { return {x * s, y * s}; }
    [[nodiscard]] constexpr Vec2 operator/(T s) const noexcept { return {x / s, y / s}; }

    // ----------------------------------------------------------
    // Unary
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec2 operator-() const noexcept { return {-x, -y}; }
    [[nodiscard]] constexpr Vec2 operator+() const noexcept { return *this; }

    // ----------------------------------------------------------
    // Compound assignment — vector
    // ----------------------------------------------------------
    constexpr Vec2& operator+=(const Vec2& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }
    constexpr Vec2& operator*=(const Vec2& rhs) noexcept { x *= rhs.x; y *= rhs.y; return *this; }
    constexpr Vec2& operator/=(const Vec2& rhs) noexcept { x /= rhs.x; y /= rhs.y; return *this; }

    // ----------------------------------------------------------
    // Compound assignment — scalar
    // ----------------------------------------------------------
    constexpr Vec2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
    constexpr Vec2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }

    // ----------------------------------------------------------
    // Comparison
    // ----------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(const Vec2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
    [[nodiscard]] constexpr bool operator!=(const Vec2& rhs) const noexcept { return !(*this == rhs); }

    // ----------------------------------------------------------
    // Vector math
    // ----------------------------------------------------------

    /// Squared length — constexpr, no sqrt.
    [[nodiscard]] constexpr T LengthSq() const noexcept { return x * x + y * y; }

    /// Euclidean length.
    [[nodiscard]] T Length() const noexcept { return std::sqrt(LengthSq()); }

    /// Returns a unit-length copy. Asserts (debug) that length > 0.
    [[nodiscard]] Vec2 Normalized() const noexcept
    {
        const T len = Length();
        assert(len > T{0} && "Vec2::Normalized() called on zero-length vector");
        return *this / len;
    }

    /// Returns a normalised copy, or `fallback` when length ≈ 0.
    [[nodiscard]] Vec2 NormalizedSafe(const Vec2& fallback = Vec2::UnitX()) const noexcept
    {
        const T len = Length();
        return (len > Constants<T>::EPSILON) ? (*this / len) : fallback;
    }

    /// Dot product.
    [[nodiscard]] constexpr T Dot(const Vec2& rhs) const noexcept
    {
        return x * rhs.x + y * rhs.y;
    }

    /// 2-D "cross" (scalar z-component of the 3-D cross product).
    [[nodiscard]] constexpr T Cross(const Vec2& rhs) const noexcept
    {
        return x * rhs.y - y * rhs.x;
    }

    /// Euclidean distance to another vector.
    [[nodiscard]] T Distance(const Vec2& other) const noexcept
    {
        return (*this - other).Length();
    }

    /// Squared distance — no sqrt, useful for comparisons.
    [[nodiscard]] constexpr T DistanceSq(const Vec2& other) const noexcept
    {
        return (*this - other).LengthSq();
    }

    /// Component-wise linear interpolation.
    [[nodiscard]] constexpr Vec2 Lerp(const Vec2& to, T t) const noexcept
    {
        return { Math::Lerp(x, to.x, t), Math::Lerp(y, to.y, t) };
    }

    /// Reflect this vector off a surface with given (unit) normal.
    [[nodiscard]] constexpr Vec2 Reflect(const Vec2& normal) const noexcept
    {
        return *this - normal * (T{2} * Dot(normal));
    }

    /// Component-wise absolute value.
    [[nodiscard]] constexpr Vec2 Abs() const noexcept
    {
        return { Math::Abs(x), Math::Abs(y) };
    }

    /// Component-wise clamp.
    [[nodiscard]] constexpr Vec2 Clamp(const Vec2& lo, const Vec2& hi) const noexcept
    {
        return { Math::Clamp(x, lo.x, hi.x), Math::Clamp(y, lo.y, hi.y) };
    }

    /// Approximate equality using Math::ApproxEqual per component.
    [[nodiscard]] constexpr bool ApproxEqual(const Vec2& rhs,
                                              T eps = Constants<T>::EPSILON) const noexcept
    {
        return Math::ApproxEqual(x, rhs.x, eps) && Math::ApproxEqual(y, rhs.y, eps);
    }

    /// Perpendicular vector (rotated 90° counter-clockwise).
    [[nodiscard]] constexpr Vec2 Perp() const noexcept { return {-y, x}; }

    // ----------------------------------------------------------
    // Stream output
    // ----------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v)
    {
        return os << "Vec2(" << v.x << ", " << v.y << ")";
    }
};

// Scalar * Vec2
template <typename T>
[[nodiscard]] constexpr Vec2<T> operator*(T s, const Vec2<T>& v) noexcept { return v * s; }

// Free-function helpers
template <typename T> [[nodiscard]] constexpr T    Dot(const Vec2<T>& a, const Vec2<T>& b)     noexcept { return a.Dot(b); }
template <typename T> [[nodiscard]] inline         T    Distance(const Vec2<T>& a, const Vec2<T>& b) noexcept { return a.Distance(b); }
template <typename T> [[nodiscard]] constexpr T    DistanceSq(const Vec2<T>& a, const Vec2<T>& b) noexcept { return a.DistanceSq(b); }
template <typename T> [[nodiscard]] inline         Vec2<T> Normalize(const Vec2<T>& v)          noexcept { return v.Normalized(); }
template <typename T> [[nodiscard]] constexpr Vec2<T> Lerp(const Vec2<T>& a, const Vec2<T>& b, T t) noexcept { return a.Lerp(b, t); }

// Common aliases
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2i = Vec2<int>;


// ================================================================
//  Vec3<T>
// ================================================================
template <typename T = float>
struct alignas(16) Vec3
{
    static_assert(std::is_arithmetic_v<T>, "Vec3<T> requires an arithmetic scalar.");

    // ----------------------------------------------------------
    // Data  (w-pad keeps the struct at 16 bytes for float / 32 for double)
    // ----------------------------------------------------------
    T x{}, y{}, z{};

private:
    T _pad{};   // padding — not exposed; keeps sizeof(Vec3<float>) == 16

public:
    // ----------------------------------------------------------
    // Named constants
    // ----------------------------------------------------------
    [[nodiscard]] static constexpr Vec3 Zero()    noexcept { return {T{0}, T{0}, T{0}}; }
    [[nodiscard]] static constexpr Vec3 One()     noexcept { return {T{1}, T{1}, T{1}}; }
    [[nodiscard]] static constexpr Vec3 UnitX()   noexcept { return {T{1}, T{0}, T{0}}; }
    [[nodiscard]] static constexpr Vec3 UnitY()   noexcept { return {T{0}, T{1}, T{0}}; }
    [[nodiscard]] static constexpr Vec3 UnitZ()   noexcept { return {T{0}, T{0}, T{1}}; }
    [[nodiscard]] static constexpr Vec3 Up()      noexcept { return UnitY(); }
    [[nodiscard]] static constexpr Vec3 Right()   noexcept { return UnitX(); }
    [[nodiscard]] static constexpr Vec3 Forward() noexcept { return UnitZ(); }

    // ----------------------------------------------------------
    // Construction
    // ----------------------------------------------------------
    constexpr Vec3() noexcept = default;
    constexpr Vec3(T x_, T y_, T z_) noexcept : x(x_), y(y_), z(z_) {}
    explicit constexpr Vec3(T scalar) noexcept : x(scalar), y(scalar), z(scalar) {}

    /// Construct from Vec2 + optional z.
    constexpr Vec3(const Vec2<T>& v, T z_ = T{0}) noexcept : x(v.x), y(v.y), z(z_) {}

    /// Construct from Vec4 by dropping w.
    explicit constexpr Vec3(const Vec4<T>& v) noexcept;

    // ----------------------------------------------------------
    // Element access
    // ----------------------------------------------------------
    [[nodiscard]] constexpr T&       operator[](int i)       noexcept { return (&x)[i]; }
    [[nodiscard]] constexpr const T& operator[](int i) const noexcept { return (&x)[i]; }

    // ----------------------------------------------------------
    // Swizzle helpers (xy, xz, yz)
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec2<T> xy() const noexcept { return {x, y}; }
    [[nodiscard]] constexpr Vec2<T> xz() const noexcept { return {x, z}; }
    [[nodiscard]] constexpr Vec2<T> yz() const noexcept { return {y, z}; }

    // ----------------------------------------------------------
    // Arithmetic — vector × vector
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec3 operator+(const Vec3& rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    [[nodiscard]] constexpr Vec3 operator-(const Vec3& rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    [[nodiscard]] constexpr Vec3 operator*(const Vec3& rhs) const noexcept { return {x * rhs.x, y * rhs.y, z * rhs.z}; }
    [[nodiscard]] constexpr Vec3 operator/(const Vec3& rhs) const noexcept { return {x / rhs.x, y / rhs.y, z / rhs.z}; }

    // ----------------------------------------------------------
    // Arithmetic — vector × scalar
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec3 operator*(T s) const noexcept { return {x * s, y * s, z * s}; }
    [[nodiscard]] constexpr Vec3 operator/(T s) const noexcept { return {x / s, y / s, z / s}; }

    // ----------------------------------------------------------
    // Unary
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec3 operator-() const noexcept { return {-x, -y, -z}; }
    [[nodiscard]] constexpr Vec3 operator+() const noexcept { return *this; }

    // ----------------------------------------------------------
    // Compound assignment — vector
    // ----------------------------------------------------------
    constexpr Vec3& operator+=(const Vec3& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    constexpr Vec3& operator*=(const Vec3& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
    constexpr Vec3& operator/=(const Vec3& rhs) noexcept { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

    // ----------------------------------------------------------
    // Compound assignment — scalar
    // ----------------------------------------------------------
    constexpr Vec3& operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
    constexpr Vec3& operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }

    // ----------------------------------------------------------
    // Comparison
    // ----------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(const Vec3& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z; }
    [[nodiscard]] constexpr bool operator!=(const Vec3& rhs) const noexcept { return !(*this == rhs); }

    // ----------------------------------------------------------
    // Vector math
    // ----------------------------------------------------------

    [[nodiscard]] constexpr T LengthSq() const noexcept { return x * x + y * y + z * z; }

    [[nodiscard]] T Length() const noexcept { return std::sqrt(LengthSq()); }

    [[nodiscard]] Vec3 Normalized() const noexcept
    {
        const T len = Length();
        assert(len > T{0} && "Vec3::Normalized() called on zero-length vector");
        return *this / len;
    }

    [[nodiscard]] Vec3 NormalizedSafe(const Vec3& fallback = Vec3::UnitX()) const noexcept
    {
        const T len = Length();
        return (len > Constants<T>::EPSILON) ? (*this / len) : fallback;
    }

    /// Dot product.
    [[nodiscard]] constexpr T Dot(const Vec3& rhs) const noexcept
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    /// Cross product — returns a vector perpendicular to both operands.
    [[nodiscard]] constexpr Vec3 Cross(const Vec3& rhs) const noexcept
    {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    [[nodiscard]] T Distance(const Vec3& other)   const noexcept { return (*this - other).Length(); }
    [[nodiscard]] constexpr T DistanceSq(const Vec3& other) const noexcept { return (*this - other).LengthSq(); }

    [[nodiscard]] constexpr Vec3 Lerp(const Vec3& to, T t) const noexcept
    {
        return { Math::Lerp(x, to.x, t), Math::Lerp(y, to.y, t), Math::Lerp(z, to.z, t) };
    }

    /// Reflect off a surface with given (unit) normal.
    [[nodiscard]] constexpr Vec3 Reflect(const Vec3& normal) const noexcept
    {
        return *this - normal * (T{2} * Dot(normal));
    }

    /// Refract through a surface (Snell's law). `eta` = ratio of indices of refraction.
    /// Returns zero vector on total internal reflection.
    [[nodiscard]] Vec3 Refract(const Vec3& normal, T eta) const noexcept
    {
        const T cosI  = -Dot(normal);
        const T sin2T = eta * eta * (T{1} - cosI * cosI);
        if (sin2T > T{1}) return Vec3::Zero(); // total internal reflection
        return (*this) * eta + normal * (eta * cosI - std::sqrt(T{1} - sin2T));
    }

    /// Project this vector onto `onto`.
    [[nodiscard]] constexpr Vec3 Project(const Vec3& onto) const noexcept
    {
        return onto * (Dot(onto) / onto.LengthSq());
    }

    /// Reject (component perpendicular to `onto`).
    [[nodiscard]] constexpr Vec3 Reject(const Vec3& onto) const noexcept
    {
        return *this - Project(onto);
    }

    [[nodiscard]] constexpr Vec3 Abs()  const noexcept { return { Math::Abs(x), Math::Abs(y), Math::Abs(z) }; }
    [[nodiscard]] constexpr Vec3 Clamp(const Vec3& lo, const Vec3& hi) const noexcept
    {
        return { Math::Clamp(x, lo.x, hi.x), Math::Clamp(y, lo.y, hi.y), Math::Clamp(z, lo.z, hi.z) };
    }

    [[nodiscard]] constexpr bool ApproxEqual(const Vec3& rhs, T eps = Constants<T>::EPSILON) const noexcept
    {
        return Math::ApproxEqual(x, rhs.x, eps)
            && Math::ApproxEqual(y, rhs.y, eps)
            && Math::ApproxEqual(z, rhs.z, eps);
    }

    // ----------------------------------------------------------
    // Stream output
    // ----------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const Vec3& v)
    {
        return os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
    }
};

template <typename T>
[[nodiscard]] constexpr Vec3<T> operator*(T s, const Vec3<T>& v) noexcept { return v * s; }

template <typename T> [[nodiscard]] constexpr T       Dot(const Vec3<T>& a, const Vec3<T>& b)      noexcept { return a.Dot(b); }
template <typename T> [[nodiscard]] constexpr Vec3<T> Cross(const Vec3<T>& a, const Vec3<T>& b)    noexcept { return a.Cross(b); }
template <typename T> [[nodiscard]] inline            T       Distance(const Vec3<T>& a, const Vec3<T>& b)  noexcept { return a.Distance(b); }
template <typename T> [[nodiscard]] constexpr T       DistanceSq(const Vec3<T>& a, const Vec3<T>& b) noexcept { return a.DistanceSq(b); }
template <typename T> [[nodiscard]] inline            Vec3<T> Normalize(const Vec3<T>& v)           noexcept { return v.Normalized(); }
template <typename T> [[nodiscard]] constexpr Vec3<T> Lerp(const Vec3<T>& a, const Vec3<T>& b, T t) noexcept { return a.Lerp(b, t); }
template <typename T> [[nodiscard]] constexpr Vec3<T> Reflect(const Vec3<T>& v, const Vec3<T>& n)  noexcept { return v.Reflect(n); }

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Vec3i = Vec3<int>;


// ================================================================
//  Vec4<T>
// ================================================================
template <typename T = float>
struct alignas(16) Vec4
{
    static_assert(std::is_arithmetic_v<T>, "Vec4<T> requires an arithmetic scalar.");

    // ----------------------------------------------------------
    // Data  — 16 bytes for float (SIMD-ready)
    // ----------------------------------------------------------
    T x{}, y{}, z{}, w{};

    // ----------------------------------------------------------
    // Named constants
    // ----------------------------------------------------------
    [[nodiscard]] static constexpr Vec4 Zero()     noexcept { return {T{0}, T{0}, T{0}, T{0}}; }
    [[nodiscard]] static constexpr Vec4 One()      noexcept { return {T{1}, T{1}, T{1}, T{1}}; }
    [[nodiscard]] static constexpr Vec4 UnitX()    noexcept { return {T{1}, T{0}, T{0}, T{0}}; }
    [[nodiscard]] static constexpr Vec4 UnitY()    noexcept { return {T{0}, T{1}, T{0}, T{0}}; }
    [[nodiscard]] static constexpr Vec4 UnitZ()    noexcept { return {T{0}, T{0}, T{1}, T{0}}; }
    [[nodiscard]] static constexpr Vec4 UnitW()    noexcept { return {T{0}, T{0}, T{0}, T{1}}; }

    // Colour helpers (interprets xyzw as rgba)
    [[nodiscard]] static constexpr Vec4 Black()    noexcept { return {T{0}, T{0}, T{0}, T{1}}; }
    [[nodiscard]] static constexpr Vec4 White()    noexcept { return {T{1}, T{1}, T{1}, T{1}}; }
    [[nodiscard]] static constexpr Vec4 Red()      noexcept { return {T{1}, T{0}, T{0}, T{1}}; }
    [[nodiscard]] static constexpr Vec4 Green()    noexcept { return {T{0}, T{1}, T{0}, T{1}}; }
    [[nodiscard]] static constexpr Vec4 Blue()     noexcept { return {T{0}, T{0}, T{1}, T{1}}; }
    [[nodiscard]] static constexpr Vec4 Transparent() noexcept { return {T{0}, T{0}, T{0}, T{0}}; }

    // ----------------------------------------------------------
    // Construction
    // ----------------------------------------------------------
    constexpr Vec4() noexcept = default;
    constexpr Vec4(T x_, T y_, T z_, T w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
    explicit constexpr Vec4(T scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}

    /// Construct from Vec3 + w  (common for homogeneous coordinates).
    constexpr Vec4(const Vec3<T>& v, T w_ = T{0}) noexcept : x(v.x), y(v.y), z(v.z), w(w_) {}

    /// Construct from Vec2 + z + w.
    constexpr Vec4(const Vec2<T>& v, T z_ = T{0}, T w_ = T{0}) noexcept
        : x(v.x), y(v.y), z(z_), w(w_) {}

    // ----------------------------------------------------------
    // Element access
    // ----------------------------------------------------------
    [[nodiscard]] constexpr T&       operator[](int i)       noexcept { return (&x)[i]; }
    [[nodiscard]] constexpr const T& operator[](int i) const noexcept { return (&x)[i]; }

    // ----------------------------------------------------------
    // Swizzle helpers
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec2<T> xy()  const noexcept { return {x, y}; }
    [[nodiscard]] constexpr Vec2<T> zw()  const noexcept { return {z, w}; }
    [[nodiscard]] constexpr Vec3<T> xyz() const noexcept { return {x, y, z}; }

    // Colour component aliases
    [[nodiscard]] constexpr T& r() noexcept { return x; }  [[nodiscard]] constexpr T r() const noexcept { return x; }
    [[nodiscard]] constexpr T& g() noexcept { return y; }  [[nodiscard]] constexpr T g() const noexcept { return y; }
    [[nodiscard]] constexpr T& b() noexcept { return z; }  [[nodiscard]] constexpr T b() const noexcept { return z; }
    [[nodiscard]] constexpr T& a() noexcept { return w; }  [[nodiscard]] constexpr T a() const noexcept { return w; }

    // ----------------------------------------------------------
    // Arithmetic — vector × vector
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec4 operator+(const Vec4& rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
    [[nodiscard]] constexpr Vec4 operator-(const Vec4& rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
    [[nodiscard]] constexpr Vec4 operator*(const Vec4& rhs) const noexcept { return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w}; }
    [[nodiscard]] constexpr Vec4 operator/(const Vec4& rhs) const noexcept { return {x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w}; }

    // ----------------------------------------------------------
    // Arithmetic — vector × scalar
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec4 operator*(T s) const noexcept { return {x * s, y * s, z * s, w * s}; }
    [[nodiscard]] constexpr Vec4 operator/(T s) const noexcept { return {x / s, y / s, z / s, w / s}; }

    // ----------------------------------------------------------
    // Unary
    // ----------------------------------------------------------
    [[nodiscard]] constexpr Vec4 operator-() const noexcept { return {-x, -y, -z, -w}; }
    [[nodiscard]] constexpr Vec4 operator+() const noexcept { return *this; }

    // ----------------------------------------------------------
    // Compound assignment — vector
    // ----------------------------------------------------------
    constexpr Vec4& operator+=(const Vec4& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
    constexpr Vec4& operator-=(const Vec4& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
    constexpr Vec4& operator*=(const Vec4& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
    constexpr Vec4& operator/=(const Vec4& rhs) noexcept { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }

    // ----------------------------------------------------------
    // Compound assignment — scalar
    // ----------------------------------------------------------
    constexpr Vec4& operator*=(T s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
    constexpr Vec4& operator/=(T s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

    // ----------------------------------------------------------
    // Comparison
    // ----------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(const Vec4& rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }
    [[nodiscard]] constexpr bool operator!=(const Vec4& rhs) const noexcept { return !(*this == rhs); }

    // ----------------------------------------------------------
    // Vector math
    // ----------------------------------------------------------

    /// 4-D squared length.
    [[nodiscard]] constexpr T LengthSq() const noexcept { return x * x + y * y + z * z + w * w; }
    [[nodiscard]] T Length() const noexcept { return std::sqrt(LengthSq()); }

    [[nodiscard]] Vec4 Normalized() const noexcept
    {
        const T len = Length();
        assert(len > T{0} && "Vec4::Normalized() called on zero-length vector");
        return *this / len;
    }

    [[nodiscard]] Vec4 NormalizedSafe(const Vec4& fallback = Vec4::UnitX()) const noexcept
    {
        const T len = Length();
        return (len > Constants<T>::EPSILON) ? (*this / len) : fallback;
    }

    /// 4-D dot product (also used for quaternion operations and plane equations).
    [[nodiscard]] constexpr T Dot(const Vec4& rhs) const noexcept
    {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    /// 3-D dot (ignores w — useful when Vec4 is used as a homogeneous position).
    [[nodiscard]] constexpr T Dot3(const Vec4& rhs) const noexcept
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    /// 3-D cross product (ignores and zeroes w).
    [[nodiscard]] constexpr Vec4 Cross3(const Vec4& rhs) const noexcept
    {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x,
            T{0}
        };
    }

    [[nodiscard]] T Distance(const Vec4& other) const noexcept { return (*this - other).Length(); }
    [[nodiscard]] constexpr T DistanceSq(const Vec4& other) const noexcept { return (*this - other).LengthSq(); }

    [[nodiscard]] constexpr Vec4 Lerp(const Vec4& to, T t) const noexcept
    {
        return {
            Math::Lerp(x, to.x, t), Math::Lerp(y, to.y, t),
            Math::Lerp(z, to.z, t), Math::Lerp(w, to.w, t)
        };
    }

    /// Perspective divide: returns xyz/w as a Vec3 (clip → NDC).
    [[nodiscard]] constexpr Vec3<T> PerspectiveDivide() const noexcept
    {
        return (w != T{0}) ? Vec3<T>{x / w, y / w, z / w} : Vec3<T>{x, y, z};
    }

    [[nodiscard]] constexpr Vec4 Abs()  const noexcept
    {
        return { Math::Abs(x), Math::Abs(y), Math::Abs(z), Math::Abs(w) };
    }

    [[nodiscard]] constexpr Vec4 Clamp(const Vec4& lo, const Vec4& hi) const noexcept
    {
        return {
            Math::Clamp(x, lo.x, hi.x), Math::Clamp(y, lo.y, hi.y),
            Math::Clamp(z, lo.z, hi.z), Math::Clamp(w, lo.w, hi.w)
        };
    }

    [[nodiscard]] constexpr bool ApproxEqual(const Vec4& rhs, T eps = Constants<T>::EPSILON) const noexcept
    {
        return Math::ApproxEqual(x, rhs.x, eps)
            && Math::ApproxEqual(y, rhs.y, eps)
            && Math::ApproxEqual(z, rhs.z, eps)
            && Math::ApproxEqual(w, rhs.w, eps);
    }

    // ----------------------------------------------------------
    // Stream output
    // ----------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const Vec4& v)
    {
        return os << "Vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    }
};

template <typename T>
[[nodiscard]] constexpr Vec4<T> operator*(T s, const Vec4<T>& v) noexcept { return v * s; }

template <typename T> [[nodiscard]] constexpr T       Dot(const Vec4<T>& a, const Vec4<T>& b)       noexcept { return a.Dot(b); }
template <typename T> [[nodiscard]] constexpr Vec4<T> Cross3(const Vec4<T>& a, const Vec4<T>& b)    noexcept { return a.Cross3(b); }
template <typename T> [[nodiscard]] inline            T       Distance(const Vec4<T>& a, const Vec4<T>& b)   noexcept { return a.Distance(b); }
template <typename T> [[nodiscard]] constexpr T       DistanceSq(const Vec4<T>& a, const Vec4<T>& b) noexcept { return a.DistanceSq(b); }
template <typename T> [[nodiscard]] inline            Vec4<T> Normalize(const Vec4<T>& v)            noexcept { return v.Normalized(); }
template <typename T> [[nodiscard]] constexpr Vec4<T> Lerp(const Vec4<T>& a, const Vec4<T>& b, T t)  noexcept { return a.Lerp(b, t); }

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;
using Vec4i = Vec4<int>;


// ================================================================
//  Cross-type constructors (defined after all three types exist)
// ================================================================

template <typename T>
constexpr Vec2<T>::Vec2(const Vec3<T>& v) noexcept : x(v.x), y(v.y) {}

template <typename T>
constexpr Vec3<T>::Vec3(const Vec4<T>& v) noexcept : x(v.x), y(v.y), z(v.z) {}


// ================================================================
//  Static-assert layout guarantees
// ================================================================

static_assert(sizeof(Vec3f) == 16,  "Vec3<float>  must be 16 bytes");
static_assert(sizeof(Vec4f) == 16,  "Vec4<float>  must be 16 bytes");
static_assert(alignof(Vec3f) == 16, "Vec3<float>  must be 16-byte aligned");
static_assert(alignof(Vec4f) == 16, "Vec4<float>  must be 16-byte aligned");

} // namespace Math