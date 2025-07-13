#include "vinteger.h"
#include <cstring>

namespace algae
{
    extern std::int64_t __set_int_sign(const std::uint64_t x, int sign);

    // 全加器函数，用于处理加法运算中的进位
    // 参数 a 和 b 为要相加的两个 64 位无符号整数
    // 参数 carry 为进位标志，引用传递
    // 返回值：相加后的结果
    inline vinteger::CUtype full_adder(vinteger::CUtype a, vinteger::CUtype b, bool& carry) 
    {
        // 计算相加结果
        vinteger::CUtype c = a + b + carry;
        // 判断是否产生进位
        carry = (c < a) || (c < b) || (carry && c == 0);
        return c;
    }

    // 全减器函数，用于处理减法运算中的借位
    // 参数 a 和 b 为要相减的两个 64 位无符号整数
    // 参数 retreat 为借位标志，引用传递
    // 返回值：相减后的结果
    inline vinteger::CUtype full_subtractor(vinteger::CUtype a, vinteger::CUtype b, bool& retreat)
    {
        // 计算相减结果
        vinteger::CUtype diff = a - b - retreat;
        // 判断是否产生借位
        retreat = a < b || (a == b && retreat);
        return diff;
    }

    // 处理进位
    // 参数 x 为要处理的 64 位无符号整数
    // 参数 carry 为进位标志，引用传递
    // 返回值：处理后的结果
    inline vinteger::CUtype carry_handle(vinteger::CUtype x, bool& carry) 
    {
        // 加上进位
        vinteger::CUtype c = x + carry;
        // 判断是否产生进位
        carry = c < x && carry;
        return c;
    }

    // 处理借位
    // 参数 x 为要处理的 64 位无符号整数
    // 参数 retreat 为借位标志，引用传递
    // 返回值：处理后的结果
    inline vinteger::CUtype retreat_handle(vinteger::CUtype x, bool& retreat)
    {
        // 减去借位
        vinteger::CUtype diff = x - retreat;
        // 判断是否产生借位
        retreat = x == 0 && retreat;
        return diff;
    }


    struct adder_context
    {
        using __CUtype = vinteger::__CUtype;
        constexpr static std::size_t __CUtype_bit_length = vinteger::__CUtype_bit_length;

        vinteger* output = nullptr;

        // 指向绝对值较大的 vinteger 对象的指针
        const vinteger* max_vint = nullptr;
        // 指向绝对值较小的 vinteger 对象的指针
        const vinteger* min_vint = nullptr;

        // 绝对值较大的 vinteger 对象的存储长度
        std::size_t max_length = 0;
        // 绝对值较小的 vinteger 对象的存储长度
        std::size_t min_length = 0;

        // 运算结果的符号，1 表示正，-1 表示负，0 表示结果为 0
        int sign = 0;
        // 运算模式，1 表示加法，-1 表示减法
        int mode = 0;

        // 处理有一个操作数为 0 的情况
        // 参数 a 和 b 为参与运算的两个 vinteger 对象
        // 返回值：0 表示两个操作数都为 0；-1 表示 a 为 0；1 表示 b 为 0
        int init_case_for_has_zero(const vinteger& a, const vinteger& b)
        {
            // 如果两个操作数都为空，结果为 0
            if(a.empty() && b.empty())
                return 0;
            
            // 如果 a 为空，将 b 作为绝对值较大的操作数
            if(a.empty())
            {
                max_vint = &b, min_vint = &a;
                return -1;
            }

            // 否则，将 a 作为绝对值较大的操作数
            max_vint = &a, min_vint = &b;    
            return 1;
        }

        // 处理两个操作数位数不同的情况
        // 参数 max_v 为绝对值较大的 vinteger 对象，min_v 为绝对值较小的 vinteger 对象
        void init_case_for_diff_bit_size(const vinteger& max_v, const vinteger& min_v)
        {
            // 设置绝对值较大和较小的操作数指针
            max_vint = &max_v, min_vint = &min_v;
            // 记录绝对值较大和较小的操作数的存储长度
            max_length = max_v.__value_length(), min_length = min_v.__value_length();
        }

        // 处理两个操作数位数相同且为减法运算的情况
        // 参数 a 和 b 为参与运算的两个 vinteger 对象
        // 返回值：1 表示 |a| > |b|；-1 表示 |a| < |b|；0 表示 |a| = |b|
        int init_case_for_same_space_size_with_miuns_mode(const vinteger& a, const vinteger& b)
        {
            // 从高位到低位逐位比较两个操作数
            for(int i = a.__value_length() - 1; i >= 0; --i)
            {
                // 如果 a 的当前位大于 b 的当前位，a 为绝对值较大的操作数
                if(a.__buffer[i] > b.__buffer[i])
                {
                    max_vint = &a, min_vint = &b;
                    max_length = min_length = i + 1;
                    return 1;
                }
                // 如果 a 的当前位小于 b 的当前位，b 为绝对值较大的操作数
                else if(a.__buffer[i] < b.__buffer[i])
                {
                    max_vint = &b, min_vint = &a;
                    max_length = min_length = i + 1;
                    return -1;
                }
            }

            // 两个操作数相等
            return 0;
        }

