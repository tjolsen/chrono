#include "chrono_parallel/math/mat33.h"
#include "chrono_parallel/math/real3.h"
#include "chrono_parallel/math/simd.h"
namespace chrono {

// dot product of each column of a matrix with itself
inline __m256d DotMM(const real* M) {
    __m256d a = _mm256_loadu_pd(&M[0]);  // Load first column of M
    __m256d b = _mm256_loadu_pd(&M[4]);  // Load second column of M
    __m256d c = _mm256_loadu_pd(&M[8]);  // Load third column of M

    __m256d xy0 = _mm256_mul_pd(a, a);
    __m256d xy1 = _mm256_mul_pd(b, b);
    __m256d xy2 = _mm256_mul_pd(c, c);
    __m256d xy3 = _mm256_set1_pd(0);  // last dot prod is a dud

    // low to high: xy00+xy01 xy10+xy11 xy02+xy03 xy12+xy13
    __m256d temp01 = _mm256_hadd_pd(xy0, xy1);

    // low to high: xy20+xy21 xy30+xy31 xy22+xy23 xy32+xy33
    __m256d temp23 = _mm256_hadd_pd(xy2, xy3);

    // low to high: xy02+xy03 xy12+xy13 xy20+xy21 xy30+xy31
    __m256d swapped = _mm256_permute2f128_pd(temp01, temp23, 0x21);

    // low to high: xy00+xy01 xy10+xy11 xy22+xy23 xy32+xy33
    __m256d blended = _mm256_blend_pd(temp01, temp23, 0b1100);

    __m256d dotproduct = _mm256_add_pd(swapped, blended);
    return dotproduct;
}
// http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
inline Mat33 MulMM(const real* a, const real* b) {
    Mat33 r;
    __m256d a_line, b_line, r_line;
    int i, j;
    for (i = 0; i < 12; i += 4) {
        // unroll the first step of the loop to avoid having to initialize r_line to zero
        a_line = _mm256_load_pd(a);              // a_line = vec4(column(a, 0))
        b_line = _mm256_set1_pd(b[i]);           // b_line = vec4(b[i][0])
        r_line = _mm256_mul_pd(a_line, b_line);  // r_line = a_line * b_line
        for (j = 1; j < 3; j++) {
            a_line = _mm256_loadu_pd(&a[j * 4]);  // a_line = vec4(column(a, j))
            b_line = _mm256_set1_pd(b[i + j]);    // b_line = vec4(b[i][j])
                                                  // r_line += a_line * b_line
            r_line = _mm256_add_pd(_mm256_mul_pd(a_line, b_line), r_line);
        }
        _mm256_storeu_pd(&r.array[i], r_line);  // r[i] = r_line
    }
    return r;
}

inline Mat33 MulM_TM(const real* M, const real* N) {
    Mat33 result;
    __m256d a = _mm256_loadu_pd(&M[0]);  // Load first column of M
    __m256d b = _mm256_loadu_pd(&M[4]);  // Load second column of M
    __m256d c = _mm256_loadu_pd(&M[8]);  // Load third column of M

    for (int i = 0; i < 3; i++) {
        __m256d v = _mm256_loadu_pd(&N[i * 4]);  // load ith column column of N
        __m256d xy0 = _mm256_mul_pd(v, a);
        __m256d xy1 = _mm256_mul_pd(v, b);
        __m256d xy2 = _mm256_mul_pd(v, c);
        __m256d xy3 = _mm256_set1_pd(0);  // last dot prod is not used

        __m256d temp01 = _mm256_hadd_pd(xy0, xy1);  // low to high: xy00+xy01 xy10+xy11 xy02+xy03 xy12+xy13
        __m256d temp23 = _mm256_hadd_pd(xy2, xy3);  // low to high: xy20+xy21 xy30+xy31 xy22+xy23 xy32+xy33
        // low to high: xy02+xy03 xy12+xy13 xy20+xy21 xy30+xy31
        __m256d swapped = _mm256_permute2f128_pd(temp01, temp23, 0x21);
        // low to high: xy00+xy01 xy10+xy11 xy22+xy23 xy32+xy33
        __m256d blended = _mm256_blend_pd(temp01, temp23, 0b1100);
        __m256d dotproduct = _mm256_add_pd(swapped, blended);
        _mm256_storeu_pd(&result.array[i * 4], dotproduct);
    }
    return result;
}

inline real3 MulMV(const real* a, const real* b) {
    real3 r;
    __m256d v1 = _mm256_load_pd(&a[0]) * _mm256_set1_pd(b[0]);
    __m256d v2 = _mm256_load_pd(&a[4]) * _mm256_set1_pd(b[1]);
    __m256d v3 = _mm256_load_pd(&a[8]) * _mm256_set1_pd(b[2]);
    __m256d out = _mm256_add_pd(_mm256_add_pd(v1, v2), v3);
    _mm256_storeu_pd(&r.array[0], out);

    return r;
}

inline Mat33 OuterProductVV(const real* a, const real* b) {
    Mat33 r;
    __m256d u = _mm256_loadu_pd(a);  // Load the first vector
    __m256d col;
    int i;
    for (i = 0; i < 3; i++) {
        col = _mm256_mul_pd(u, _mm256_set1_pd(b[i]));
        _mm256_storeu_pd(&r.array[i * 4], col);
    }
    return r;
}

inline Mat33 ScaleMat(const real* a, const real b) {
    Mat33 r;
    __m256d s = _mm256_set1_pd(b);
    __m256d c, col;
    int i;
    for (i = 0; i < 3; i++) {
        c = _mm256_loadu_pd(&a[i * 4]);
        col = _mm256_mul_pd(c, s);
        _mm256_storeu_pd(&r.array[i * 4], col);
    }
    return r;
}

inline SymMat33 NormalEquations(const real* M) {
    SymMat33 result;
    __m256d a = _mm256_loadu_pd(&M[0]);  // Load first column of M
    __m256d b = _mm256_loadu_pd(&M[4]);  // Load second column of M
    __m256d c = _mm256_loadu_pd(&M[8]);  // Load third column of M

    __m256d xy0 = _mm256_mul_pd(a, a);
    __m256d xy1 = _mm256_mul_pd(a, b);
    __m256d xy2 = _mm256_mul_pd(a, c);
    __m256d xy3 = _mm256_mul_pd(b, b);

    __m256d xy4 = _mm256_mul_pd(b, c);
    __m256d xy5 = _mm256_mul_pd(c, c);
    __m256d xy6 = _mm256_set1_pd(0);
    __m256d xy7 = _mm256_set1_pd(0);

    __m256d temp01 = _mm256_hadd_pd(xy0, xy1);
    __m256d temp23 = _mm256_hadd_pd(xy2, xy3);
    __m256d swapped = _mm256_permute2f128_pd(temp01, temp23, 0x21);
    __m256d blended = _mm256_blend_pd(temp01, temp23, 0b1100);
    __m256d dotproduct = _mm256_add_pd(swapped, blended);

    _mm256_storeu_pd(&result.array[0], dotproduct);

    temp01 = _mm256_hadd_pd(xy4, xy5);
    temp23 = _mm256_hadd_pd(xy6, xy7);  // This should be zero
    swapped = _mm256_permute2f128_pd(temp01, temp23, 0x21);
    blended = _mm256_blend_pd(temp01, temp23, 0b1100);
    dotproduct = _mm256_add_pd(swapped, blended);
    _mm256_storeu_pd(&result.array[4], dotproduct);
    return result;
}

//[0,4,8 ]
//[1,5,9 ]
//[2,6,10]
//[3,7,11]
real3 operator*(const Mat33& M, const real3& v) {
    return MulMV(M.array, v.array);
}

Mat33 operator*(const Mat33& N, const real scale) {
    return ScaleMat(N.array, scale);
}

Mat33 operator*(const Mat33& M, const Mat33& N) {
    return MulMM(M.array, N.array);
}
Mat33 operator+(const Mat33& M, const Mat33& N) {
    return Mat33(M[0] + N[0], M[1] + N[1], M[2] + N[2], M[4] + N[4], M[5] + N[5], M[6] + N[6], M[8] + N[8], M[9] + N[9],
                 M[10] + N[10]);
}

Mat33 operator-(const Mat33& M, const Mat33& N) {
    return Mat33(M[0] - N[0], M[1] - N[1], M[2] - N[2], M[4] - N[4], M[5] - N[5], M[6] - N[6], M[8] - N[8], M[9] - N[9],
                 M[10] - N[10]);
}
Mat33 operator-(const Mat33& M) {
    return Mat33(-M[0], -M[1], -M[2], -M[4], -M[5], -M[6], -M[8], -M[9], -M[10]);
}
Mat33 Abs(const Mat33& m) {
    return Mat33(simd::Abs(m.cols[0]), simd::Abs(m.cols[1]), simd::Abs(m.cols[2]));
}
Mat33 SkewSymmetric(const real3& r) {
    return Mat33(real3(0, r[2], -r[1]), real3(-r[2], 0, r[0]), real3(r[1], -r[0], 0));
}

Mat33 MultTranspose(const Mat33& M, const Mat33& N) {
    return Mat33(M * real3(N[0], N[4], N[8]),  //
                 M * real3(N[1], N[5], N[9]),  //
                 M * real3(N[2], N[6], N[10]));
}

Mat33 TransposeMult(const Mat33& M, const Mat33& N) {
    return MulM_TM(M.array, N.array);
}

Mat33 Transpose(const Mat33& a) {
    return Mat33(a[0], a[4], a[8], a[1], a[5], a[9], a[2], a[6], a[10]);
}

real Trace(const Mat33& m) {
    return m[0] + m[5] + m[10];
}
// Multiply a 3x1 by a 1x3 to get a 3x3
Mat33 OuterProduct(const real3& a, const real3& b) {
    return OuterProductVV(a.array, b.array);
}

real InnerProduct(const Mat33& A, const Mat33& B) {
    return Dot(A.cols[0], B.cols[0]) + Dot(A.cols[1], B.cols[1]) + Dot(A.cols[0], B.cols[0]);
}

Mat33 Adjoint(const Mat33& A) {
    Mat33 T;
    T[0] = A[5] * A[10] - A[9] * A[6];
    T[1] = -A[1] * A[10] + A[9] * A[2];
    T[2] = A[1] * A[6] - A[5] * A[2];

    T[4] = -A[4] * A[10] + A[8] * A[6];
    T[5] = A[0] * A[10] - A[8] * A[2];
    T[6] = -A[0] * A[6] + A[4] * A[2];

    T[8] = A[4] * A[9] - A[8] * A[5];
    T[9] = -A[0] * A[9] + A[8] * A[1];
    T[10] = A[0] * A[5] - A[4] * A[1];
    return T;
}

Mat33 AdjointTranspose(const Mat33& A) {
    Mat33 T;
    T[0] = A[5] * A[10] - A[9] * A[6];
    T[1] = -A[4] * A[10] + A[8] * A[6];
    T[2] = A[4] * A[9] - A[8] * A[5];

    T[4] = -A[1] * A[10] + A[9] * A[2];
    T[5] = A[0] * A[10] - A[8] * A[2];
    T[6] = -A[0] * A[9] + A[8] * A[1];

    T[8] = A[1] * A[6] - A[5] * A[2];
    T[9] = -A[0] * A[6] + A[4] * A[2];
    T[10] = A[0] * A[5] - A[4] * A[1];

    return T;
}

real Determinant(const Mat33& m) {
    return m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) +
           m[8] * (m[1] * m[6] - m[5] * m[2]);
}

