#include "vinteger.h"

namespace algae
{
    extern std::int64_t __set_int_sign(const std::uint64_t x, int sign);

    struct multiplier_context
    {
        using __CUtype = vinteger::__CUtype;
        constexpr static std::size_t __CUtype_bit_length = vinteger::__CUtype_bit_length;

        const vinteger * vint_max = nullptr;
        const vinteger * vint_min = nullptr;

        vinteger * output = nullptr;
        int sign = 0;

        bool pretreatment(const vinteger& x, const vinteger& y, vinteger& z) 
        {
            if(x.empty() || y.empty())
                z.clear();
            else if(x.value_bit_width() == 1)
                z = x.sign() == y.sign() ? y : -y;
            else if(y.value_bit_width() == 1)
                z = x.sign() == y.sign() ? x : -x;
            else if(x.value_bit_width() > 32 || y.value_bit_width() > 32)
                return false;
            else
            {
                z.__change_capacity(1);
                z.__buffer[0] = x.__buffer[0] * y.__buffer[0];
                z.__bit_length = __set_int_sign(std::bit_width(z.__buffer[0]), x.sign() * y.sign());
            }

            return true;
        }

        multiplier_context(const vinteger& x, const vinteger& y, vinteger& z)
        {
            if(pretreatment(x, y, z))
                return;
            
            sign = x.sign() * y.sign();
            output = &z;

            if (x >= y)
                vint_max = &x, vint_min = &y;
            else
                vint_max = &y, vint_min = &x;

            naive_multiplication();
        }

        void naive_multiplication()
        {
            for(int i = vint_min->value_bit_width() - 1; i >= 0; --i)
            {
                if(!(vint_min->__buffer[i / __CUtype_bit_length] & (1ull << __CUtype(i % __CUtype_bit_length))))
                    continue;

                *output += (*vint_max << i);
            }

            output->__bit_length = __set_int_sign(output->__bit_length, sign);
        }
    };

    vinteger& vinteger::operator*=(const vinteger& other) 
    {
        *this = *this * other;
        return *this;
    }

    vinteger operator*(const vinteger& a, const vinteger& b) 
    {
        vinteger c;
        multiplier_context(a, b, c);
        return c;
    }
}