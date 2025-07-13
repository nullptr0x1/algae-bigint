#include "vinteger.h"
#include <cstring>

namespace algae
{
    void vinteger::__change_capacity(std::uint32_t new_capacity, bool keep_value, bool initial)
    {
        if(new_capacity == 0)
            return clear();

        __CUtype * new_buffer = new __CUtype[new_capacity];
        if(initial)
            std::memset(new_buffer, 0, new_capacity * sizeof(__CUtype));

        if(__buffer)
        {
            if(keep_value)
                std::memcpy(new_buffer, __buffer, __capacity * sizeof(__CUtype));
            delete[] __buffer;
        }

        __buffer = new_buffer;
        __capacity = new_capacity;
    }

    void vinteger::__try_reserve(std::size_t unit_count)
    {
        if(unit_count > __capacity)
            __change_capacity(unit_count, true);
        else if(__value_length() == 0)
            clear();
    }

    void vinteger::__capacity_adaptive() 
    {
        if(__capacity > __value_length())
            __change_capacity(__value_length());
    }


    
    void vinteger::__initialization_by_cinteger(std::int64_t x)
    {
        if(x == 0)
            return;

        __change_capacity(1);
        __buffer[0] = (std::uintmax_t)std::abs(x);
        __bit_length = std::bit_width(__buffer[0]) * (x < 0 ? -1 : 1);
    }

    void vinteger::__initialization_by_cinteger(std::uint64_t x)
    {
        __change_capacity(1);
        __buffer[0] = x;
        __bit_length = std::bit_width(__buffer[0]);
    }
   


    vinteger::vinteger(const vinteger& source) { 
        operator =(source);
    }

    vinteger::vinteger(vinteger&& source) {
        operator =(std::move(source));
    }



    vinteger& vinteger::operator=(const vinteger& source)
    {
        if(this == &source)
            return *this;

        __change_capacity(source.__value_length());
        std::memcpy(__buffer, source.__buffer, source.__value_length() * sizeof(__CUtype));
        __bit_length = source.__bit_length;

        return *this;
    }

    vinteger& vinteger::operator=(vinteger&& source)
    {
        if(this == &source)
            return *this;

        if(__buffer)
            delete[] __buffer;

        __buffer = std::exchange(source.__buffer, nullptr);
        __bit_length = std::exchange(source.__bit_length, 0);
        __capacity = std::exchange(source.__capacity, 0);
        
        return *this;
    }



    vinteger::~vinteger() {
        clear();
    }



    bool vinteger::empty() const {
        return __bit_length == 0;
    }

    int vinteger::sign() const 
    {
        if(__bit_length == 0)
            return 0;

        return __bit_length > 0 ? 1 : -1;
    }

    std::size_t vinteger::value_bit_width() const {
        return std::abs(__bit_length);
    }

    inline std::size_t bit_capacity(const std::size_t bit_count, const std::size_t unit_size) {
        return bit_count / unit_size + bool(bit_count % unit_size);
    }

    std::size_t vinteger::__value_length() const {
        return bit_capacity(std::abs(__bit_length), __CUtype_bit_length);
    }

    std::size_t vinteger::__HCU_value_length() const {
        return bit_capacity(std::abs(__bit_length), __HCUtype_bit_length);
    }



    void vinteger::clear()
    {
        if(__buffer)
            delete[] __buffer;

        __buffer = nullptr;
        __bit_length = 0;
        __capacity = 0;
    }
}