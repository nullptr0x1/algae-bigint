#include "vinteger.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <vector>

namespace algae
{
    // 转换标准数类，用于在高精度整数转换过程中作为转换标准
    class cast_standard
    {
        private:
            // 初始化溢出边界的静态常量表达式函数
            // 计算 10 的 std::numeric_limits<std::uint64_t>::digits10 次幂
            static constexpr std::uint64_t init_overflow_bound()
            {
                std::uint64_t l = 1;

                for(int i = 0; i < std::numeric_limits<std::uint64_t>::digits10; ++i)
                    l *= 10;

                return l;
            }

        public:
            // 静态成员变量，溢出边界，即 10 的 std::numeric_limits<std::uint64_t>::digits10 次幂
            static std::uint64_t overflow_bound;
            // 静态成员变量，溢出边界的一半
            static std::uint64_t half_overflow_bound;

            // 存储以 10^19 进制表示的转换标准数的值
            std::vector<std::uint64_t> buffer;
            // 当前转换标准数对应的 2 的幂次，初始为 64
            std::size_t count = 64;

            // 构造函数，初始化转换标准数为 2^64 的 10^19 进制表示
            cast_standard() 
                : buffer({std::numeric_limits<std::uint64_t>::max() % overflow_bound + 1,
                        std::numeric_limits<std::uint64_t>::max() / overflow_bound})
            {}

            // 获取转换标准数的长度（以 10^19 进制表示）
            std::size_t length() const {
                return buffer.size();
            }

            // 获取转换标准数以 2^64 进制表示时所需的数组长度
            std::size_t binary_length() const {
                return bit_width() / 64 + bool(bit_width() % 64);
            }

            // 获取转换标准数的二进制位宽
            std::size_t bit_width() const {
                return count + 1;
            }

            // 获取转换标准数对应的 2 的幂次
            std::size_t power_of_2() const {
                return count;
            }

            // 将转换标准数乘以 2 的函数
            void mul2()
            {
                // 进位标志
                bool carry = false;

                // 遍历 buffer 中的每个元素，进行乘以 2 的操作
                for(auto& i : buffer)
                {
                    // 判断当前元素是否大于等于溢出边界的一半
                    bool carry_out = i >= half_overflow_bound;
                    // 计算乘以 2 后的结果，并加上进位
                    i = (carry_out ? (i - half_overflow_bound) : i) * 2 + carry;
                    // 更新进位标志
                    carry = carry_out;
                }

                // 如果最后还有进位，将进位添加到 buffer 的末尾
                if(carry)
                    buffer.push_back(carry);

                // 更新 2 的幂次
                ++count;
            }

            // 将转换标准数除以 2 的函数
            void div2()
            {
                // 借位标志
                bool retreat = false;

                // 逆序遍历 buffer 中的每个元素，进行除以 2 的操作
                std::for_each(buffer.rbegin(), buffer.rend(), [&](std::uint64_t& i) 
                {
                    // 判断当前元素是否为奇数
                    bool retreat_out = i & 1;
                    // 计算除以 2 后的结果，并处理借位
                    i = retreat ? (i / 2) + half_overflow_bound : i / 2;
                    // 更新借位标志
                    retreat = retreat_out;
                });

                // 移除 buffer 末尾的零元素
                while(buffer.back() == 0)
                    buffer.pop_back();

                // 更新 2 的幂次
                --count;
            }
    };

    // 转换中间态类，用于在高精度整数转换过程中作为中间状态
    class intermediate_state_integer 
    {
        public:
            // 存储以 10^19 进制表示的中间态整数的绝对值
            std::uint64_t * buffer = nullptr;
            // 中间态整数的长度（以 10^19 进制表示）
            std::uint32_t length = 0;
            // 中间态整数的符号，true 表示负数，false 表示正数
            bool sign = false;

        private:
            // 分配 std::uint64_t 类型内存的函数
            static std::uint64_t * make_int64_memory(const std::size_t n) 
            {
                // 分配 n 个 std::uint64_t 类型的内存空间
                auto memory = new std::uint64_t[n];
                // 将分配的内存空间初始化为 0
                std::memset(memory, 0, n * sizeof(std::uint64_t));
                return memory;
            }

            // 获取指定位的值的函数
            inline bool bitget(const std::uint8_t * memory, std::size_t index) {
                return memory[index / 8] & (1 << (index % 8));
            }

            // 带进位加法函数，用于计算两个 64 位无符号整数的加法，并处理进位
            inline static std::uint64_t add_with_carry(std::uint64_t a, std::uint64_t b, bool& carry) 
            {
                // 先将进位加到加数上
                a += carry;
                // 判断是否需要进位
                carry = a >= cast_standard::overflow_bound - b;
                // 根据是否进位返回计算结果
                return carry ? a - (cast_standard::overflow_bound - b) : a + b;
            }