        // 最终初始化，默认将 a 作为绝对值较大的操作数
        // 参数 a 和 b 为参与运算的两个 vinteger 对象
        // 返回值：始终返回 1
        int init_final(const vinteger& a, const vinteger& b) 
        {
            // 调用 init_case_for_diff_bit_size 函数，将 a 作为绝对值较大的操作数
            init_case_for_diff_bit_size(a, b);
            return 1;
        }



        /*
        运算     a符号  b符号  无符号数计算(绝对值运算)         结果符号判断条件                                      最终结果(符号+无符号结果)
        ------------------------------------------------------------------------------------------------------------------------------------------------------------------
        a + b    正     正     |a| + |b|                      恒为正(同正相加)                                       + (|a| + |b|)
        a + b    正     负     max(|a|,|b|) - min(|a|,|b|)    若|a| > |b|：正(与a一致)；若|a| < |b|：负(与b一致)       若|a|>|b|：+(|a| - |b|)；若|a|<|b|：-(|b| - |a|)
        a + b    负     正     max(|a|,|b|) - min(|a|,|b|)    若|a| > |b|：负(与a一致)；若|a| < |b|：正(与b一致)       若|a|>|b|：-(|a| - |b|)；若|a|<|b|：+(|b| - |a|)
        a + b    负     负     |a| + |b|                      恒为负(同负相加)                                       - (|a| + |b|)
        ------------------------------------------------------------------------------------------------------------------------------------------------------------------
        a - b    正     正     max(|a|,|b|) - min(|a|,|b|)    若|a| > |b|：正(与a一致)；若|a| < |b|：负(与a异号)       若|a|>|b|：+(|a| - |b|)；若|a|<|b|：-(|b| - |a|)
        a - b    正     负     |a| + |b|                      恒为正(正-负=正+正)                                    + (|a| + |b|)
        a - b    负     正     |a| + |b|                      恒为负(负-正=负+负)                                    - (|a| + |b|)
        a - b    负     负     max(|a|,|b|) - min(|a|,|b|)    若|a| > |b|：负(与a一致)；若|a| < |b|：正(与a异号)       若|a|>|b|：-(|a| - |b|)；若|a|<|b|：+(|b| - |a|)
        */

        // 根据不同的运算情况初始化结果的符号
        // 参数 r_cmp 为比较两个操作数绝对值大小的结果，1 表示 |a| > |b|，-1 表示 |a| < |b|，0 表示 |a| = |b|
        // 参数 a 和 b 为参与运算的两个 vinteger 对象
        // 参数 plan 为运算计划，1 表示加法，-1 表示减法
        void init_sign(int r_cmp, const vinteger& a, const vinteger& b, int plan)
        {
            // 如果两个操作数绝对值相等，结果符号为 0
            if(r_cmp == 0)
                return;

            if(mode == 0)
            {
                if(plan > 0)
                    sign = r_cmp > 0 ? a.sign() : b.sign();
                else
                    sign = r_cmp > 0 ? a.sign() : -b.sign();

                return;
            }

            // 计划加法运算情况
            if(plan == 1)
            {
                if(a.sign() > 0 && b.sign() < 0)
                    sign = r_cmp;
                else if(a.sign() < 0 && b.sign() > 0)
                    sign = -r_cmp;
                else
                    sign = a.sign();

                return;
            }

            // 计划减法运算情况
            if(a.sign() > 0 && b.sign() > 0)
                sign = r_cmp;
            else if(a.sign() < 0 && b.sign() < 0)
                sign = -r_cmp;
            else
                sign = a.sign();
        }

        // 构造函数，初始化 adder_context 对象
        // 参数 a 和 b 为参与运算的两个 vinteger 对象
        // 参数 plan 为运算计划，1 表示加法，-1 表示减法
        adder_context(const vinteger& a, const vinteger& b, vinteger& c, int plan)
            :output(&c), mode(a.sign() * b.sign() * plan)
        {
            // 比较结果
            int r_cmp = 0;

            // 如果有一个操作数为 0
            if(mode == 0)
                r_cmp = init_case_for_has_zero(a, b);

            // a 的位数大于 b 的位数
            else if(a.value_bit_width() > b.value_bit_width())
                init_case_for_diff_bit_size(a, b), r_cmp = 1;

            // a 的位数小于 b 的位数
            else if(a.value_bit_width() < b.value_bit_width())
                init_case_for_diff_bit_size(b, a), r_cmp = -1;

            // 减法运算且位数相同
            else if(mode == -1)
                r_cmp = init_case_for_same_space_size_with_miuns_mode(a, b);

            // 其他情况
            else
                r_cmp = init_final(a, b);

            // 初始化结果符号
            init_sign(r_cmp, a, b, plan);
            run();
        }


