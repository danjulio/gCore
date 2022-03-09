/** @file Vec4.h */
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


#ifndef _TGX_VEC4_H_
#define _TGX_VEC4_H_

// only C++, no plain C
#ifdef __cplusplus



#include <stdint.h>

#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"


namespace tgx
{


    /** forward declarations */

    template<typename T> struct Vec2;    

    template<typename T> struct Vec3;
   
    template<typename T> struct Vec4;



    /** Specializations */

    typedef Vec4<int> iVec4;            // integer valued 2-D vector with platform int

    typedef Vec4<int16_t> iVec4_s16;    // integer valued 2-D vector with 16 bit int

    typedef Vec4<int32_t> iVec4_s32;    // integer valued 2-D vector with 32 bit int

    typedef Vec4<float>   fVec4;        // floating point value 2-D vector with float precision

    typedef Vec4<double>  dVec4;        // floating point value 2-D vector with double precision




    /********************************************
    * template class for a 4-d vector
    *********************************************/
    template<typename T> struct Vec4 : public Vec3<T>
        {

        // first three coordinates from base classes
        using Vec2<T>::x;
        using Vec2<T>::y;
        using Vec3<T>::z;

        // and the new fourth coordinates
        T w;


        /**
        * Default ctor. Uninitialized components. 
        **/
        Vec4() {}


        /**
        * ctor. Explicit initialization
        **/
        constexpr Vec4(T X, T Y, T Z, T W) : Vec3<T>(X,Y,Z), w(W)
            {
            }


        /**
        * default copy ctor.
        **/
        Vec4(const Vec4 & V) = default;


        /**
        * ctor from a Vec2
        **/
        constexpr Vec4(Vec2<T> V, T Z, T W) : Vec3<T>(V, Z), w(W)
            {
            }


        /**
        * ctor from a Vec3
        **/
        constexpr Vec4(Vec3<T> V, T W) : Vec3<T>(V), w(W)
            {
            }


        /**
        * Default assignment operator.
        **/
        Vec4 & operator=(const Vec4 & V) = default;



        /**
        * Explicit conversion to another vector
        **/
        template<typename U>
        explicit operator Vec4<U>() { return Vec4<U>((U)x, (U)y, (U)z, (U)w); }



        /**
        * Implicit conversion to floating point type.
        **/
        operator Vec4<typename DefaultFPType<T>::fptype>() { return Vec4<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)x, (typename DefaultFPType<T>::fptype)y, (typename DefaultFPType<T>::fptype)z, (typename DefaultFPType<T>::fptype)w); }



        /**
         * Equality comparator. True if both component are equal. 
         **/
        inline bool operator==(const Vec4 V) const 
            {
            return ((x == V.x) && (y == V.y) && (z == V.z) && (w == V.w));
            }


        /**
         * Inequality operator.
         **/
        inline bool operator!=(const Vec4 V) const
            { 
            return(!(operator==(V))); 
            }


        /**
         * Less-than comparison operator. Lexicographical order.
         **/
        inline bool operator<(const Vec4 V) const 
            { 
            if (x < V.x) return true;
            if (x == V.x)
                {
                if (y < V.y) return true;
                if (y == V.y)
                    {
                    if (z < V.z) return true;
                    if (z == V.z) return true;
                        {
                        if (w < V.w) return true;
                        }
                    }
                }
            return false;
            }


        /**
         * Less-than-or-equal comparison operator. Lexicographical order.
         **/
        inline bool operator<=(const Vec4 V) const
            {
            if (x < V.x) return true;
            if (x == V.x)
                {
                if (y < V.y) return true;
                if (y == V.y)
                    {
                    if (z < V.z) return true;
                    if (z == V.z) return true;
                        {
                        if (w <= V.w) return true;
                        }
                    }
                }
            return false;
            }
            

        /**
         * Greater-than comparison operator. Lexicographical order.
         **/
        inline bool operator>(const Vec4 V) const 
            { 
            return(!(operator<=(V))); 
            }


        /**
         * Greater-than-or-equal comparison operator. Lexicographical order.
         * @return true if the first parameter is greater than or equal to the second.
         **/
        inline bool operator>=(const Vec4 V) const
            { 
            return(!(operator<(V))); 
            }


        /**
        * Add another vector to this one.
        **/
        inline void operator+=(const Vec4 V)
            { 
            x += V.x;
            y += V.y;
            z += V.z;
            w += V.w;
            }


        /**
        * Substract another vector from this one.
        **/
        inline void operator-=(const Vec4 V) 
            {
            x -= V.x;
            y -= V.y;
            z -= V.z;
            w -= V.w;
            }


        /**
         * Multiply this vector by another one. 
         * Coordinate by coordinate multiplication.
         **/
        inline void operator*=(const Vec4 V) 
            {
            x *= V.x;
            y *= V.y;
            z *= V.z;
            w *= V.w;
            }


        /**
         * Divide this vector by another one.
         * Coordinate by coordinate division.
         **/
        inline void operator/=(const Vec4 V)
            {
            x /= V.x;
            y /= V.y;
            z /= V.z;
            w /= V.w;
            }


        /**
         * scalar addition. Add v to each of the vector components. 
         **/
        inline void operator+=(const T v) 
            {
            x += v;
            y += v;
            z += v;
            w += v;
            }


        /**
         * scalar substraction. Add v to each of the vector components.
         **/
        inline void operator-=(const T v) 
            {
            x -= v;
            y -= v;
            z -= v;
            w -= v;
            }

        /**
         * scalar multiplication. Multiply each of the vector components by v.
         **/
        inline void operator*=(const T v) 
            {
            x *= v;
            y *= v;
            z *= v;
            w *= v;
            }

        /**
         * scalar division. Divde each of the vector components by v.
         **/
        inline void operator/=(const T v) 
            {
            x /= v;
            y /= v;
            z /= v;
            w /= v;
            }


        /**
         * unary negation operator
         * DO NOT DEFINE FOR PROJECTIVE VECTORS 
         * TO PREVENT SPUPID MISTAKES WHEN CASTING... 
         **/
        /*
        inline Vec4 operator-() const
            {
            return Vec4{ -x, -y, -z, w };
            }
        */

        /**
         * Compute the squared euclidian norm of the vector (as type T).
        **/
        inline T norm2() const 
            { 
            return x*x + y*y +z*z +w*w; 
            }


        /**
         * Compute the euclidian norm of the vector (return a Tfloat).
        * Tfloat selects the floating point type used for computation.
         **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat norm() const
            { 
            return (Tfloat)sqrt((Tfloat)(x*x + y*y + z*z + w*w));
            }


        /**
        * Normalise the vector so that its norm is 1, does nothing if the vector is 0.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void normalize()
            { 
            Tfloat a = norm<Tfloat>(); 
            if (a > 0) 
                {
                x = (T)(x / a);
                y = (T)(y / a);
                z = (T)(z / a);
                w = (T)(w / a);
                }
            }


        /**
        * Return the normalize vector, return the same vector if it is 0.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec4<T> getNormalize() const
            { 
            Vec4<T> V(*this);
            V.normalize();
            return V;
            }








        /**
        * Performs the 'z-divide' operation: 
        * x  <-  x/w
        * y  <-  x/w
        * z  <-  z/w
        * w  <-  1/w
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void zdivide() 
            {
            const float iw = 1 / w;
            x = iw*x;
            y = iw*y;
            z = iw*z;
            w = iw;
            }


#ifdef TGX_ON_ARDUINO

        /***
        * Print the vector using a given stream object.
        **/
        inline void print(Stream & outputStream = Serial) const
            {
            outputStream.printf("[%.3f \t %.3f \t %.3f \t %.3f]\n", x, y, z, w);
            }

