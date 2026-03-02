#pragma once
#include <cmath>
#include <cassert>
#include <iostream>

template<int n> struct vec {
    double data[n] = {0};
    double& operator[](const int i)       { assert(i>=0 && i<n); return data[i]; }
    double  operator[](const int i) const { assert(i>=0 && i<n); return data[i]; }
};

template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i=0; i<n; i++) out << v[i] << " ";
    return out;
}

template<> struct vec<2> {
    double x = 0, y = 0;
    double& operator[](const int i)       { assert(i>=0 && i<2); return i ? y : x; }
    double  operator[](const int i) const { assert(i>=0 && i<2); return i ? y: x; }
    vec<2>() = default;
    vec<2>(const double& x_, const double& y_):x(x_),y(y_){}
    vec<2> operator+(const vec<2>& v) const {
        return vec<2>{x+v.x, y+v.y};
    }
    vec<2> operator-(const vec<2>& v) const {
        return vec<2>{x-v.x, y-v.y};
    }
    double dot(const vec<2>& v) const {
        return x * v.x + y * v.y;
    }

    double cross(const vec<2>& v) const {
        return x * v.y - y * v.x;
    }
};


template<> struct vec<3> {
    double x = 0, y = 0, z = 0;
    double& operator[](const int i)       { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    double  operator[](const int i) const { assert(i>=0 && i<3); return i ? (1==i ? y : z) : x; }
    vec<3>() = default;
    vec<3>(const double& x_, const double& y_, const double& z_):x(x_),y(y_),z(z_){}
    vec<3> operator+(const vec<3>& v) const {
        return vec<3>{x+v.x, y+v.y, z+v.z};
    }
    vec<3> operator-(const vec<3>& v) const {
        return vec<3>{x-v.x, y-v.y, z-v.z};
    }
    double dot(const vec<3>& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    vec<3> cross(const vec<3>& v) const {
        return vec<3>{y * v.z - z * v.y,z * v.x - x * v.z,x * v.y - y * v.x };
    }
};

template<>
struct vec<4> {
    double x = 0, y = 0, z = 0, w = 0;
    constexpr vec<4>() = default;
    constexpr vec<4>(double x_, double y_, double z_, double w_ = 1.0): x(x_), y(y_), z(z_), w(w_) {}
    double& operator[](int i) {
        assert(i >= 0 && i < 4);
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return w;
        }
    }
    double operator[](int i) const {
        assert(i >= 0 && i < 4);
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return w; // i == 3
        }
    }
    vec<4> operator+(const vec<4>& v) const {
        return vec<4>{x + v.x, y + v.y, z + v.z, w + v.w};
    }
    vec<4> operator-(const vec<4>& v) const {
        return vec<4>{x - v.x, y - v.y, z - v.z, w - v.w};
    }
    double dot(const vec<4>& v) const {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }
};

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;