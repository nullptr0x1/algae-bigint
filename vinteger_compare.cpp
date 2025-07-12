#include "vinteger.h"

namespace algae
{
    inline int __int_sign(const std::int64_t x) {
        if(x > 0)
            return 1;
        
        return x ? -1 : 0;
    }

    inline int __int_sign(const std::uint64_t x) {
        return x ? 1 : 0;
    }

    inline std::int64_t __set_int_sign(const std::uint64_t x, int sign) {
        return sign > 0 ? x : -((std::int64_t)x);
    }

    std::strong_ordering vinteger::__compare_template(const vinteger& a, const std::int64_t b)
    {
        if(auto r = a.sign() <=> __int_sign(b) ; r != std::strong_ordering::equal || (a.empty() && b == 0))
            return r;

        if(a.value_bit_width() >= __CUtype_bit_length - 1)
            return std::strong_ordering::greater;

        return __set_int_sign(a.__buffer[0], a.sign()) <=> b;
    }

    std::strong_ordering vinteger::__compare_template(const vinteger& a, const std::uint64_t b)
    {
        if(auto r = a.sign() <=> __int_sign(b); r != std::strong_ordering::equal || (a.empty() && b == 0)) 
            return r;

        if(a.value_bit_width() >= __CUtype_bit_length)
            return std::strong_ordering::greater;

        return a.__buffer[0] * a.sign() <=> b;
    }


    

    std::strong_ordering operator <=>(const vinteger& a, const vinteger& b)
    {
        if(auto r = a.__bit_length <=> b.__bit_length; r != std::strong_ordering::equal || (a.empty() && b.empty()))
            return r;

        for(int i = a.__value_length() - 1; i >= 0; --i)
            if(auto r = a.__buffer[i] <=> b.__buffer[i]; r != std::strong_ordering::equal)
                return r;

        return std::strong_ordering::equal;
    }
}