#endif 

    };




        /**
        * Return the normalized vector, does nothing if the vector is 0.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline  Vec4<T> normalize(Vec4<T> V)
            {
            V.normalize<Tfloat>();
            return V;
            }


        /**
        * Compute the squared euclidian distance between two vectors. Return as type T.
        **/
        template<typename T> inline T dist2(const Vec4<T>  V1, const Vec4<T>  V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            const T zz = V1.z - V2.z;
            const T ww = V1.w - V2.w;
            return xx * xx + yy * yy + zz * zz + ww * ww;
            }


        /**
         * Compute the euclidian distance between two vectors, return as Tfloat.
         **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > Tfloat dist(Vec4<T> V1, const Vec4<T> V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            const T zz = V1.z - V2.z;
            const T ww = V1.w - V2.w;
            return (Tfloat)sqrt((Tfloat)(xx * xx + yy * yy + zz * zz + ww * ww));
            }


        /**
         * Addition operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec4<T> operator+(Vec4<T> V1, const Vec4<T> V2) { V1 += V2; return V1; }


        /**
         * Substraction operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec4<T> operator-(Vec4<T> V1, const Vec4<T> V2) { V1 -= V2; return V1; }


        /**
         * Multiplication operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec4<T> operator*(Vec4<T> V1, const Vec4<T> V2) { V1 *= V2; return V1; }


        /**
         * Division operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec4<T> operator/(Vec4<T> V1, const Vec4<T> V2) { V1 /= V2; return V1; }


        /**
         * Scalar addition operator.
         **/
        template<typename T> inline Vec4<T> operator+(const T a, Vec4<T> V) { V += a; return V; }
        template<typename T> inline Vec4<T> operator+(Vec4<T> V, const T a) { V += a; return V; }


        /**
         * Scalar substraction operator.
         **/
        template<typename T> inline Vec4<T> operator-(const T a, Vec4<T> V) { V -= a; return V; }
        template<typename T> inline Vec4<T> operator-(Vec4<T> V, const T a) { V -= a; return V; }


        /**
         * Scalar multiplication operator.
         **/
        template<typename T> inline Vec4<T> operator*(const T a, Vec4<T> V) { V *= a; return V; }
        template<typename T> inline Vec4<T> operator*(Vec4<T> V, const T a) { V *= a; return V; }


        /**
         * Scalar division operator.
         **/
        template<typename T> inline Vec4<T> operator/(const T a, Vec4<T> V) { V /= a; return V; }
        template<typename T> inline Vec4<T> operator/(Vec4<T> V, const T a) { V /= a; return V; }


        /**
         * Return the dot product U.V between two vectors.
         **/
        template<typename T> inline T dotProduct(const Vec4<T> U, const Vec4<T> V) 
            { 
            return U.x * V.x + U.y * V.y + U.z * V.z + U.w * V.w;
            }


        /**
        * Return the cross product of u x v as 3-dim vector(returned component w = 0)
        **/
        template<typename T> inline Vec4<T> crossProduct(const Vec4<T> & U, const Vec4<T> & V) 
            { 
            return Vec4<T>{ U.y * V.z - U.z * V.y,
                            U.z * V.x - U.x * V.z,
                            U.x * V.y - U.y * V.x,
                            0 };
            }



        /**
        * Linear interpolation: V1 + alpha(V2 - V1).
        **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec4<T> lerp(Tfloat alpha, Vec4<T> V1, Vec4<T> V2)
            {
            return Vec4<T>{ (T)(V1.x + alpha * (V2.x - V1.x)),
                            (T)(V1.y + alpha * (V2.y - V1.y)),
                            (T)(V1.z + alpha * (V2.z - V1.z)),
                            (T)(V1.w + alpha * (V2.w - V1.w)) };
            }


}

#endif

#endif

/** end of file */

