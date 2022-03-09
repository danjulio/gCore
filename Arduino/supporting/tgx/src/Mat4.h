/** @file Mat4.h */
//
// Copyright 2020 Arvind Singh
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; If not, see <http://www.gnu.org/licenses/>.


#ifndef _TGX_MAT4_H_
#define _TGX_MAT4_H_

// only C++, no plain C
#ifdef __cplusplus


#include <stdint.h>
#include <type_traits>

#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"


namespace tgx
{

    /** forward declaration */

    template<typename T> struct Vec2;

    template<typename T> struct Vec3;

    template<typename T> struct Vec4;

    template<typename T> struct Mat4;


    /** Specializations (only floating types T make sense for matrix class). */

    typedef Mat4<float>   fMat4;        // floating point value 2-D vector with float precision

    typedef Mat4<double>  dMat4;        // floating point value 2-D vector with double precision



    /** forward declaration of matrix multiplication*/
    template<typename T> Mat4<T> operator*(const Mat4<T> & A, const Mat4<T> & B);



    /** 
    * 4x4 matrix (used for representing a 3d transform). 
    **/
    template<typename T> struct Mat4
        {

        static_assert(std::is_floating_point<T>::value, "The template parameter T of class Mat4<T> must be a floating point number.");

        // column major ordering:
        //
        // M[0]  M[4]  M[8]   M[12]
        // M[1]  M[5]  M[9]   M[13]
        // M[2]  M[6]  M[10]  M[14]
        // M[3]  M[7]  M[11]  M[15]
        //
        T M[16];  


        /** default constructor, undefined values in the matrix. */
        Mat4() {}


        /** direct assignement from 16 values */
        constexpr Mat4(T x1, T y1, T z1, T t1,
                       T x2, T y2, T z2, T t2,
                       T x3, T y3, T z3, T t3,
                       T x4, T y4, T z4, T t4) : M{ x1,x2,x3,x4, y1,y2,y3,y4, z1,z2,z3,z4, t1,t2,t3,t4 }
            {
            }


        /** Construct from an array of T*/
        Mat4(const T* mat) { memcpy(M, mat, 16 * sizeof(T)); }


        /** copy ctor */
        Mat4(const Mat4 & mat) = default;


        /** assignement operator */
        Mat4& operator=(const Mat4 & mat) = default;


        /** implicit conversion to T* */
        operator T*() { return M; }


        /** set as the nul matrix */
        void setZero()
            {
            memset(M, 0, 16 * sizeof(T));
            }


        /** set as the identity matrix */
        void setIdentity()
            {
            memset(M, 0, 16 * sizeof(T));
            M[0] = M[5] = M[10] = M[15] = ((T)1);
            }


        /** set as an orthographic matrix: 
         * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml 
         **/
        void setOrtho(T left, T right, T bottom, T top, T zNear, T zFar)
            {
            memset(M, 0, 16 * sizeof(T));
            M[0] = ((T)2) / (right - left);
            M[5] = ((T)2) / (top - bottom);
            M[10] = -((T)2) / (zFar - zNear);
            M[12] = (right + left) / (left - right);
            M[13] = (top + bottom) / (bottom - top);
            M[14] = (zFar + zNear) / (zNear - zFar);
            M[15] = (T)1;
            }


        /**
        * Set as a perspective matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
        **/
        void setFrustum(T left, T right, T bottom, T top, T zNear, T zFar)
            {
            memset(M, 0, 16 * sizeof(T));
            M[0] = (((T)2) * zNear) / (right - left);
            M[5] = (((T)2) * zNear) / (top - bottom);
            M[8] = (right + left) / (right - left);
            M[9] = (top + bottom) / (top - bottom);
            M[10] = (zFar + zNear) / (zNear - zFar);
            M[11] = -((T)1);
            M[14] = (((T)2) * zFar * zNear) / (zNear - zFar);
            }


        /**
        * Set as a perspective matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
        **/
        void setPerspective(T fovy, T aspect, T zNear, T zFar)
            {
            static const T deg2rad = (T)(M_PI / 180);
            const T aux = tan((fovy / ((T)2)) * deg2rad);
            const T top = zNear * aux;
            const T bottom = -top;
            const T right = zNear * aspect * aux;
            const T left = -right;
            setFrustum(left, right, bottom, top, zNear, zFar);
            }


        /**
        * Set as a rotation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glRotate.xml
        **/
        void setRotate(T angle, T x, T y, T z)
            {
            static const T deg2rad = (T)(M_PI / 180);
            const T norm = sqrt(x*x + y*y + z*z);
            if (norm == 0) { setIdentity(); return; }
            const T nx = x / norm;
            const T ny = y / norm;
            const T nz = z / norm;
            const T c = cos(deg2rad * angle);
            const T oneminusc = ((T)1) - c;
            const T s = sin(deg2rad * angle);

            memset(M, 0, 16 * sizeof(T));
            M[0] = nx * nx * oneminusc + c;
            M[1] = ny * nx * oneminusc + nz * s;
            M[2] = nx * nz * oneminusc - ny * s;
            M[4] = nx * ny * oneminusc - nz * s;
            M[5] = ny * ny * oneminusc + c;
            M[6] = ny * nz * oneminusc + nx * s;
            M[8] = nx * nz * oneminusc + ny * s;
            M[9] = ny * nz * oneminusc - nx * s;
            M[10] = nz * nz * oneminusc + c;
            M[15] = (T)1;
            }


        /**
        * Set as a rotation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glRotate.xml
        **/
        void setRotate(T angle, const Vec3<T> v)
            {
            setRotate(angle, v.x, v.y, v.z);
            }


        /**
        * Multiply this matrix by a rotation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glRotate.xml
        **/
        void multRotate(T angle, T x, T y, T z)
            {
            Mat4 mat;
            mat.setRotate(angle, x, y, z);
            *this = (mat * (*this));
            }


        /**
        * Multiply this matrix by a rotation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glRotate.xml
        **/
        void multRotate(T angle, const Vec3<T> v)
            {
            multRotate(angle, v.x, v.y, v.z);
            }



        /**
        * set as a translation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTranslate.xml
        **/
        void setTranslate(T x, T y, T z)
            {
            memset(M, 0, 16 * sizeof(T));
            M[0] = (T)1;
            M[5] = (T)1;
            M[10] = (T)1;
            M[12] = x;
            M[13] = y;
            M[14] = z;
            M[15] = (T)1;
            }


        /**
        * set as a translation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTranslate.xml
        **/
        void setTranslate(const Vec3<T> v)
            {
            setTranslate(v.x, v.y, v.z);
            }


        /**
        * multiply this matrix by a translation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTranslate.xml
        **/
        void multTranslate(T x, T y, T z)
            {
            Mat4 mat;
            mat.setTranslate(x, y, z);
            *this = (mat * (*this));
            }


        /**
        * multiply this matrix by a translation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTranslate.xml
        **/
        void multTranslate(const Vec3<T> v)
            {
            multTranslate(v.x, v.y, v.z);
            }



        /**
        * set as a dilatation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glScale.xml
        **/
        void setScale(T x, T y, T z)
            {
            memset(M, 0, 16 * sizeof(T));
            M[0] = x;
            M[5] = y;
            M[10] = z;
            M[15] = (T)1;
            }


        /**
        * set as a dilatation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glScale.xml
        **/
        void setScale(const Vec3<T> v)
            {
            setScale(v.x, v.y, v.z);
            }


        /**
        * multiply this matrix by a dilatation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glScale.xml
        **/
        void multScale(T x, T y, T z)
            {
            Mat4 mat;
            mat.setScale(x, y, z);
            *this = (mat * (*this));
            }


        /**
        * multiply this matrix by a dilatation matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glScale.xml
        **/
        void multScale(const Vec3<T> v)
            {
            multScale(v.x, v.y, v.z);
            }


        /**
        * invert the y axis, same as multScale({1,-1,1})
        **/
        void invertYaxis()
            {
            M[4] = -M[4];
            M[5] = -M[5];
            M[6] = -M[6];
            M[7] = -M[7];
            }


        /** 
        * Set the matrix a camera looking at a given direction.
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        * The formula given by khronos is wrong ! 
        * The cross-product of two unit vectors is not normalized LOL :-)
        * see: https://stackoverflow.com/questions/30409318/lookat-matrix-distorts-when-looking-up-or-down
        **/
        void setLookAt(T eyeX, T eyeY, T eyeZ, T centerX, T centerY, T centerZ, T upX, T upY, T upZ)
            {
            // normalise direction
            const Vec4<T> f = normalize(Vec4<T>{ centerX - eyeX, centerY - eyeY, centerZ - eyeZ, 0.0f });
            // right = direction x up
            const Vec4<T> s = crossProduct(f, Vec4<T>{ upX, upY, upZ, 0.0f });
            // normalise right. 
            const Vec4<T> sn = normalize(s);
            // up = right x direction
            const Vec4<T> u =  crossProduct(sn, f);
            memset(M, 0, 16 * sizeof(T));
            M[0] = sn.x;    M[4] = sn.y;    M[8] = sn.z;    M[12] = sn.x * eyeX + sn.y * eyeY + sn.z * eyeZ;
            M[1] = u.x;     M[5] = u.y;     M[9] = u.z;     M[13] = u.x * eyeX + u.y * eyeY + u.z * eyeZ;
            M[2] = -f.x;    M[6] = -f.y;    M[10] = -f.z;   M[14] = f.x * eyeX + f.y * eyeY + f.z * eyeZ;
            M[15] = 1.0;
            }


        /**
        * Set the matrix a camera looking at a given direction.
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        **/
        void setLookAt(const Vec3<T> eye, const Vec3<T> center, const Vec3<T> up)
            {
            setLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);
            }


