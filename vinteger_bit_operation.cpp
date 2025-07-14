#include "vinteger.h"
#include <cstring>

namespace algae
{
    // ****** bit move ******
    extern std::int64_t __set_int_sign(const std::uint64_t x, int sign);

    vinteger& vinteger::operator <<=(const std::size_t shift)
    {
        if(shift == 0 || empty())
            return *this;

        const std::size_t length = __value_length();
        const std::size_t shift_unit = shift / __CUtype_bit_length;
        const std::size_t shift_bit = shift % __CUtype_bit_length;
        
        std::uint64_t overflow = 0;

        __bit_length += __set_int_sign(shift, sign());
        __try_reserve(__value_length());

        if(std::size_t i = 0; shift_bit)
        {
            while(__buffer[i] == 0)
            {   
                if((++i) >= length)
                    break;
            }

            for(;i < length; ++i)
            {
                const std::uint64_t temp = __buffer[i];
                __buffer[i] = overflow | (temp << shift_bit);
                overflow = temp >> (__CUtype_bit_length - shift_bit);
            }
        }
        
        if(shift_unit)
        {
            std::memmove(__buffer + shift_unit, __buffer, length * sizeof(__CUtype));
            std::memset(__buffer, 0, shift_unit * sizeof(__CUtype));
        }

        if(overflow)
            __buffer[__value_length() - 1] = overflow;

        return *this;
    }

    vinteger vinteger::operator<<(const std::size_t shift) const
    {
        vinteger result(*this);
        result <<= shift;
        return result;
    }
    

    
    vinteger& vinteger::operator >>=(const std::size_t shift)
    {
        if(shift == 0 || empty())
            return *this;

        const std::size_t length = __value_length();
        const std::size_t shift_unit = shift / __CUtype_bit_length;
        const std::size_t shift_bit = shift % __CUtype_bit_length;

        if(std::uint64_t underflow = 0; shift_bit)
        {
            int stop = 0;
            while(__buffer[stop] == 0)
            {   
                if((++stop) >= (int)length)
                    break;
            }

            for(int i = length - 1; i >= stop; --i)
            {
                const std::uint64_t temp = __buffer[i];
                __buffer[i] = underflow | (temp >> shift_bit);
                underflow = temp << (__CUtype_bit_length - shift_bit);
            }

            if(stop && underflow)
                __buffer[stop - 1] = underflow;
        }
        
        if(shift_unit)
            std::memmove(__buffer, __buffer + shift_unit, length * sizeof(__CUtype));

        __bit_length -= __set_int_sign(shift, sign());
        __try_reserve(__value_length());

        return *this;
    }

    vinteger vinteger::operator>>(const std::size_t shift) const
    {
        vinteger result(*this);
        result >>= shift;
        return result;
    }
}