#ifndef ALGAE_VINTEGER_H
#define ALGAE_VINTEGER_H

#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <cmath>
#include <iostream>

namespace algae
{
    class vinteger
    {
        // 转换中间态类，用于在高精度整数转换过程中作为中间状态
        friend class intermediate_state_integer;

        using __computing_unit_type = std::uint_fast64_t;
        using __CUtype = __computing_unit_type;
        using __HCUtype = std::uint_fast32_t;
        constexpr static std::size_t __CUtype_bit_length = sizeof(std::uint64_t) * 8;
        constexpr static std::size_t __HCUtype_bit_length = sizeof(std::uint32_t ) * 8;

        __CUtype * __buffer = nullptr;
        std::int32_t __bit_length = 0;
        std::uint32_t __capacity = 0;

        void __change_capacity(std::uint32_t new_capacity, bool keep_value = false, bool initial = false);
        void __try_reserve(std::size_t unit_count);
        void __capacity_adaptive();

        void __initialization_by_cinteger(std::int64_t x);
        void __initialization_by_cinteger(std::uint64_t x);

    public:
        using CUtype = __CUtype;

        vinteger() = default;
        
        template<std::integral T>
        vinteger(T x) 
        { 
            if(std::is_signed_v<T>)
                __initialization_by_cinteger((std::int64_t)x); 
            else
                __initialization_by_cinteger((std::uint64_t)x); 
        }

        template<std::floating_point T>
        vinteger(T x) { 
            __initialization_by_cinteger((std::int64_t)std::floor(x)); 
        }


        vinteger(std::string_view source);
        vinteger(const vinteger& source);
        vinteger(vinteger&& source);


        template<std::unsigned_integral T>
        vinteger& operator=(const T x)
        {
            if(x == 0)
                clear();
            else
                __change_capacity(1), __bit_length = std::bit_width(x);

            return *this;
        }

        template<std::signed_integral T>
        vinteger& operator=(const T x)
        {
            this->operator=((std::make_unsigned<T>)std::abs(x));
            __bit_length = __set_int_sign(__bit_length, x < 0 ? -1 : 1);
            return *this;
        }

        vinteger& operator=(const vinteger& source);
        vinteger& operator=(vinteger&& source);

        ~vinteger();



        int sign() const; 
        bool empty() const;
        std::size_t value_bit_width() const;

    private:
        std::size_t __value_length() const;
        std::size_t __HCU_value_length() const;
    
    public:
        void clear();


        // ****** cmp operations ******
        friend std::strong_ordering operator <=>(const vinteger&, const vinteger&);

    private:
        std::strong_ordering __compare_template(const vinteger& a, const std::int64_t b);
        std::strong_ordering __compare_template(const vinteger& a, const std::uint64_t b);
    
    public:
        template<std::integral T>
        friend std::strong_ordering operator <=>(const vinteger&, const T);

        template<std::integral T>
        friend std::strong_ordering operator <=>(const T, const vinteger&);

        template<std::floating_point T>
        friend std::strong_ordering operator <=>(const vinteger&, const T);

        template<std::floating_point T>
        friend std::strong_ordering operator <=>(const T, const vinteger&);



        // ****** bit move ******
        vinteger& operator <<=(const std::size_t shift);
        vinteger operator<<(const std::size_t shift) const;
        
        vinteger& operator >>=(const std::size_t shift);
        vinteger operator>>(const std::size_t shift) const;


        // ****** arithmetic operations ******
    private:
        // adder_context 类，用于处理高精度整数加法和减法运算的上下文信息
        friend struct adder_context;

    public:
        vinteger operator-() const;

        friend vinteger operator+(const vinteger&, const vinteger&);
        friend vinteger operator-(const vinteger&, const vinteger&);

        template<std::integral T>
        friend vinteger operator+(const vinteger&, const T);

        template<std::integral T>
        friend vinteger operator-(const vinteger&, const T);

        template<std::integral T>
        friend vinteger operator+(const T, const vinteger&);

        template<std::integral T>
        friend vinteger operator-(const T, const vinteger&);

        

        vinteger& operator+=(const vinteger& b);
        vinteger& operator-=(const vinteger& b);

        template<std::integral T>
        vinteger& operator+=(const T x) {
            return operator+=(vinteger(x));
        }

