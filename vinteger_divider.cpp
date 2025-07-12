#include "vinteger.h"
#include <stdexcept>

namespace algae
{
    extern std::int64_t __set_int_sign(const std::uint64_t x, int sign);

    struct divider_context
    {
        using __CUtype = vinteger::__CUtype;
        constexpr static std::size_t __CUtype_bit_length = vinteger::__CUtype_bit_length;

        const vinteger *divisor = nullptr;
        const vinteger *dividend = nullptr;

        vinteger *merchant = nullptr, *remainder = nullptr;
        int sign = 0;

        bool pretreatment(const vinteger& x, const vinteger& y) 
        {
            if(x.empty())
            {
                if (merchant)
                    merchant->clear();

                if (remainder)
                    remainder->clear();
            }
            else if(y.empty())
                throw std::runtime_error("divisor is zero");
            else if(y.value_bit_width() == 1)
            {
                if (merchant)
                    *merchant = x.sign() == y.sign() ? x : -x;

                if (remainder)
                    remainder->clear();
            }
            else if(x.value_bit_width() > 64 || y.value_bit_width() > 64)
                return false;
            else
            {
                if (merchant)
                {
                    merchant->__change_capacity(1);
                    merchant->__buffer[0] = x.__buffer[0] / y.__buffer[0];
                    merchant->__bit_length = __set_int_sign(std::bit_width(merchant->__buffer[0]), sign);
                }

                if (remainder)
                {
                    remainder->__change_capacity(1);
                    remainder->__buffer[0] = x.__buffer[0] % y.__buffer[0];
                    remainder->__bit_length = __set_int_sign(std::bit_width(remainder->__buffer[0]), sign);
                }
            }

            return true;
        }

        void naive_division()
        {
            if (this->divisor->value_bit_width() < this->dividend->value_bit_width())
            {
                if (remainder)
                    *remainder = *this->divisor;
                return;
            }

            vinteger divisor = *(this->divisor);
            vinteger dividend = *(this->dividend) << (this->divisor->value_bit_width() - this->dividend->value_bit_width());
             
            for(int shift = this->divisor->value_bit_width() - this->dividend->value_bit_width(); shift >= 0; )
            {
                if(divisor < dividend)
                {
                    shift--, dividend >>= 1;
                    continue;
                }
                
                divisor -= dividend;
                
                if (merchant == nullptr)
                    continue;

                if(merchant->__capacity == 0)
                    merchant->__change_capacity(std::max(std::size_t(shift / __CUtype_bit_length + bool(shift % __CUtype_bit_length)), std::size_t(1)), false, true);
                
                merchant->__buffer[shift / __CUtype_bit_length] |= 1ull << shift % __CUtype_bit_length;
            }

            if (merchant)
                if(merchant->__capacity != 0)
            {
                auto bit_length = std::bit_width(merchant->__buffer[merchant->__capacity - 1]) + __CUtype_bit_length * (merchant->__capacity - 1);
                merchant->__bit_length = __set_int_sign(bit_length, sign);
            }

            if (remainder)
                *remainder = std::move(divisor);
        }


        divider_context(const vinteger& x, const vinteger& y, vinteger* z = nullptr, vinteger* w = nullptr)
            :divisor(&x), dividend(&y), merchant(z), remainder(w), sign(x.sign() * y.sign())
        {
            if(pretreatment(x, y))
                return;

            naive_division();
        }
    };


    vinteger operator/(const vinteger& a, const vinteger& b) 
    {
        vinteger merchant;
        divider_context context(a, b, &merchant);
        return merchant;
    }

    vinteger operator%(const vinteger& a, const vinteger& b)
    {
        vinteger remainder;
        divider_context context(a, b, nullptr, &remainder);
        return remainder;
    }

    vinteger& vinteger::operator/=(const vinteger& other)
    {
        *this = *this / other;
        return *this;
    }

    vinteger& vinteger::operator%=(const vinteger& other)
    {
        *this = *this % other;
        return *this;
    }

}