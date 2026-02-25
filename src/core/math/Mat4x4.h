#pragma once

// ================================================================
//  Mat4x4.h
//  Column-major 4×4 matrix for 3-D graphics and transforms.
//
//  Depends on:  VecTypes.h  →  MathUtils.h
//
//  Memory layout (column-major — identical to OpenGL / GLSL / HLSL
//  row-major upload):
//
//      cols[0]  cols[1]  cols[2]  cols[3]
//      [ m00     m01      m02      m03  ]   ← row 0
//      [ m10     m11      m12      m13  ]   ← row 1
//      [ m20     m21      m22      m23  ]   ← row 2
//      [ m30     m31      m32      m33  ]   ← row 3
//
//  Indexing:  cols[col][row]   e.g. cols[3][1] == m13  (translation Y)
//
//  Raw float pointer (&cols[0].x) can be passed directly to
//  glUniformMatrix4fv(..., GL_FALSE, ptr).
//
//  Design goals
//  ─────────────
//  • constexpr everywhere std::sin/cos/sqrt/tan are not required.
//  • inline (not constexpr) for transcendental factory methods.
//  • [[nodiscard]] on every pure function.
//  • alignas(16) so arrays of Mat4x4f are SIMD-friendly.
//  • Analytic inverse (cofactor / adjugate) — no heap, no pivoting.
//
//  Requires C++17 or later.
// ================================================================

#include "VecTypes.h"

#include <cmath>
#include <cassert>
#include <ostream>
#include <iomanip>

namespace Math
{

template <typename T = float>
struct alignas(16) Mat4x4
{
    static_assert(std::is_floating_point_v<T>,
                  "Mat4x4<T> requires a floating-point scalar.");

    // ============================================================
    // Data  —  four column vectors, stored contiguously
    // ============================================================
    Vec4<T> cols[4];

    // ============================================================
    // Construction
    // ============================================================

    /// Default: zero matrix (not identity — use Mat4x4::Identity()).
    constexpr Mat4x4() noexcept : cols{} {}

    /// Construct from four explicit column vectors.
    constexpr Mat4x4(const Vec4<T>& c0,
                     const Vec4<T>& c1,
                     const Vec4<T>& c2,
                     const Vec4<T>& c3) noexcept
        : cols{c0, c1, c2, c3} {}

    /// Construct from 16 scalars in column-major order
    /// (c0r0, c0r1, c0r2, c0r3, c1r0, …).
    constexpr Mat4x4(
        T c0r0, T c0r1, T c0r2, T c0r3,
        T c1r0, T c1r1, T c1r2, T c1r3,
        T c2r0, T c2r1, T c2r2, T c2r3,
        T c3r0, T c3r1, T c3r2, T c3r3) noexcept
        : cols{
            Vec4<T>{c0r0, c0r1, c0r2, c0r3},
            Vec4<T>{c1r0, c1r1, c1r2, c1r3},
            Vec4<T>{c2r0, c2r1, c2r2, c2r3},
            Vec4<T>{c3r0, c3r1, c3r2, c3r3}}
    {}

    // ============================================================
    // Element access  —  (col, row)  and  raw pointer
    // ============================================================

    /// cols[col][row] — column-major convention.
    [[nodiscard]] constexpr T& operator()(int col, int row) noexcept
    {
        return cols[col][row];
    }
    [[nodiscard]] constexpr const T& operator()(int col, int row) const noexcept
    {
        return cols[col][row];
    }

    /// Direct column access.
    [[nodiscard]] constexpr Vec4<T>&       operator[](int col)       noexcept { return cols[col]; }
    [[nodiscard]] constexpr const Vec4<T>& operator[](int col) const noexcept { return cols[col]; }

    /// Raw float pointer (column-major) for GPU upload.
    [[nodiscard]]       T* Data()       noexcept { return &cols[0].x; }
    [[nodiscard]] const T* Data() const noexcept { return &cols[0].x; }

    // ============================================================
    // Comparison
    // ============================================================