        /**
        * Matrix-vector multiplication
        **/
         TGX_INLINE inline Vec4<T> mult(const Vec4<T> V) const
            {
            return Vec4<T>{ M[0] * V.x + M[4] * V.y + M[8] * V.z + M[12] * V.w,
                M[1] * V.x + M[5] * V.y + M[9] * V.z + M[13] * V.w,
                M[2] * V.x + M[6] * V.y + M[10] * V.z + M[14] * V.w,
                M[3] * V.x + M[7] * V.y + M[11] * V.z + M[15] * V.w };
            }


        /**
        * Matrix-vector multiplication
        **/
        TGX_INLINE inline Vec4<T> mult(const Vec3<T> & V, float w) const
            {
            return Vec4<T>{ M[0] * V.x + M[4] * V.y + M[8] * V.z + M[12] * w,
                M[1] * V.x + M[5] * V.y + M[9] * V.z + M[13] * w,
                M[2] * V.x + M[6] * V.y + M[10] * V.z + M[14] * w,
                M[3] * V.x + M[7] * V.y + M[11] * V.z + M[15] * w };
            }


        /**
        * Matrix-vector multiplication (last component w = 0)
        **/
        TGX_INLINE inline Vec4<T> mult0(const Vec3<T> & V) const
            {
            return Vec4<T>{ M[0] * V.x + M[4] * V.y + M[8] * V.z,
                M[1] * V.x + M[5] * V.y + M[9] * V.z,
                M[2] * V.x + M[6] * V.y + M[10] * V.z,
                M[3] * V.x + M[7] * V.y + M[11] * V.z };
            }


