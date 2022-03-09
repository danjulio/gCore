/** @file Vec3.h */
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


#ifndef _TGX_VEC3_H_
#define _TGX_VEC3_H_

// only C++, no plain C
#ifdef __cplusplus


#include <stdint.h>

#include "Misc.h"
#include "Vec2.h"


namespace tgx
{


    /** forward declarations */

    template<typename T> struct Vec2;
    
    template<typename T> struct Vec3;

    template<typename T> struct Vec4;


    /** Specializations */

    typedef Vec3<int> iVec3;            // integer valued 2-D vector with platform int

    typedef Vec3<int16_t> iVec3_s16;    // integer valued 2-D vector with 16 bit int

    typedef Vec3<int32_t> iVec3_s32;    // integer valued 2-D vector with 32 bit int

    typedef Vec3<float>   fVec3;        // floating point value 2-D vector with float precision

    typedef Vec3<double>  dVec3;        // floating point value 2-D vector with double precision




    /********************************************
    * template class for a 4-d vector
    *********************************************/
    template<typename T> struct Vec3 : public Vec2<T>
        {

        // first two coordinates from the base class
        using Vec2<T>::x;
        using Vec2<T>::y;

        // and the new third coordinates
        T z;


        /**
        * Default ctor. Uninitialized components. 
        **/
        Vec3() {}


        /**
        * ctor. Explicit initialization
        **/
        constexpr Vec3(T X, T Y, T Z) : Vec2<T>(X,Y), z(Z)
            {
            }


        /**
        * ctor from a Vec2
        **/
        constexpr Vec3(Vec2<T> V, T Z) : Vec2<T>(V), z(Z)
            {
            }


        /**
        * default copy ctor.
        **/
        Vec3(const Vec3 & V) = default;


        /**
        * Default assignment operator.
        **/
        Vec3 & operator=(const Vec3 & V) = default;



        /**
        * Explicit conversion to another vector
        **/
        template<typename U>
        explicit operator Vec3<U>() { return Vec3<U>((U)x, (U)y, (U)z); }


        /**
        * Implicit conversion to floating point type.
        **/
        operator Vec3<typename DefaultFPType<T>::fptype>() { return Vec3<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)x, (typename DefaultFPType<T>::fptype)y, (typename DefaultFPType<T>::fptype)z); }



        /**
         * Equality comparator. True if both component are equal. 
         **/
        inline bool operator==(const Vec3 V) const 
            {
            return ((x == V.x) && (y == V.y) && (z == V.z));
            }


        /**
         * Inequality operator.
         **/
        inline bool operator!=(const Vec3 V) const
            { 
            return(!(operator==(V))); 
            }


        /**
         * Less-than comparison operator. Lexicographical order.
         **/
        inline bool operator<(const Vec3 V) const 
            { 
            if (x < V.x) return true;
            if (x == V.x)
                {
                if (y < V.y) return true;
                if (y == V.y)
                    {
                    if (z < V.z) return true;                    
                    }
                }
            return false;
            }


        /**
         * Less-than-or-equal comparison operator. Lexicographical order.
         **/
        inline bool operator<=(const Vec3 V) const
            {
            if (x < V.x) return true;
            if (x == V.x)
                {
                if (y < V.y) return true;
                if (y == V.y)
                    {
                    if (z <= V.z) return true;
                    }
                }
            return false;
            }
            

        /**
         * Greater-than comparison operator. Lexicographical order.
         **/
        inline bool operator>(const Vec3 V) const 
            { 
            return(!(operator<=(V))); 
            }


        /**
         * Greater-than-or-equal comparison operator. Lexicographical order.
         * @return true if the first parameter is greater than or equal to the second.
         **/
        inline bool operator>=(const Vec3 V) const
            { 
            return(!(operator<(V))); 
            }


        /**
        * Add another vector to this one.
        **/
        inline void operator+=(const Vec3 V)
            { 
            x += V.x;
            y += V.y;
            z += V.z;
            }


        /**
        * Substract another vector from this one.
        **/
        inline void operator-=(const Vec3 V) 
            {
            x -= V.x;
            y -= V.y;
            z -= V.z;
            }


        /**
         * Multiply this vector by another one. 
         * Coordinate by coordinate multiplication.
         **/
        inline void operator*=(const Vec3 V)
            {
            x *= V.x;
            y *= V.y;
            z *= V.z;
            }


        /**
         * Divide this vector by another one.
         * Coordinate by coordinate division.
         **/
        inline void operator/=(const Vec3 V) 
            {
            x /= V.x;
            y /= V.y;
            z /= V.z;
            }


        /**
         * scalar addition. Add v to each of the vector components. 
         **/
        inline void operator+=(const T v) 
            {
            x += v;
            y += v;
            z += v;
            }


        /**
         * scalar substraction. Add v to each of the vector components.
         **/
        inline void operator-=(const T v) 
            {
            x -= v;
            y -= v;
            z -= v;
            }

        /**
         * scalar multiplication. Multiply each of the vector components by v.
         **/
        inline void operator*=(const T v)
            {
            x *= v;
            y *= v;
            z *= v;
            }

        /**
         * scalar division. Divde each of the vector components by v.
         **/
        inline void operator/=(const T v)
            {
            x /= v;
            y /= v;
            z /= v;
            }


        /**
         * unary negation operator
         **/
        inline Vec3 operator-() const
            {
            return Vec3{ -x, -y, -z };
            }



        /**
         * Compute the squared euclidian norm of the vector (as type T).
        **/
        inline T norm2() const
            { 
            return x*x + y*y +z*z; 
            }


        /**
         * Compute the euclidian norm of the vector (return a Tfloat).
        * Tfloat selects the floating point type used for computation.
         **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat norm() const
            { 
            return (Tfloat)sqrt((Tfloat)(x*x + y*y +z*z));
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
                }
            }


        /**
        * Return the normalize vector, return the same vector if it is 0.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec3<T> getNormalize() const
            { 
            Vec3<T> V(*this);
            V.normalize();
            return V;
            }





#ifdef TGX_ON_ARDUINO

        /***
        * Print the vector using a given stream object.
        **/
        inline void print(Stream & outputStream = Serial) const
            {
            outputStream.printf("[%.3f \t %.3f \t %.3f]\n", x, y, z);
            }