        public:
            // 构造函数，接受一个 vinteger 对象作为参数，将其转换为中间态整数
            intermediate_state_integer(const vinteger& source)
            {
                // 分配内存空间，用于存储中间态整数
                buffer = make_int64_memory(std::max(std::size_t((source.__value_length() * 64 * 0.3010) / 8 + 1), std::size_t(2)));
                // 获取源 vinteger 对象的符号
                sign = source.__bit_length < 0;

                // 处理低位部分
                buffer[0] = source.__buffer[0] % cast_standard::overflow_bound;
                buffer[1] = source.__buffer[0] / cast_standard::overflow_bound;
                // 更新长度
                length = buffer[1] != 0 ? 2 : 1;

                // 遍历源 vinteger 对象的每一位
                for(cast_standard standard; standard.power_of_2() < source.value_bit_width(); standard.mul2())
                {
                    // 进位标志
                    bool carry = false;
                    // 若该位为 1
                    if(bitget((const std::uint8_t*)source.__buffer, standard.power_of_2()))
                    {
                        // 逐位相加
                        for(std::size_t i = 0; i < standard.length(); ++i)
                            buffer[i] = add_with_carry(buffer[i], standard.buffer[i], carry);
                        
                        // 处理进位
                        buffer[standard.length()] += carry;
                        // 更新长度
                        length = std::max((std::uint32_t)standard.length() + carry, length);
                    }
                }
            }

            // 将中间态整数转换为 10 进制字符串的函数
            std::string convert_to_string()
            {
                // 存储结果的字符串
                std::string result;
                result.reserve(length * 19 + sign);

                // 前缀零标志
                bool prefix_zeros = true;

                // 若为负数，添加负号
                if(sign)
                    result += '-';

                // 从高位到低位遍历中间态整数
                for(std::int64_t i = length - 1; i >= 0; --i)
                {
                    // 用于逐位提取数字的移位变量
                    for(std::uint64_t c = buffer[i], shift = cast_standard::overflow_bound / 10; shift >= 1; shift /= 10)
                    {
                        // 提取当前位的数字
                        std::uint8_t r = c / shift;
                        // 若为前缀零，跳过
                        if(r == 0 && prefix_zeros)
                            continue;
                        
                        // 标记前缀零结束
                        prefix_zeros = false;
                        // 将数字转换为字符并添加到结果字符串中
                        result += r + '0';
                        // 更新当前值
                        c %= shift;
                    }
                }

            // 返回结果字符串
            return result;
        }
    };

    std::uint64_t cast_standard::overflow_bound = cast_standard::init_overflow_bound();
    std::uint64_t cast_standard::half_overflow_bound = cast_standard::overflow_bound / 2;


    std::string vinteger::to_string() const
    {
        if(empty())
            return "0";

        return intermediate_state_integer(*this).convert_to_string();
    }

    vinteger::operator std::string() const {
        return to_string();
    }


    extern std::int64_t __set_int_sign(const std::uint64_t x, int sign);

    int __legitimacy_testing(std::string_view source)
    {
        // 若输入字符串为空，抛出无效参数异常
        if(source.empty())
            throw std::invalid_argument("source is empty");

        // 检查字符串的第一个字符是否为负号，若是则标记为负数
        int sign = source[0] == '-' ? -1 : 1;
        // 若第一个字符既不是正负号也不是数字，抛出无效参数异常
        if(!(source[0] == '-' || source[0] == '+' || std::isdigit(source[0])))
            throw std::invalid_argument("source is not a valid integer");
        
        // 遍历字符串的剩余部分，检查是否都是数字
        for(size_t i = 1; i < source.size(); i++)
            if(!std::isdigit(source[i]))
                throw std::invalid_argument("source is not a valid integer");

        return sign;
    }

    // 移除字符串前缀零的函数
    std::string_view __remove_prefix_zeros(std::string_view source)
    {
        // 遍历字符串，找到第一个非零字符
        for(std::size_t i = std::isdigit(source.front()) ? 0 : 1; i < source.size(); ++i)
        {
            if(source[i] != '0')
                return source.substr(i);
        }

        // 若字符串全为零，返回空字符串
        return "";
    }

    vinteger::vinteger(std::string_view source)
    {
        int sign = __legitimacy_testing(source);
        std::string_view copy = __remove_prefix_zeros(source);

        if(copy.empty())
            return;

        __HCUtype temporary = 0, shift = 1;
        const static __HCUtype overflow_bound = std::pow(10, std::numeric_limits<__HCUtype>::digits10);
        auto it = copy.begin();

        for(;it != copy.end() && shift < overflow_bound; ++it)
        {
            if(shift != 1)
                temporary *= 10;

            temporary += (*it - '0');
            shift *= 10;
        }

        *this += temporary;
        temporary = 0, shift = 1;

        for(; it != copy.end(); ++it)
        {
            if(shift == overflow_bound)
            {
                *this *= overflow_bound;
                *this += temporary;
                temporary = 0, shift = 1;
            }

            if(shift != 1)
                temporary *= 10;

            temporary += (*it - '0');
            shift *= 10;
        }

        *this *= shift;
        *this += temporary;

        __bit_length = __set_int_sign(__bit_length, sign);
    }


    std::istream& operator >> (std::istream& in, vinteger& arg)
    {
        std::string s;
        in >> s;
        arg = vinteger(s);
        return in;
    }

    std::ostream& operator << (std::ostream& out, const vinteger& arg)
    {
        out << arg.to_string();
        return out;
    }

    namespace vinteger_literals
    {
        vinteger operator ""_vi(const char* x) {
            return vinteger(x);
        }
    }
}