        /**
        * Matrix-vector multiplication (last component w = 1)
        **/
        TGX_INLINE inline Vec4<T> mult1(const Vec3<T> & V) const
            {
            return Vec4<T>{ M[0] * V.x + M[4] * V.y + M[8] * V.z + M[12],
                M[1] * V.x + M[5] * V.y + M[9] * V.z + M[13],
                M[2] * V.x + M[6] * V.y + M[10] * V.z + M[14],
                M[3] * V.x + M[7] * V.y + M[11] * V.z + M[15]};
            }


        /**
        * Matrix multiplication : (*this ) = M * (*this) 
        **/
        /*
        void operator*=(const Mat4 & M)
            {
            *this = (M * (*this));
            }
            */

        /**
        * Scalar  multiplication 
        **/
        inline void operator*=(T a)
            {
            for (int i = 0; i < 16; i++) { M[i] *= a; }
            }


#ifdef TGX_ON_ARDUINO

        inline void print(Stream & outputStream = Serial) const
            {
            outputStream.printf("%.3f  \t %.3f  \t %.3f \t  %.3f\n", M[0],M[4],M[8],M[12]);
            outputStream.printf("%.3f  \t %.3f  \t %.3f \t  %.3f\n", M[1], M[5], M[9], M[13]);
            outputStream.printf("%.3f  \t %.3f  \t %.3f \t  %.3f\n", M[2], M[6], M[10], M[14]);
            outputStream.printf("%.3f  \t %.3f  \t %.3f \t  %.3f\n\n", M[3], M[7], M[11], M[15]);
            }

#endif


        };



    /** 
    * Matrix-vector multiplication 
    **/
    template<typename T> TGX_INLINE inline Vec4<T> operator*(const Mat4<T> & M, const Vec4<T> V)
        {
        return Vec4<T>{ M.M[0] * V.x + M.M[4] * V.y + M.M[8] * V.z + M.M[12] * V.w,
                        M.M[1] * V.x + M.M[5] * V.y + M.M[9] * V.z + M.M[13] * V.w,
                        M.M[2] * V.x + M.M[6] * V.y + M.M[10] * V.z + M.M[14] * V.w,
                        M.M[3] * V.x + M.M[7] * V.y + M.M[11] * V.z + M.M[15] * V.w  };
        }



    /**
    * Matrix-matrix multiplication 
    **/
    template<typename T> inline Mat4<T> operator*(const Mat4<T> & A, const Mat4<T> & B)
        {
        Mat4<T> R; 
        for (int i = 0; i < 4; i++)
            {
            for (int j = 0; j < 4; j++)
                {
                R.M[i + j*4] = 0.0;
                for (int k = 0; k < 4; k++) { R.M[i + j * 4] += A.M[i + k * 4] * B.M[k + j * 4]; }
                }
            }
        return R;
        }


    /**
    * Scalar-matrix multiplication 
    **/
    template<typename T> inline Mat4<T> operator*(T a, const Mat4<T> & m)
        {
        Mat4<T> R(m);
        for (int i = 0; i < 16; i++) { R.M[i] *= a; }
        return R;
        }



}

#endif

#endif

/** end of file */


