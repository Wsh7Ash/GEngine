#pragma once

// ================================================================
//  Quaternion.h
//  Unit-quaternion rotation math for 3-D graphics and physics.
//
//  Depends on:  Mat4x4.h  →  VecTypes.h  →  MathUtils.h
//
//  Convention
//  ──────────
//  Storage order:  { x, y, z, w }   (vector part first, scalar last)
//    x, y, z = vector / imaginary part  (i, j, k coefficients)
//    w       = scalar / real part
//
//  Identity:  Quat{0, 0, 0, 1}  (no rotation)
//
//  Multiplication order is RIGHT-TO-LEFT, matching matrix convention:
//    (a * b).RotateVector(v)  ==  a.RotateVector(b.RotateVector(v))
//
//  Euler angles use the intrinsic ZYX order (roll → pitch → yaw),
//  which is the most common convention in game engines and robotics:
//    X = pitch (nose up/down)
//    Y = yaw   (nose left/right)
//    Z = roll  (bank)
//
//  Design goals
//  ────────────
//  • constexpr for all operations not requiring std::sin/cos/acos/atan2.
//  • [[nodiscard]] on every pure function.
//  • alignas(16) for SIMD-friendly layout in arrays.
//  • No heap allocation; everything is value-type.
//
//  Requires C++17 or later.
// ================================================================

#include "Mat4x4.h"

#include <cmath>
#include <cassert>
#include <ostream>

namespace Math
{

template <typename T = float>
struct alignas(16) Quaternion
{
    static_assert(std::is_floating_point_v<T>,
                  "Quaternion<T> requires a floating-point scalar.");

    // ============================================================
    //  Data  —  vector part (xyz) then scalar part (w)
    //  alignas(16) means four floats fit in one XMM/NEON register.
    // ============================================================
    T x{T{0}}, y{T{0}}, z{T{0}}, w{T{1}};

    // ============================================================
    //  Named constants
    // ============================================================

    /// Identity quaternion — represents zero rotation.
    [[nodiscard]] static constexpr Quaternion Identity() noexcept
    {
        return Quaternion{T{0}, T{0}, T{0}, T{1}};
    }

    // ============================================================
    //  Construction
    // ============================================================

    /// Default: identity (no rotation).
    constexpr Quaternion() noexcept = default;

    /// Explicit component constructor  (x, y, z, w).
    constexpr Quaternion(T x_, T y_, T z_, T w_) noexcept
        : x(x_), y(y_), z(z_), w(w_) {}

    /// Construct from a Vec3 (imaginary part) and scalar w.
    constexpr Quaternion(const Vec3<T>& v, T w_) noexcept
        : x(v.x), y(v.y), z(v.z), w(w_) {}

    // ============================================================
    //  Element access
    // ============================================================

    [[nodiscard]] constexpr T& operator[](int i) noexcept
    {
        switch (i) { case 0: return x; case 1: return y; case 2: return z; default: return w; }
    }
    [[nodiscard]] constexpr const T& operator[](int i) const noexcept
    {
        switch (i) { case 0: return x; case 1: return y; case 2: return z; default: return w; }
    }

    /// Vector (imaginary) part.
    [[nodiscard]] constexpr Vec3<T> Vec()    const noexcept { return {x, y, z}; }
    /// Scalar (real) part — same as w, provided for clarity.
    [[nodiscard]] constexpr T       Scalar() const noexcept { return w; }

    // ============================================================
    //  Comparison
    // ============================================================