        template<std::integral T>
        vinteger& operator-=(const T x) {
            return operator-=(vinteger(x));
        }

        template<std::floating_point T>
        vinteger& operator+=(const T x) {
            return operator+=(vinteger(x));
        }

        template<std::floating_point T>
        vinteger& operator-=(const T x) {
            return operator-=(vinteger(x));
        }


    private:
        friend struct multiplier_context;

    public:
        friend vinteger operator*(const vinteger&, const vinteger&);

        template<std::integral T>
        friend vinteger operator*(const vinteger&, const T);

        template<std::integral T>
        friend vinteger operator*(const T, const vinteger&);

        vinteger& operator*=(const vinteger& other);

        template<std::integral T>
        vinteger operator*=(const T x) {
            return (*this *= vinteger(x));
        }

    
    
    private:
        friend struct divider_context;

    public:
        friend vinteger operator/(const vinteger&, const vinteger&);
        friend vinteger operator%(const vinteger&, const vinteger&);

        template<std::integral T>
        friend vinteger operator/(const vinteger&, const T);

        template<std::integral T>
        friend vinteger operator/(const T, const vinteger&);

        template<std::integral T>
        friend vinteger operator%(const vinteger&, const T);

        template<std::integral T>
        friend vinteger operator%(const T, const vinteger&);

        vinteger& operator/=(const vinteger& other);
        vinteger& operator%=(const vinteger& other);

        template<std::integral T>
        vinteger operator/=(const T x) {
            return (*this /= vinteger(x));
        }

        template<std::integral T>
        vinteger operator%=(const T x) {
            return (*this %= vinteger(x));
        }

        
        std::string to_string() const;
        operator std::string() const;
    };

    

    std::strong_ordering operator <=>(const vinteger& a, const vinteger& b);

    template<std::integral T>
    std::strong_ordering operator <=>(const vinteger& a, const T b) {
        return vinteger::__compare_template(a, b);
    }

    template<std::integral T>
    std::strong_ordering operator <=>(const T b, const vinteger& a) 
    {
        auto r = vinteger::__compare_template(a, b);
        
        if(r == std::strong_ordering::less)
            return std::strong_ordering::greater;

        if(r == std::strong_ordering::greater)
            return std::strong_ordering::less;

        return std::strong_ordering::equal;
    }

    template<std::floating_point T>
    std::strong_ordering operator <=>(const vinteger& a, const T b) {
        return a <=> (std::int64_t)std::ceil(b);
    }

    template<std::floating_point T>
    std::strong_ordering operator <=>(const T a, const vinteger& b) {
        return (std::int64_t)std::ceil(a) <=> b;
    }




    vinteger operator+(const vinteger& a, const vinteger& b);
    vinteger operator-(const vinteger& a, const vinteger& b);

    template<std::integral T>
    vinteger operator+(const vinteger& a, const T b) {
        return a + vinteger(b);
    }

    template<std::integral T>
    vinteger operator-(const vinteger& a, const T b) {
        return a - vinteger(b);
    }

    template<std::integral T>
    vinteger operator+(const T a, const vinteger& b) {
        return vinteger(a) + b;
    }

    template<std::integral T>
    vinteger operator-(const T a, const vinteger& b) {
        return vinteger(a) - b;
    }



    vinteger operator*(const vinteger& a, const vinteger& b);

    template<std::integral T>
    vinteger operator*(const vinteger& a, const T b) {
        return a * vinteger(b);
    }

    template<std::integral T>
    vinteger operator*(const T a, const vinteger& b) {
        return vinteger(a) * b;
    }


    template<std::integral T>
    vinteger operator/(const vinteger& a, const T b) {
        return a / vinteger(b);
    }

    template<std::integral T>
    vinteger operator/(const T a, const vinteger& b) {
        return vinteger(a) / b;
    }

    template<std::integral T>
    vinteger operator%(const vinteger& a, const T b) {
        return a % vinteger(b);
    }

    template<std::integral T>
    vinteger operator%(const T a, const vinteger& b) {
        return vinteger(a) % b;
    }

    
    std::istream& operator >> (std::istream& in, vinteger& arg);
    std::ostream& operator << (std::ostream& out, const vinteger& arg);
    
    namespace vinteger_literals {
        vinteger operator ""_vi(const char* x);
    }
}

#endif