        // 处理两个操作数重叠部分的运算
        // Mode 为模板参数，true 表示加法，false 表示减法
        // 参数 c 为存储结果的 vinteger 对象
        // 返回值：是否产生进位或借位
        template<bool Mode>
        bool handle_overlapped_part()
        {
            // 进位或借位标志
            bool carry_or_retreat = false;
        
            // 处理重叠部分
            for(std::size_t i = 0; i < min_length; ++i) 
            {
                if constexpr(Mode)
                    output->__buffer[i] = full_adder(max_vint->__buffer[i], min_vint->__buffer[i], carry_or_retreat);
                else
                    output->__buffer[i] = full_subtractor(max_vint->__buffer[i], min_vint->__buffer[i], carry_or_retreat);
            }

            return carry_or_retreat;
        }

        // 处理溢出部分的运算
        // Mode 为模板参数，true 表示加法，false 表示减法
        // 参数 c 为存储结果的 vinteger 对象
        // 参数 carry_or_retreat 为进位或借位标志
        // 返回值：处理后的结果
        template<bool Mode>
        void handle_overflow_part(bool carry_or_retreat)
        {
            // 从重叠部分的下一位开始处理
            std::size_t i = min_length;
            
            // 处理溢出部分，直到没有进位或借位
            for(; i < max_length && carry_or_retreat; ++i)
            {
                if constexpr(Mode)
                    output->__buffer[i] = carry_handle(max_vint->__buffer[i], carry_or_retreat);
                else
                    output->__buffer[i] = retreat_handle(max_vint->__buffer[i], carry_or_retreat);
            }
            
            // 如果还有进位
            if(carry_or_retreat)
            {
                // 将进位存储到最高位
                output->__buffer[max_length] = carry_or_retreat;
                // 更新结果的位数和符号
                output->__bit_length = __set_int_sign(max_vint->value_bit_width() + 1, sign);
            }
            // 没有进位
            else
            {
                // 复制剩余部分
                std::memmove(output->__buffer + i, max_vint->__buffer + i, (max_length - i) * sizeof(__CUtype));

                // 减法运算且最高位为 0
                if(mode < 0 && output->__buffer[max_length - 1] == 0)
                {
                    if(max_length > 1)
                        output->__bit_length = __set_int_sign(std::bit_width(output->__buffer[max_length - 2]) + (max_length - 2) * __CUtype_bit_length, sign);
                    else
                        output->clear();
                }
                // 其他情况
                else
                    output->__bit_length = __set_int_sign(std::bit_width(output->__buffer[max_length - 1]) + (max_length - 1) * __CUtype_bit_length, sign);
            }
        }

        // 计算最终的运算结果
        // 返回值：运算结果的 vinteger 对象
        void run()
        {
            // 如果结果符号为 0，返回 0
            if(sign == 0)
                output->clear();
            // 如果有一个操作数为 0
            else if(mode == 0)
                *output = (sign == max_vint->sign() ? *max_vint : -(*max_vint));
            else
            {
                // 根据运算模式分配内存
                output->__try_reserve(mode > 0 ? max_length + 1 : max_length);

                // 处理重叠部分
                bool carry_or_retreat = mode > 0 ? handle_overlapped_part<true>() : handle_overlapped_part<false>();

                // 处理溢出部分
                if(mode > 0)
                    handle_overflow_part<true>(carry_or_retreat);
                else
                    handle_overflow_part<false>(carry_or_retreat);
            }
        }
    };

    vinteger vinteger::operator-() const
    {
        vinteger result(*this);
        result.__bit_length = -result.__bit_length;
        return result;
    }

    vinteger operator+(const vinteger& a, const vinteger& b) 
    {
        vinteger c;
        adder_context(a, b, c, 1);
        return c;
    }

    vinteger operator-(const vinteger& a, const vinteger& b) 
    {
        vinteger c;
        adder_context(a, b, c, -1);
        return c;
    }


    vinteger& vinteger::operator+=(const vinteger& b)
    {
        adder_context(*this, b, *this, 1);
        return *this;
    }

    vinteger& vinteger::operator-=(const vinteger& b)
    {
        adder_context(*this, b, *this, -1);
        return *this;
    }
}