#include "vinteger.h"

namespace algae
{
    extern std::int64_t __set_int_sign(const std::uint64_t x, int sign);
    extern std::size_t bit_capacity(const std::size_t bit_count, const std::size_t unit_size);
    extern vinteger::CUtype full_adder(vinteger::CUtype a, vinteger::CUtype b, bool& carry);
    extern vinteger::CUtype carry_handle(vinteger::CUtype x, bool& carry);


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



        inline bool bitget(const __CUtype* buffer, const int i) const {
            return buffer[i / __CUtype_bit_length] & (1ull << __CUtype(i % __CUtype_bit_length));
        }

        std::size_t shift_plus(const int shift) 
        {
            const std::size_t length = vint_max->__value_length();
            const std::size_t shift_unit = shift / __CUtype_bit_length;
            const std::size_t shift_bit = shift % __CUtype_bit_length;

            __CUtype overflow = 0, index = 0;
            bool carry = false;
            
            for(; index < length; ++index)
            {
                const __CUtype temporary = vint_max->__buffer[index], shift_index = index + shift_unit;
                output->__buffer[shift_index] = full_adder(output->__buffer[shift_index], overflow | (temporary << shift_bit), carry);
                overflow = temporary >> (__CUtype_bit_length - shift_bit);
            }
            
            if(overflow)
            {
                output->__buffer[index + shift_unit] = full_adder(output->__buffer[index + shift_unit], overflow, carry);
                ++index;
            }

            for(; carry; ++index)
            {
                const __CUtype shift_index = index + shift_unit;
                output->__buffer[shift_index] = carry_handle(output->__buffer[shift_index], carry);
            }

            return index + shift_unit - 1;
        }

        void naive_multiplication()
        {
            std::size_t highest_order = 0;

            for(int i = vint_min->value_bit_width() - 1; i >= 0; --i)
            {
                if(!bitget(vint_min->__buffer, i))
                    continue;
                
                if(output->__capacity == 0)
                    output->__change_capacity(bit_capacity(vint_min->value_bit_width() + vint_max->value_bit_width(), __CUtype_bit_length) + 1, false, true);

                highest_order = std::max(highest_order, shift_plus(i));
            }

            output->__bit_length = __set_int_sign(std::bit_width(output->__buffer[highest_order]) + highest_order * __CUtype_bit_length, sign);
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