    [[nodiscard]] constexpr bool operator==(const Mat4x4& rhs) const noexcept
    {
        return cols[0] == rhs.cols[0] && cols[1] == rhs.cols[1]
            && cols[2] == rhs.cols[2] && cols[3] == rhs.cols[3];
    }
    [[nodiscard]] constexpr bool operator!=(const Mat4x4& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    [[nodiscard]] constexpr bool ApproxEqual(const Mat4x4& rhs,
                                              T eps = Constants<T>::EPSILON) const noexcept
    {
        return cols[0].ApproxEqual(rhs.cols[0], eps)
            && cols[1].ApproxEqual(rhs.cols[1], eps)
            && cols[2].ApproxEqual(rhs.cols[2], eps)
            && cols[3].ApproxEqual(rhs.cols[3], eps);
    }

    // ============================================================
    //  Arithmetic operators
    // ============================================================

    // ── Mat × Mat ────────────────────────────────────────────────
    //
    //  Column-major multiply: result column j = M * rhs.cols[j]
    //  which expands to the standard dot-of-row-with-column formula.
    //
    [[nodiscard]] constexpr Mat4x4 operator*(const Mat4x4& rhs) const noexcept
    {
        Mat4x4 out;
        for (int col = 0; col < 4; ++col)
        {
            // Each output column is this matrix applied to rhs column.
            for (int row = 0; row < 4; ++row)
            {
                out.cols[col][row] =
                    cols[0][row] * rhs.cols[col][0] +
                    cols[1][row] * rhs.cols[col][1] +
                    cols[2][row] * rhs.cols[col][2] +
                    cols[3][row] * rhs.cols[col][3];
            }
        }
        return out;
    }

    constexpr Mat4x4& operator*=(const Mat4x4& rhs) noexcept
    {
        *this = *this * rhs;
        return *this;
    }

    // ── Mat × Vec4  (column vector on the right) ─────────────────
    //
    //  result[row] = dot(row_row, v)
    //
    [[nodiscard]] constexpr Vec4<T> operator*(const Vec4<T>& v) const noexcept
    {
        return {
            cols[0][0]*v.x + cols[1][0]*v.y + cols[2][0]*v.z + cols[3][0]*v.w,
            cols[0][1]*v.x + cols[1][1]*v.y + cols[2][1]*v.z + cols[3][1]*v.w,
            cols[0][2]*v.x + cols[1][2]*v.y + cols[2][2]*v.z + cols[3][2]*v.w,
            cols[0][3]*v.x + cols[1][3]*v.y + cols[2][3]*v.z + cols[3][3]*v.w,
        };
    }

    // ── Mat × scalar ─────────────────────────────────────────────
    [[nodiscard]] constexpr Mat4x4 operator*(T s) const noexcept
    {
        return { cols[0]*s, cols[1]*s, cols[2]*s, cols[3]*s };
    }
    [[nodiscard]] constexpr Mat4x4 operator/(T s) const noexcept
    {
        return { cols[0]/s, cols[1]/s, cols[2]/s, cols[3]/s };
    }
    [[nodiscard]] friend constexpr Mat4x4 operator*(T s, const Mat4x4& m) noexcept
    {
        return m * s;
    }

    // ── Mat + Mat / Mat - Mat ────────────────────────────────────
    [[nodiscard]] constexpr Mat4x4 operator+(const Mat4x4& rhs) const noexcept
    {
        return { cols[0]+rhs.cols[0], cols[1]+rhs.cols[1],
                 cols[2]+rhs.cols[2], cols[3]+rhs.cols[3] };
    }
    [[nodiscard]] constexpr Mat4x4 operator-(const Mat4x4& rhs) const noexcept
    {
        return { cols[0]-rhs.cols[0], cols[1]-rhs.cols[1],
                 cols[2]-rhs.cols[2], cols[3]-rhs.cols[3] };
    }

    // ── Unary ────────────────────────────────────────────────────
    [[nodiscard]] constexpr Mat4x4 operator-() const noexcept
    {
        return { -cols[0], -cols[1], -cols[2], -cols[3] };
    }

    // ============================================================
    //  Transform helpers
    // ============================================================

    /// Transform a point (w=1): applies translation.
    [[nodiscard]] constexpr Vec3<T> TransformPoint(const Vec3<T>& p) const noexcept
    {
        const Vec4<T> r = *this * Vec4<T>{p, T{1}};
        return Vec3<T>{r};
    }

    /// Transform a direction (w=0): ignores translation.
    [[nodiscard]] constexpr Vec3<T> TransformDir(const Vec3<T>& d) const noexcept
    {
        const Vec4<T> r = *this * Vec4<T>{d, T{0}};
        return Vec3<T>{r};
    }

    // ============================================================
    //  Core operations
    // ============================================================

    // ── Transpose ────────────────────────────────────────────────
    [[nodiscard]] constexpr Mat4x4 Transposed() const noexcept
    {
        return {
            // new col 0 = old row 0
            Vec4<T>{ cols[0][0], cols[1][0], cols[2][0], cols[3][0] },
            // new col 1 = old row 1
            Vec4<T>{ cols[0][1], cols[1][1], cols[2][1], cols[3][1] },
            // new col 2 = old row 2
            Vec4<T>{ cols[0][2], cols[1][2], cols[2][2], cols[3][2] },
            // new col 3 = old row 3
            Vec4<T>{ cols[0][3], cols[1][3], cols[2][3], cols[3][3] }
        };
    }

    // ── Determinant  (Leibniz cofactor expansion along column 0) ─
    //
    //  det(M) = Σ_i  M[0][i] * C[0][i]
    //
    //  where C[0][i] = (-1)^i * det(minor(0,i))
    //
    [[nodiscard]] constexpr T Determinant() const noexcept
    {
        // Cache column elements for readability.
        const T a00 = cols[0][0], a10 = cols[0][1], a20 = cols[0][2], a30 = cols[0][3];
        const T a01 = cols[1][0], a11 = cols[1][1], a21 = cols[1][2], a31 = cols[1][3];
        const T a02 = cols[2][0], a12 = cols[2][1], a22 = cols[2][2], a32 = cols[2][3];
        const T a03 = cols[3][0], a13 = cols[3][1], a23 = cols[3][2], a33 = cols[3][3];

        // 2×2 sub-determinants reused across cofactors (Bareiss-style cache)
        const T s0 = a22*a33 - a32*a23;
        const T s1 = a21*a33 - a31*a23;
        const T s2 = a21*a32 - a31*a22;
        const T s3 = a20*a33 - a30*a23;
        const T s4 = a20*a32 - a30*a22;
        const T s5 = a20*a31 - a30*a21;

        // Cofactors of column 0
        const T c0 =  (a11*s0 - a12*s1 + a13*s2);
        const T c1 = -(a01*s0 - a02*s1 + a03*s2);
        const T c2 =  (a01*(a12*a33-a32*a13) - a02*(a11*a33-a31*a13) + a03*(a11*a32-a31*a12));
        const T c3 = -(a01*(a12*a23-a22*a13) - a02*(a11*a23-a21*a13) + a03*s5);

        // Use the cached sub-dets for c2/c3 where possible; direct expansion is
        // clear-enough and the compiler will CSE anyway.
        (void)s3; (void)s4; (void)s5; // suppress unused warnings for alternate forms

        return a00*c0 + a10*c1 + a20*c2 + a30*c3;
    }

    // ── Analytic Inverse  (adjugate / det) ───────────────────────
    //
    //  Uses the full 4×4 cofactor matrix.  Each cofactor is the
    //  signed 3×3 minor determinant.  The adjugate is the *transpose*
    //  of the cofactor matrix, so we write each cofactor directly into
    //  the transposed position, giving adj(M) in one pass.
    //
    //  inv(M) = adj(M) / det(M)
    //
    //  Returns identity and asserts (debug) if the matrix is singular.
    //
    [[nodiscard]] constexpr Mat4x4 Inverted() const noexcept
    {
        // Unpack all 16 elements — named by (col, row).
        const T a00 = cols[0][0], a10 = cols[0][1], a20 = cols[0][2], a30 = cols[0][3];
        const T a01 = cols[1][0], a11 = cols[1][1], a21 = cols[1][2], a31 = cols[1][3];
        const T a02 = cols[2][0], a12 = cols[2][1], a22 = cols[2][2], a32 = cols[2][3];
        const T a03 = cols[3][0], a13 = cols[3][1], a23 = cols[3][2], a33 = cols[3][3];

        // ── 2×2 sub-determinants (12 unique pairs, each used ≥ 2×) ──
        const T b00 = a22*a33 - a32*a23;   const T b01 = a21*a33 - a31*a23;
        const T b02 = a21*a32 - a31*a22;   const T b03 = a20*a33 - a30*a23;
        const T b04 = a20*a32 - a30*a22;   const T b05 = a20*a31 - a30*a21;
        const T b06 = a02*a13 - a12*a03;   const T b07 = a01*a13 - a11*a03;
        const T b08 = a01*a12 - a11*a02;   const T b09 = a00*a13 - a10*a03;
        const T b10 = a00*a12 - a10*a02;   const T b11 = a00*a11 - a10*a01;

        // ── Determinant (expansion along col 0) ──────────────────
        const T det = a00*( a11*b00 - a12*b01 + a13*b02)
                    - a10*( a01*b00 - a02*b01 + a03*b02)
                    + a20*( a01*(a12*a33-a32*a13) - a02*(a11*a33-a31*a13) + a03*(a11*a32-a31*a12))
                    - a30*( a01*(a12*a23-a22*a13) - a02*(a11*a23-a21*a13) + a03*(a11*a22-a21*a12));

        assert(Math::Abs(det) > Constants<T>::EPSILON
               && "Mat4x4::Inverted() called on singular matrix");

        if (Math::Abs(det) <= Constants<T>::EPSILON)
            return Identity(); // graceful fallback in release

        const T invDet = T{1} / det;

        // ── Cofactor matrix written into transposed position ─────
        // (cofactor[col][row] → result.cols[row][col])
        Mat4x4 inv;

        inv.cols[0][0] =  (a11*b00 - a12*b01 + a13*b02) * invDet;
        inv.cols[1][0] = -(a01*b00 - a02*b01 + a03*b02) * invDet;
        inv.cols[2][0] =  (a01*(a12*a33-a32*a13) - a02*(a11*a33-a31*a13) + a03*(a11*a32-a31*a12)) * invDet;
        inv.cols[3][0] = -(a01*(a12*a23-a22*a13) - a02*(a11*a23-a21*a13) + a03*(a11*a22-a21*a12)) * invDet;

        inv.cols[0][1] = -(a10*b00 - a12*b03 + a13*b04) * invDet;
        inv.cols[1][1] =  (a00*b00 - a02*b03 + a03*b04) * invDet;
        inv.cols[2][1] = -(a00*(a12*a33-a32*a13) - a02*(a10*a33-a30*a13) + a03*(a10*a32-a30*a12)) * invDet;
        inv.cols[3][1] =  (a00*(a12*a23-a22*a13) - a02*(a10*a23-a20*a13) + a03*(a10*a22-a20*a12)) * invDet;

        inv.cols[0][2] =  (a10*b01 - a11*b03 + a13*b05) * invDet;
        inv.cols[1][2] = -(a00*b01 - a01*b03 + a03*b05) * invDet;
        inv.cols[2][2] =  (a00*(a11*a33-a31*a13) - a01*(a10*a33-a30*a13) + a03*(a10*a31-a30*a11)) * invDet;
        inv.cols[3][2] = -(a00*(a11*a23-a21*a13) - a01*(a10*a23-a20*a13) + a03*(a10*a21-a20*a11)) * invDet;

        inv.cols[0][3] = -(a10*b02 - a11*b04 + a12*b05) * invDet;
        inv.cols[1][3] =  (a00*b02 - a01*b04 + a02*b05) * invDet;
        inv.cols[2][3] = -(a00*(a11*a32-a31*a12) - a01*(a10*a32-a30*a12) + a02*(a10*a31-a30*a11)) * invDet;
        inv.cols[3][3] =  (a00*(a11*a22-a21*a12) - a01*(a10*a22-a20*a12) + a02*(a10*a21-a20*a11)) * invDet;

        // Silence unused-variable warnings on the b06..b11 batch; they are
        // handy if extending to a full symbolic check but aren't needed here
        // because we expanded the 3×3 minors directly above.
        (void)b06; (void)b07; (void)b08; (void)b09; (void)b10; (void)b11;

        return inv;
    }

    // ============================================================
    //  Factory methods
    // ============================================================

    // ── Identity ─────────────────────────────────────────────────
    [[nodiscard]] static constexpr Mat4x4 Identity() noexcept
    {
        return {
            Vec4<T>{T{1}, T{0}, T{0}, T{0}},
            Vec4<T>{T{0}, T{1}, T{0}, T{0}},
            Vec4<T>{T{0}, T{0}, T{1}, T{0}},
            Vec4<T>{T{0}, T{0}, T{0}, T{1}}
        };
    }

    // ── Translation ──────────────────────────────────────────────
    [[nodiscard]] static constexpr Mat4x4 Translation(T tx, T ty, T tz) noexcept
    {
        Mat4x4 m = Identity();
        m.cols[3] = Vec4<T>{tx, ty, tz, T{1}};
        return m;
    }
    [[nodiscard]] static constexpr Mat4x4 Translation(const Vec3<T>& t) noexcept
    {
        return Translation(t.x, t.y, t.z);
    }
    [[nodiscard]] static constexpr Mat4x4 Translate(const Vec3<T>& t) noexcept
    {
        return Translation(t);
    }

    // ── Scale ────────────────────────────────────────────────────
    [[nodiscard]] static constexpr Mat4x4 Scale(T sx, T sy, T sz) noexcept
    {
        return {
            Vec4<T>{sx,   T{0}, T{0}, T{0}},
            Vec4<T>{T{0}, sy,   T{0}, T{0}},
            Vec4<T>{T{0}, T{0}, sz,   T{0}},
            Vec4<T>{T{0}, T{0}, T{0}, T{1}}
        };
    }
    [[nodiscard]] static constexpr Mat4x4 Scale(T s) noexcept
    {
        return Scale(s, s, s);
    }
    [[nodiscard]] static constexpr Mat4x4 Scale(const Vec3<T>& s) noexcept
    {
        return Scale(s.x, s.y, s.z);
    }


    // ── RotationX ────────────────────────────────────────────────
    //
    //  Rotates counter-clockwise around +X when viewed from +X toward origin.
    //
    //  [ 1    0     0    0 ]
    //  [ 0   cos  -sin   0 ]
    //  [ 0   sin   cos   0 ]
    //  [ 0    0     0    1 ]
    //
    [[nodiscard]] static Mat4x4 RotationX(T radians) noexcept
    {
        const T c = std::cos(radians);
        const T s = std::sin(radians);
        return {
            Vec4<T>{T{1}, T{0}, T{0}, T{0}},
            Vec4<T>{T{0},    c,    s, T{0}},
            Vec4<T>{T{0},   -s,    c, T{0}},
            Vec4<T>{T{0}, T{0}, T{0}, T{1}}
        };
    }

    // ── RotationY ────────────────────────────────────────────────
    //
    //  [ cos   0   sin   0 ]
    //  [  0    1    0    0 ]
    //  [-sin   0   cos   0 ]
    //  [  0    0    0    1 ]
    //
    [[nodiscard]] static Mat4x4 RotationY(T radians) noexcept
    {
        const T c = std::cos(radians);
        const T s = std::sin(radians);
        return {
            Vec4<T>{   c, T{0},   -s, T{0}},
            Vec4<T>{T{0}, T{1}, T{0}, T{0}},
            Vec4<T>{   s, T{0},    c, T{0}},
            Vec4<T>{T{0}, T{0}, T{0}, T{1}}
        };
    }

    // ── RotationZ ────────────────────────────────────────────────
    //
    //  [ cos  -sin   0   0 ]
    //  [ sin   cos   0   0 ]
    //  [  0     0    1   0 ]
    //  [  0     0    0   1 ]
    //
    [[nodiscard]] static Mat4x4 RotationZ(T radians) noexcept
    {
        const T c = std::cos(radians);
        const T s = std::sin(radians);
        return {
            Vec4<T>{   c,    s, T{0}, T{0}},
            Vec4<T>{  -s,    c, T{0}, T{0}},
            Vec4<T>{T{0}, T{0}, T{1}, T{0}},
            Vec4<T>{T{0}, T{0}, T{0}, T{1}}
        };
    }

    // ── RotationAxis (Rodrigues' formula) ────────────────────────
    //
    //  Rotates `radians` counter-clockwise around arbitrary unit `axis`.
    //  `axis` must be normalised.
    //
    [[nodiscard]] static Mat4x4 RotationAxis(const Vec3<T>& axis, T radians) noexcept
    {
        const T c    = std::cos(radians);
        const T s    = std::sin(radians);
        const T omc  = T{1} - c;                // "one minus cos"
        const T x    = axis.x, y = axis.y, z = axis.z;

        return {
            Vec4<T>{ c + x*x*omc,       y*x*omc + z*s,   z*x*omc - y*s,  T{0} },
            Vec4<T>{ x*y*omc - z*s,     c + y*y*omc,     z*y*omc + x*s,  T{0} },
            Vec4<T>{ x*z*omc + y*s,     y*z*omc - x*s,   c + z*z*omc,    T{0} },
            Vec4<T>{ T{0},              T{0},             T{0},           T{1} }
        };
    }

    // ── LookAt (right-handed, -Z forward — OpenGL convention) ────
    //
    //  Builds a view matrix that places the camera at `eye`, looking
    //  toward `center`, with `up` defining the vertical axis.
    //
    [[nodiscard]] static Mat4x4 LookAt(const Vec3<T>& eye,
                                        const Vec3<T>& center,
                                        const Vec3<T>& up) noexcept
    {
        const Vec3<T> f = (center - eye).Normalized();   // forward (-Z)
        const Vec3<T> r = f.Cross(up).Normalized();       // right   (+X)
        const Vec3<T> u = r.Cross(f);                     // recomputed up (+Y)

        // Rotation part (rows of the view matrix are the camera's basis)
        // plus the translation dot-products fold in the eye position.
        return {
            Vec4<T>{ r.x,  u.x, -f.x, T{0} },
            Vec4<T>{ r.y,  u.y, -f.y, T{0} },
            Vec4<T>{ r.z,  u.z, -f.z, T{0} },
            Vec4<T>{ -r.Dot(eye), -u.Dot(eye), f.Dot(eye), T{1} }
        };
    }

    // ── Perspective  (right-handed, depth mapped to [-1, +1]) ────
    //
    //  `fovY`   — vertical field of view in radians
    //  `aspect` — width / height
    //  `zNear`  — near clip plane (> 0)
    //  `zFar`   — far clip plane  (> zNear)
    //
    //  Matches glm::perspective / glFrustum (OpenGL clip space).
    //
    [[nodiscard]] static Mat4x4 Perspective(T fovY, T aspect, T zNear, T zFar) noexcept
    {
        assert(Math::Abs(aspect) > Constants<T>::EPSILON && "Perspective: aspect must be non-zero");
        assert(zNear > T{0} && zFar > zNear && "Perspective: invalid near/far planes");

        const T tanHalfFov = std::tan(fovY * T{0.5});
        const T range      = zFar - zNear;

        Mat4x4 m;   // zero-initialised
        m.cols[0][0] = T{1} / (aspect * tanHalfFov);
        m.cols[1][1] = T{1} / tanHalfFov;
        m.cols[2][2] = -(zFar + zNear) / range;
        m.cols[2][3] = T{-1};
        m.cols[3][2] = -(T{2} * zFar * zNear) / range;
        return m;
    }

    // ── PerspectiveZO  (depth mapped to [0, +1] — Vulkan / DX) ──
    //
    //  Same signature as Perspective, but writes depth into [0,1].
    //
    [[nodiscard]] static Mat4x4 PerspectiveZO(T fovY, T aspect, T zNear, T zFar) noexcept
    {
        assert(Math::Abs(aspect) > Constants<T>::EPSILON);
        assert(zNear > T{0} && zFar > zNear);

        const T tanHalfFov = std::tan(fovY * T{0.5});
        const T range      = zFar - zNear;

        Mat4x4 m;
        m.cols[0][0] = T{1} / (aspect * tanHalfFov);
        m.cols[1][1] = T{1} / tanHalfFov;
        m.cols[2][2] = -zFar / range;
        m.cols[2][3] = T{-1};
        m.cols[3][2] = -(zFar * zNear) / range;
        return m;
    }

    // ── Orthographic  (right-handed, depth mapped to [-1, +1]) ───
    //
    //  Maps the box [left,right] × [bottom,top] × [zNear,zFar]
    //  into the NDC cube [-1,1]³.
    //
    [[nodiscard]] static constexpr Mat4x4 Orthographic(
        T left, T right, T bottom, T top, T zNear, T zFar) noexcept
    {
        const T rl = right  - left;
        const T tb = top    - bottom;
        const T fn = zFar   - zNear;

        return {
            Vec4<T>{ T{2}/rl,          T{0},          T{0},  T{0} },
            Vec4<T>{ T{0},          T{2}/tb,           T{0},  T{0} },
            Vec4<T>{ T{0},             T{0},       T{-2}/fn,  T{0} },
            Vec4<T>{ -(right+left)/rl, -(top+bottom)/tb, -(zFar+zNear)/fn, T{1} }
        };
    }

    // ── OrthographicZO  (depth mapped to [0, +1] — Vulkan / DX) ─
    [[nodiscard]] static constexpr Mat4x4 OrthographicZO(
        T left, T right, T bottom, T top, T zNear, T zFar) noexcept
    {
        const T rl = right  - left;
        const T tb = top    - bottom;
        const T fn = zFar   - zNear;

        return {
            Vec4<T>{ T{2}/rl,          T{0},          T{0},  T{0} },
            Vec4<T>{ T{0},          T{2}/tb,           T{0},  T{0} },
            Vec4<T>{ T{0},             T{0},       T{-1}/fn,  T{0} },
            Vec4<T>{ -(right+left)/rl, -(top+bottom)/tb, -zNear/fn, T{1} }
        };
    }

    // ============================================================
    //  Stream output  —  pretty-printed row form
    // ============================================================
    friend std::ostream& operator<<(std::ostream& os, const Mat4x4& m)
    {
        os << std::fixed << std::setprecision(4);
        os << "Mat4x4[\n";
        for (int row = 0; row < 4; ++row)
        {
            os << "  [ ";
            for (int col = 0; col < 4; ++col)
                os << std::setw(9) << m.cols[col][row] << (col < 3 ? "  " : "");
            os << " ]\n";
        }
        os << "]";
        return os;
    }

}; // struct Mat4x4

// ================================================================
//  Aliases
// ================================================================
using Mat4x4f = Mat4x4<float>;
using Mat4x4d = Mat4x4<double>;

using Mat4f = Mat4x4f;
using Mat4  = Mat4f;
using Mat4d = Mat4x4d;

// ================================================================
//  Layout guarantees
// ================================================================
static_assert(sizeof(Mat4x4f)  == 64,  "Mat4x4<float> must be 64 bytes");
static_assert(alignof(Mat4x4f) == 16,  "Mat4x4<float> must be 16-byte aligned");
static_assert(sizeof(Mat4x4d)  == 128, "Mat4x4<double> must be 128 bytes");

} // namespace Math