Mat33 Inverse(const Mat33& A) {
    real s = Determinant(A);
    // if (s > 0.0) {
    return Adjoint(A) * real(1.0 / s);
    //} else {
    //    return Mat33(0);
    //}
}

// Same as inverse but we store it transposed
Mat33 InverseTranspose(const Mat33& A) {
    real s = Determinant(A);
    if (s > 0.0) {
        return AdjointTranspose(A) * real(1.0 / s);
    } else {
        return Mat33(0);
    }
}

real Norm(const Mat33& A) {
    return Sqrt(Trace(A * Transpose(A)));
}

real3 LargestColumnNormalized(const Mat33& A) {
    real3 scale = DotMM(A.array);
    real3 sqrt_scale = simd::SquareRoot(scale);
    if (scale.x > scale.y) {
        if (scale.x > scale.z) {
            return A.cols[0] / sqrt_scale.x;
        }
    } else if (scale.y > scale.z) {
        return A.cols[1] / sqrt_scale.y;
    }
    return A.cols[2] / sqrt_scale.z;
}
//// ========================================================================================

Mat33 operator*(const DiagMat33& M, const Mat33& N) {
    return Mat33(real3(M.x11 * N.cols[0].x, M.x22 * N.cols[0].y, M.x33 * N.cols[0].z),
                 real3(M.x11 * N.cols[1].x, M.x22 * N.cols[1].y, M.x33 * N.cols[1].z),
                 real3(M.x11 * N.cols[2].x, M.x22 * N.cols[2].y, M.x33 * N.cols[2].z));
}
real3 operator*(const DiagMat33& M, const real3& v) {
    real3 result;
    result.x = M.x11 * v.x;
    result.y = M.x22 * v.y;
    result.z = M.x33 * v.z;
    return result;
}
//// ========================================================================================
SymMat33 operator-(const SymMat33& M, const real& v) {
    return SymMat33(M.x11 - v, M.x21, M.x31, M.x22 - v, M.x32, M.x33 - v);  // only subtract diagonal
}
SymMat33 CofactorMatrix(const SymMat33& A) {
    SymMat33 T;
    T.x11 = A.x22 * A.x33 - A.x32 * A.x32;   //
    T.x21 = -A.x21 * A.x33 + A.x32 * A.x31;  //
    T.x22 = A.x11 * A.x33 - A.x31 * A.x31;   //
    T.x31 = A.x21 * A.x32 - A.x22 * A.x31;   //
    T.x32 = -A.x11 * A.x32 + A.x21 * A.x31;  //
    T.x33 = A.x11 * A.x22 - A.x21 * A.x21;   //
    return T;
}
real3 LargestColumnNormalized(const SymMat33& A) {
    real scale1 = Length2(real3(A.x11, A.x21, A.x31));
    real scale2 = Length2(real3(A.x21, A.x22, A.x32));
    real scale3 = Length2(real3(A.x31, A.x32, A.x33));
    if (scale1 > scale2) {
        if (scale1 > scale3) {
            return real3(A.x11, A.x21, A.x31) / sqrt(scale1);
        }
    } else if (scale2 > scale3) {
        return real3(A.x21, A.x22, A.x32) / sqrt(scale2);
    }
    if (scale3 > 0)
        return real3(A.x31, A.x32, A.x33) / sqrt(scale3);
    else {
        return (real3(1, 0, 0));
    }
}
SymMat33 NormalEquationsMatrix(const Mat33& A) {
    return NormalEquations(A.array);
}
//// ========================================================================================