    [[nodiscard]] constexpr bool operator==(const Quaternion& rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }
    [[nodiscard]] constexpr bool operator!=(const Quaternion& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    /// Component-wise approximate equality.
    [[nodiscard]] constexpr bool ApproxEqual(const Quaternion& rhs,
                                              T eps = Constants<T>::EPSILON) const noexcept
    {
        return Math::ApproxEqual(x, rhs.x, eps)
            && Math::ApproxEqual(y, rhs.y, eps)
            && Math::ApproxEqual(z, rhs.z, eps)
            && Math::ApproxEqual(w, rhs.w, eps);
    }

    /// Rotational equivalence — q and -q represent the same rotation.
    [[nodiscard]] constexpr bool RotationEqual(const Quaternion& rhs,
                                                T eps = Constants<T>::EPSILON) const noexcept
    {
        return ApproxEqual(rhs, eps) || ApproxEqual(-rhs, eps);
    }

    // ============================================================
    //  Arithmetic operators
    // ============================================================

    // ── Unary ────────────────────────────────────────────────────
    /// Negates all four components (gives the antipodal quaternion —
    /// same rotation, opposite hemisphere of the 4-D unit sphere).
    [[nodiscard]] constexpr Quaternion operator-() const noexcept
    {
        return {-x, -y, -z, -w};
    }

    // ── Component-wise add / subtract (used by Lerp / Slerp blend) ──
    [[nodiscard]] constexpr Quaternion operator+(const Quaternion& rhs) const noexcept
    {
        return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
    }
    [[nodiscard]] constexpr Quaternion operator-(const Quaternion& rhs) const noexcept
    {
        return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
    }

    // ── Scalar scale (used internally by Slerp) ──────────────────
    [[nodiscard]] constexpr Quaternion operator*(T s) const noexcept
    {
        return {x * s, y * s, z * s, w * s};
    }
    [[nodiscard]] constexpr Quaternion operator/(T s) const noexcept
    {
        return {x / s, y / s, z / s, w / s};
    }
    [[nodiscard]] friend constexpr Quaternion operator*(T s, const Quaternion& q) noexcept
    {
        return q * s;
    }

    // ── Quaternion multiplication  (Hamilton product) ────────────
    //
    //  q1 * q2 computes the rotation q2 applied FIRST, then q1.
    //  (Same right-to-left convention as matrix multiplication.)
    //
    //  Formula:
    //    w = w1*w2 - x1*x2 - y1*y2 - z1*z2
    //    x = w1*x2 + x1*w2 + y1*z2 - z1*y2
    //    y = w1*y2 - x1*z2 + y1*w2 + z1*x2
    //    z = w1*z2 + x1*y2 - y1*x2 + z1*w2
    //
    [[nodiscard]] constexpr Quaternion operator*(const Quaternion& rhs) const noexcept
    {
        return {
            w*rhs.x + x*rhs.w + y*rhs.z - z*rhs.y,   // x
            w*rhs.y - x*rhs.z + y*rhs.w + z*rhs.x,   // y
            w*rhs.z + x*rhs.y - y*rhs.x + z*rhs.w,   // z
            w*rhs.w - x*rhs.x - y*rhs.y - z*rhs.z    // w
        };
    }
    constexpr Quaternion& operator*=(const Quaternion& rhs) noexcept
    {
        *this = *this * rhs;
        return *this;
    }

    // ── Compound add/scale (used by Lerp internals) ──────────────
    constexpr Quaternion& operator+=(const Quaternion& rhs) noexcept
    {
        x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this;
    }
    constexpr Quaternion& operator*=(T s) noexcept
    {
        x *= s; y *= s; z *= s; w *= s; return *this;
    }

    // ============================================================
    //  Core operations
    // ============================================================

    /// Squared magnitude — constexpr, avoids sqrt.
    [[nodiscard]] constexpr T LengthSq() const noexcept
    {
        return x*x + y*y + z*z + w*w;
    }

    /// Magnitude (should be ≈ 1 for a unit quaternion).
    [[nodiscard]] T Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    // ── Dot product ──────────────────────────────────────────────
    //  Geometric dot product on the 4-D unit sphere.
    //  |dot(q1, q2)| == 1  →  same rotation.
    //  dot(q1, q2) close to  0  →  90° apart.
    [[nodiscard]] constexpr T Dot(const Quaternion& rhs) const noexcept
    {
        return x*rhs.x + y*rhs.y + z*rhs.z + w*rhs.w;
    }

    // ── Conjugate  (q*) ──────────────────────────────────────────
    //  Negates the vector part: (x,y,z,w) → (-x,-y,-z,w).
    //  For a unit quaternion, conjugate == inverse.
    [[nodiscard]] constexpr Quaternion Conjugate() const noexcept
    {
        return {-x, -y, -z, w};
    }

    // ── Normalize ────────────────────────────────────────────────
    //  Returns the unit-length quaternion.  Asserts in debug if
    //  already zero (degenerate input).
    [[nodiscard]] Quaternion Normalized() const noexcept
    {
        const T len = Length();
        assert(len > Constants<T>::EPSILON
               && "Quaternion::Normalized() called on zero-length quaternion");
        return *this / len;
    }

    /// Normalize in-place.
    Quaternion& Normalize() noexcept
    {
        *this = Normalized();
        return *this;
    }

    // ── Inverse  (q⁻¹) ───────────────────────────────────────────
    //  For a unit quaternion:  inverse == conjugate.
    //  For a general quaternion:  q⁻¹ = q* / |q|²
    [[nodiscard]] constexpr Quaternion Inverse() const noexcept
    {
        const T lenSq = LengthSq();
        assert(lenSq > Constants<T>::EPSILON
               && "Quaternion::Inverse() called on zero quaternion");
        return Conjugate() / lenSq;
    }

    // ============================================================
    //  Vector rotation
    // ============================================================

    //  Rotate a Vec3 by this (assumed unit) quaternion.
    //
    //  Classic formula:  v' = q * (0,v) * q*
    //
    //  Optimised "Rodrigues-style" form (only 15 muls, 15 adds):
    //    t  = 2 * cross(q.xyz, v)
    //    v' = v + w*t + cross(q.xyz, t)
    //
    //  This avoids constructing two full quaternion products and is
    //  branch-free; the compiler will auto-vectorise it nicely.
    //
    [[nodiscard]] constexpr Vec3<T> operator*(const Vec3<T>& v) const noexcept
    {
        const Vec3<T> qv{x, y, z};
        const Vec3<T> t  = qv.Cross(v) * T{2};
        return v + t * w + qv.Cross(t);
    }

    // ============================================================
    //  Factory: SetAxisAngle / FromAxisAngle
    // ============================================================

    //  Builds a quaternion representing a rotation of `angle` radians
    //  around `axis`.  `axis` must be unit-length.
    //
    //  Formula:  q = { axis * sin(angle/2),  cos(angle/2) }
    //
    [[nodiscard]] static Quaternion FromAxisAngle(const Vec3<T>& axis, T angle) noexcept
    {
        const T half    = angle * T{0.5};
        const T sinHalf = std::sin(half);
        const T cosHalf = std::cos(half);
        return { axis.x * sinHalf,
                 axis.y * sinHalf,
                 axis.z * sinHalf,
                 cosHalf };
    }

    /// Alias matching the naming convention requested in the spec.
    static void SetAxisAngle(Quaternion& out,
                              const Vec3<T>& axis, T angle) noexcept
    {
        out = FromAxisAngle(axis, angle);
    }

    // ============================================================
    //  GetAxisAngle
    // ============================================================

    //  Extracts the axis and angle from a unit quaternion.
    //
    //  Degeneracy: when w ≈ ±1 the rotation angle is ≈ 0 and the
    //  axis is arbitrary — we return (0,1,0) as a sensible fallback.
    //
    void GetAxisAngle(Vec3<T>& axisOut, T& angleOut) const noexcept
    {
        // Clamp w to [-1,1] to guard against floating-point overshoot
        // before acos.
        const T clamped = Math::Clamp(w, T{-1}, T{1});
        angleOut = T{2} * std::acos(clamped);

        const T sinHalfSq = T{1} - clamped * clamped;
        if (sinHalfSq <= Constants<T>::EPSILON)
        {
            // Angle ≈ 0 — axis is degenerate, return canonical up axis.
            axisOut = Vec3<T>::UnitY();
            return;
        }
        const T invSinHalf = T{1} / std::sqrt(sinHalfSq);
        axisOut = { x * invSinHalf, y * invSinHalf, z * invSinHalf };
    }

    // ============================================================
    //  Factory: Euler angles  (intrinsic ZYX — roll/pitch/yaw)
    // ============================================================

    //  Builds a quaternion from Euler angles given in radians:
    //    `pitch` — rotation around X  (nose up/down)
    //    `yaw`   — rotation around Y  (nose left/right)
    //    `roll`  — rotation around Z  (bank / tilt)
    //
    //  Application order: roll first (Z), then pitch (X), then yaw (Y).
    //  Equivalent to:  Qy(yaw) * Qx(pitch) * Qz(roll)
    //
    //  Using the half-angle product shortcut avoids three full
    //  quaternion multiplications.
    //
    [[nodiscard]] static Quaternion FromEuler(T pitch, T yaw, T roll) noexcept
    {
        const T hp = pitch * T{0.5};
        const T hy = yaw   * T{0.5};
        const T hr = roll  * T{0.5};

        const T cp = std::cos(hp), sp = std::sin(hp);
        const T cy = std::cos(hy), sy = std::sin(hy);
        const T cr = std::cos(hr), sr = std::sin(hr);

        //  ZYX: Qy * Qx * Qz   (yaw last so yaw is world-up rotation)
        return {
            cy*sp*cr + sy*cp*sr,   // x
            sy*cp*cr - cy*sp*sr,   // y
            cy*cp*sr - sy*sp*cr,   // z
            cy*cp*cr + sy*sp*sr    // w
        };
    }

    /// Convenience overload accepting a Vec3{pitch, yaw, roll}.
    [[nodiscard]] static Quaternion FromEuler(const Vec3<T>& pyr) noexcept
    {
        return FromEuler(pyr.x, pyr.y, pyr.z);
    }

    // ============================================================
    //  Conversion: ToEuler
    // ============================================================

    //  Extracts Euler angles (pitch, yaw, roll) in radians from a
    //  unit quaternion (ZYX / intrinsic order, matching FromEuler).
    //
    //  Returns Vec3{pitch, yaw, roll}.
    //
    //  Gimbal-lock singularity (pitch ≈ ±90°) is handled: yaw is
    //  set to 0 and all rotation is absorbed into roll.
    //
    [[nodiscard]] Vec3<T> ToEuler() const noexcept
    {
        // Derived from the Ry*Rx*Rz rotation matrix (matching FromEuler's convention).
        // Maps quaternion components directly to the matrix elements that encode each angle:
        //
        //   m(2,1) = 2*(y*z - w*x)  =  -sin(pitch)
        //   m(2,0) = 2*(x*z + w*y)  =   sin(yaw)*cos(pitch)
        //   m(2,2) = 1 - 2*(x²+y²)  =   cos(yaw)*cos(pitch)
        //   m(0,1) = 2*(x*y + w*z)  =   cos(pitch)*sin(roll)
        //   m(1,1) = 1 - 2*(x²+z²)  =   cos(pitch)*cos(roll)

        // Pitch (X-axis rotation)
        const T sinP  = T{-2} * (y*z - w*x);
        const T pitch = std::asin(Math::Clamp(sinP, T{-1}, T{1}));

        // Gimbal-lock guard: |sin(pitch)| ~= 1 => yaw and roll degenerate.
        if (Math::Abs(sinP) > T{1} - Constants<T>::EPSILON)
        {
            const T roll = std::atan2(T{-2} * (x*z - w*y),
                                       T{1}  - T{2} * (y*y + z*z));
            return { pitch, T{0}, roll };
        }

        // Yaw (Y-axis rotation)
        const T yaw = std::atan2(T{2} * (x*z + w*y),
                                  T{1} - T{2} * (x*x + y*y));

        // Roll (Z-axis rotation)
        const T roll = std::atan2(T{2} * (x*y + w*z),
                                   T{1} - T{2} * (x*x + z*z));

        return { pitch, yaw, roll };
    }

    // ============================================================
    //  Conversion: ToMat4x4
    // ============================================================

    //  Converts a unit quaternion to a 4×4 rotation matrix
    //  (column-major, matching Mat4x4<T>).
    //
    //  Formula (from unit quaternion q = {x, y, z, w}):
    //
    //   [ 1-2(y²+z²)   2(xy-wz)    2(xz+wy)   0 ]
    //   [ 2(xy+wz)     1-2(x²+z²)  2(yz-wx)   0 ]
    //   [ 2(xz-wy)     2(yz+wx)    1-2(x²+y²) 0 ]
    //   [ 0            0           0           1 ]
    //
    [[nodiscard]] constexpr Mat4x4<T> ToMat4x4() const noexcept
    {
        const T xx = x*x, yy = y*y, zz = z*z;
        const T xy = x*y, xz = x*z, yz = y*z;
        const T wx = w*x, wy = w*y, wz = w*z;

        const T one = T{1}, two = T{2};

        // Column vectors (column-major)
        return Mat4x4<T>{
            Vec4<T>{ one - two*(yy+zz),  two*(xy+wz),   two*(xz-wy),  T{0} },  // col 0
            Vec4<T>{ two*(xy-wz),         one-two*(xx+zz), two*(yz+wx), T{0} },  // col 1
            Vec4<T>{ two*(xz+wy),         two*(yz-wx),   one-two*(xx+yy), T{0}}, // col 2
            Vec4<T>{ T{0},                T{0},          T{0},         one  }    // col 3
        };
    }

    // ============================================================
    //  Factory: FromMat4x4
    // ============================================================

    //  Extracts a unit quaternion from the upper-left 3×3 of a
    //  column-major rotation matrix.
    //
    //  Uses Shepperd's method: pick the largest diagonal component
    //  to maximise numerical stability, then derive the others.
    //
    [[nodiscard]] static Quaternion FromMat4x4(const Mat4x4<T>& m) noexcept
    {
        // m(col, row) — column-major
        const T m00 = m(0,0), m11 = m(1,1), m22 = m(2,2);
        const T trace = m00 + m11 + m22;

        Quaternion q;

        if (trace > T{0})
        {
            const T s = std::sqrt(trace + T{1}) * T{2};   // s = 4w
            q.w = T{0.25} * s;
            q.x = (m(1,2) - m(2,1)) / s;
            q.y = (m(2,0) - m(0,2)) / s;
            q.z = (m(0,1) - m(1,0)) / s;
        }
        else if (m00 > m11 && m00 > m22)
        {
            const T s = std::sqrt(T{1} + m00 - m11 - m22) * T{2};   // s = 4x
            q.w = (m(1,2) - m(2,1)) / s;
            q.x = T{0.25} * s;
            q.y = (m(0,1) + m(1,0)) / s;
            q.z = (m(2,0) + m(0,2)) / s;
        }
        else if (m11 > m22)
        {
            const T s = std::sqrt(T{1} + m11 - m00 - m22) * T{2};   // s = 4y
            q.w = (m(2,0) - m(0,2)) / s;
            q.x = (m(0,1) + m(1,0)) / s;
            q.y = T{0.25} * s;
            q.z = (m(1,2) + m(2,1)) / s;
        }
        else
        {
            const T s = std::sqrt(T{1} + m22 - m00 - m11) * T{2};   // s = 4z
            q.w = (m(0,1) - m(1,0)) / s;
            q.x = (m(2,0) + m(0,2)) / s;
            q.y = (m(1,2) + m(2,1)) / s;
            q.z = T{0.25} * s;
        }

        return q.Normalized();
    }

    // ============================================================
    //  Interpolation
    // ============================================================

    // ── NLerp  (normalised linear interpolation) ─────────────────
    //
    //  Cheap, constant-angular-velocity approximation for small
    //  angles.  Always chooses the shortest arc (negates q2 if the
    //  dot product is negative).
    //
    //  Prefer Slerp when smooth, constant-speed interpolation over
    //  large angles is required.
    //
    [[nodiscard]] static Quaternion NLerp(Quaternion a,
                                           Quaternion b, T t) noexcept
    {
        // Shortest-path correction
        if (a.Dot(b) < T{0}) b = -b;

        return Quaternion{
            Math::Lerp(a.x, b.x, t),
            Math::Lerp(a.y, b.y, t),
            Math::Lerp(a.z, b.z, t),
            Math::Lerp(a.w, b.w, t)
        }.Normalized();
    }

    /// Lerp is an alias for NLerp — matches the naming convention of
    /// Vec2/3/4 while still producing a unit quaternion.
    [[nodiscard]] static Quaternion Lerp(const Quaternion& a,
                                          const Quaternion& b, T t) noexcept
    {
        return NLerp(a, b, t);
    }

    // ── Slerp  (spherical linear interpolation) ──────────────────
    //
    //  Produces constant angular velocity and always takes the
    //  shortest arc on the 4-D unit sphere.
    //
    //  Formula:
    //    θ₀ = acos(dot(q1, q2))
    //    q  = (sin((1-t)·θ₀) / sin(θ₀)) * q1
    //       + (sin(t·θ₀)     / sin(θ₀)) * q2
    //
    //  Falls back to NLerp when θ₀ < SLERP_THRESHOLD to avoid
    //  division by sin(≈0).
    //
    [[nodiscard]] static Quaternion Slerp(Quaternion a,
                                           Quaternion b, T t) noexcept
    {
        constexpr T SLERP_THRESHOLD = static_cast<T>(0.9995);

        T dot = a.Dot(b);

        // Shortest-path: if dot < 0 the long way around would be taken,
        // so flip b to stay on the same hemisphere.
        if (dot < T{0})
        {
            b   = -b;
            dot = -dot;
        }

        // When quaternions are very close, linear interpolation is safe
        // and avoids the near-zero division in the general formula.
        if (dot > SLERP_THRESHOLD)
            return NLerp(a, b, t);

        // General spherical interpolation
        dot = Math::Clamp(dot, T{-1}, T{1});
        const T theta0    = std::acos(dot);          // angle between inputs
        const T theta     = theta0 * t;              // angle to interpolate
        const T sinTheta0 = std::sin(theta0);
        const T sinTheta  = std::sin(theta);

        const T s1 = std::cos(theta) - dot * sinTheta / sinTheta0;
        const T s2 = sinTheta / sinTheta0;

        return (a * s1 + b * s2).Normalized();
    }

    // ── SquadSetup / Squad  (Spherical Cubic interpolation) ──────
    //
    //  C1-continuous spline through a sequence of rotations.
    //  Tangent quaternions for keyframe i are:
    //    Si = qi * exp( -( log(qi⁻¹ * qi₊₁) + log(qi⁻¹ * qi₋₁) ) / 4 )
    //
    //  For convenience: use SquadTangent() to precompute the inner
    //  control points, then call Squad() for each frame.
    //
    //  Reference: Shoemake, SIGGRAPH 1985.
    //
    [[nodiscard]] static Quaternion SquadTangent(const Quaternion& qPrev,
                                                   const Quaternion& q,
                                                   const Quaternion& qNext) noexcept
    {
        // log(q) = { axis * theta,  0 }  for a unit quaternion
        // exp(q) = FromAxisAngle(q.xyz.Normalized(), q.xyz.Length())
        //
        // Approximate via the double-slerp formula (Shoemake):
        //   S_i = q_i * exp(-( log(qi⁻¹ * q_{i+1}) + log(qi⁻¹ * q_{i-1}) ) / 4)
        //
        const Quaternion qInv     = q.Conjugate();
        const Quaternion ql       = LogQ(qInv * qPrev);
        const Quaternion qr       = LogQ(qInv * qNext);
        const Quaternion sum      = (ql + qr) * T{-0.25};
        return (q * ExpQ(sum)).Normalized();
    }

    /// Spherical cubic (Squad) interpolation between q1 → q2
    /// using precomputed inner control points s1 and s2.
    /// t ∈ [0, 1].
    [[nodiscard]] static Quaternion Squad(const Quaternion& q1,
                                           const Quaternion& q2,
                                           const Quaternion& s1,
                                           const Quaternion& s2,
                                           T t) noexcept
    {
        return Slerp(Slerp(q1, q2, t), Slerp(s1, s2, t), T{2} * t * (T{1} - t));
    }

    // ============================================================
    //  Utility
    // ============================================================

    /// Returns the angular difference (in radians) between two quaternions.
    /// Result is always in [0, π].
    ///
    /// Normalises the dot product by both lengths before acos so that equal
    /// (not necessarily unit) quaternions always return exactly 0 — even
    /// when sin²+cos² is not perfectly 1.0 in floating-point arithmetic.
    [[nodiscard]] T AngleTo(const Quaternion& other) const noexcept
    {
        const T len2 = std::sqrt(LengthSq() * other.LengthSq());
        if (len2 < Constants<T>::EPSILON) return T{0};
        // Abs handles the double-cover ambiguity (q and -q same rotation).
        const T d = Math::Abs(Dot(other)) / len2;
        return T{2} * std::acos(Math::Clamp(d, T{0}, T{1}));
    }

    // ============================================================
    //  Stream output
    // ============================================================

    friend std::ostream& operator<<(std::ostream& os, const Quaternion& q)
    {
        return os << "Quat(" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << ")";
    }

    // ============================================================
    //  Private helpers for Squad
    // ============================================================

private:
    // Log of a unit quaternion:  log(q) = {axis * theta,  0}
    // where theta = acos(clamp(w, -1, 1)).
    [[nodiscard]] static Quaternion LogQ(const Quaternion& q) noexcept
    {
        const T theta    = std::acos(Math::Clamp(q.w, T{-1}, T{1}));
        const T sinTheta = std::sin(theta);
        if (Math::Abs(sinTheta) < Constants<T>::EPSILON)
            return Quaternion{q.x, q.y, q.z, T{0}};
        const T scale = theta / sinTheta;
        return Quaternion{ q.x * scale, q.y * scale, q.z * scale, T{0} };
    }

    // Exp of a pure quaternion (w == 0):  exp(q) = {axis*sin(θ), cos(θ)}
    // where θ = |q.xyz|.
    [[nodiscard]] static Quaternion ExpQ(const Quaternion& q) noexcept
    {
        const T theta = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z);
        if (theta < Constants<T>::EPSILON)
            return Quaternion{q.x, q.y, q.z, T{1}};
        const T sinTheta = std::sin(theta);
        const T scale    = sinTheta / theta;
        return Quaternion{ q.x*scale, q.y*scale, q.z*scale, std::cos(theta) };
    }

}; // struct Quaternion

// ================================================================
//  Free-function aliases
// ================================================================

template <typename T>
[[nodiscard]] inline Quaternion<T> Normalize(const Quaternion<T>& q) noexcept
{
    return q.Normalized();
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> Conjugate(const Quaternion<T>& q) noexcept
{
    return q.Conjugate();
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> Inverse(const Quaternion<T>& q) noexcept
{
    return q.Inverse();
}

template <typename T>
[[nodiscard]] inline Quaternion<T> Slerp(const Quaternion<T>& a,
                                          const Quaternion<T>& b, T t) noexcept
{
    return Quaternion<T>::Slerp(a, b, t);
}

template <typename T>
[[nodiscard]] inline Quaternion<T> Lerp(const Quaternion<T>& a,
                                         const Quaternion<T>& b, T t) noexcept
{
    return Quaternion<T>::Lerp(a, b, t);
}

// ================================================================
//  Aliases
// ================================================================
using Quatf = Quaternion<float>;
using Quatd = Quaternion<double>;

// ================================================================
//  Layout guarantees
// ================================================================
static_assert(sizeof(Quatf)  == 16, "Quaternion<float>  must be 16 bytes");
static_assert(alignof(Quatf) == 16, "Quaternion<float>  must be 16-byte aligned");
static_assert(sizeof(Quatd)  == 32, "Quaternion<double> must be 32 bytes");

} // namespace Math