#endif


    };





        /**
        * Compute the squared euclidian distance between two vectors. Return as type T.
        **/
        template<typename T> inline T dist2(const Vec3<T>  V1, const Vec3<T>  V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            const T zz = V1.z - V2.z;
            return xx * xx + yy * yy + zz * zz;
            }


        /**
         * Compute the euclidian distance between two vectors, return as Tfloat.
         **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > Tfloat dist(Vec3<T> V1, const Vec3<T> V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            const T zz = V1.z - V2.z;
            return (Tfloat)sqrt((Tfloat)(xx * xx + yy * yy + zz * zz));
            }


        /**
         * Addition operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec3<T> operator+(Vec3<T> V1, const Vec3<T> V2) { V1 += V2; return V1; }


        /**
         * Substraction operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec3<T> operator-(Vec3<T> V1, const Vec3<T> V2) { V1 -= V2; return V1; }


        /**
         * Multiplication operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec3<T> operator*(Vec3<T> V1, const Vec3<T> V2) { V1 *= V2; return V1; }


        /**
         * Division operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec3<T> operator/(Vec3<T> V1, const Vec3<T> V2) { V1 /= V2; return V1; }


        /**
         * Scalar addition operator.
         **/
        template<typename T> inline Vec3<T> operator+(const T a, Vec3<T> V) { V += a; return V; }
        template<typename T> inline Vec3<T> operator+(Vec3<T> V, const T a) { V += a; return V; }


        /**
         * Scalar substraction operator.
         **/
        template<typename T> inline Vec3<T> operator-(const T a, Vec3<T> V) { V -= a; return V; }
        template<typename T> inline Vec3<T> operator-(Vec3<T> V, const T a) { V -= a; return V; }


        /**
         * Scalar multiplication operator.
         **/
        template<typename T> inline Vec3<T> operator*(const T a, Vec3<T> V) { V *= a; return V; }
        template<typename T> inline Vec3<T> operator*(Vec3<T> V, const T a) { V *= a; return V; }


        /**
         * Scalar division operator.
         **/
        template<typename T> inline Vec3<T> operator/(const T a, Vec3<T> V) { V /= a; return V; }
        template<typename T> inline Vec3<T> operator/(Vec3<T> V, const T a) { V /= a; return V; }


        /**
         * Return the dot product U.V between two vectors.
         **/
        template<typename T> inline T dotProduct(const Vec3<T> U, const Vec3<T> V) 
            { 
            return U.x * V.x + U.y * V.y + U.z * V.z;
            }


        /**
        * Return the cross product of u x v.
        **/
        template<typename T> inline Vec3<T> crossProduct(const Vec3<T> & U, const Vec3<T> & V) 
            { 
            return Vec3<T>{ U.y * V.z - U.z * V.y,
                            U.z * V.x - U.x * V.z,
                            U.x * V.y - U.y * V.x };
            }


        /**
        * Linear interpolation: V1 + alpha(V2 - V1).
        **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec3<T> lerp(Tfloat alpha, Vec3<T> V1, Vec3<T> V2)
            {
            return Vec3<T>{ (T)(V1.x + alpha * (V2.x - V1.x)),
                            (T)(V1.y + alpha * (V2.y - V1.y)),
                            (T)(V1.z + alpha * (V2.z - V1.z)) };
            }


}

#endif

#endif

/** end of file */