real3 operator*(const Mat32& M, const real2& v) {
    real3 result;
    result.x = M[0] * v.x + M[4] * v.y;
    result.y = M[1] * v.x + M[5] * v.y;
    result.z = M[2] * v.x + M[6] * v.y;

    return result;
}
Mat32 operator*(const SymMat33& M, const Mat32& N) {
    Mat32 result;
    // x11 x21 x31  c11 c12
    // x21 x22 x32  c21 c22
    // x31 x32 x33  c31 c32
    result[0] = M.x11 * N[0] + M.x21 * N[1] + M.x31 * N[2];
    result[1] = M.x21 * N[0] + M.x22 * N[1] + M.x32 * N[2];
    result[2] = M.x31 * N[0] + M.x32 * N[1] + M.x33 * N[2];

    result[4] = M.x11 * N[4] + M.x21 * N[5] + M.x31 * N[6];
    result[5] = M.x21 * N[4] + M.x22 * N[5] + M.x32 * N[6];
    result[6] = M.x31 * N[4] + M.x32 * N[5] + M.x33 * N[6];

    return result;
}
//// ========================================================================================
SymMat22 operator-(const SymMat22& M, const real& v) {
    return SymMat22(M.x11 - v, M.x21, M.x22 - v);  //
}
SymMat22 CofactorMatrix(const SymMat22& A) {
    SymMat22 T;
    T.x11 = A.x22;   //
    T.x21 = -A.x21;  //
    T.x22 = A.x11;   //
    return T;
}
real2 LargestColumnNormalized(const SymMat22& A) {
    real scale1 = Length2(real2(A.x11, A.x21));
    real scale2 = Length2(real2(A.x21, A.x22));
    if (scale1 > scale2) {
        return real2(A.x11, A.x21) / sqrt(scale1);
    } else if (scale2 > 0) {
        return real2(A.x21, A.x22) / sqrt(scale2);
    } else {
        return real2(1, 0);
    }
}

// A^T*B
SymMat22 TransposeTimesWithSymmetricResult(const Mat32& A, const Mat32& B) {
    SymMat22 T;
    T.x11 = Dot(real3(A[0], A[1], A[2]), real3(B[0], A[1], A[2]));
    T.x21 = Dot(real3(A[4], A[5], A[6]), real3(B[0], A[1], A[2]));
    T.x22 = Dot(real3(A[4], A[5], A[6]), real3(B[4], A[5], A[6]));
    return T;
}

SymMat22 ConjugateWithTranspose(const Mat32& A, const SymMat33& B) {
    return TransposeTimesWithSymmetricResult(B * A, A);
}
}