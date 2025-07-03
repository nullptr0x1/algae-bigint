#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>
#include <utility>
#include <cstring>
#include <cmath>
#include <iostream>
#include <algorithm>


namespace algae
{
    /*
        高精度整数:
            成员:
                __bit_length: 整数位长度, 其绝对值描述当前整数的占用了多少bit，其符号为整数的符号
                __buffer:以2^64进制的形式存储高精度整数的绝对值，以低位优先的方式存储(最低位从0开始)
            
            构造:
                O.参数:void
                    此时vinteger表示0

                A.参数:10进制整数的字符串
                    1.先把10进制整数的字符串转换为10^19进制的中间态(intermediate_state_integer::intermediate_state_integer)
-                        a.对字符串进行预处理, 解析其格式是否正确并获取符号，如何获取其无符号部分
-                        b.逆序遍历字符串, 并逐个的把字符转换为10^19进制数
-                    2.进行短除法,短除2^64后得到2^64进制表示的数(intermediate_state_integer::convert_to_binary_system)
-                        1.创建转换标准数(cast_standard)，其值为2^64的10^19进制数
-                        2.把转换标准数逐渐乘以2, 直到其大小超过了中间态
-                            如何比较中间态与标准转换数的大小(intermediate_state_integer::short_compare)
-                                a.比较空间占用
-                                b.如果空间占用相同，从高到底位逐位比较
-                        3.如果转换标准数大于中间态, 除以2并重新比较大小
-                          如果转换标准数等于中间态, 把结果vinteger.__buffer的第power_of_2位设置为1, 并结束
-                          如果转换标准数小于中间态, 把结果vinteger.__buffer的第power_of_2位设置为1, 把中间态减去转换标准数, 之后转换标准数大于中间态
-                          直到转换标准数为2^64
-
-                          --其中在等于或小于下:
-                            如果vinteger.__buffer == nullptr,  则其为cast_standard.binary_length
-                            如果vinteger.__bit_length == 0, 则其为cast_standard.bit_width
-
-                        4.处理小于2^64的部分

                B.参数:c++ 算术类型
                    1.对于浮点数,进行floor后转换为有符号整数
                    2.对于有符号整数, 提取符号后把其绝对值以无符号数进行处理
                    3.对于无符号整数, 开空间后, 直接写入__buffer[0]

                
    */
    class vinteger
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
                // 合法性测试函数，用于检查输入的字符串是否为有效的整数
                void legitimacy_testing(std::string_view source)
                {
                    // 若输入字符串为空，抛出无效参数异常
                    if(source.empty())
                        throw std::invalid_argument("source is empty");

                    // 检查字符串的第一个字符是否为负号，若是则标记为负数
                    sign = source[0] == '-';
                    // 若第一个字符既不是正负号也不是数字，抛出无效参数异常
                    if(!(sign || source[0] == '+' || std::isdigit(source[0])))
                        throw std::invalid_argument("source is not a valid integer");
                    
                    // 遍历字符串的剩余部分，检查是否都是数字
                    for(size_t i = 1; i < source.size(); i++)
                        if(!std::isdigit(source[i]))
                            throw std::invalid_argument("source is not a valid integer");
                }

                // 移除字符串前缀零的函数
                static std::string remove_prefix_zeros(std::string source)
                {
                    // 遍历字符串，找到第一个非零字符
                    for(std::size_t i = 0; i < source.size(); ++i)
                        if(source[i] != '0')
                            return source.substr(i);

                    // 若字符串全为零，返回空字符串
                    return "";
                }

                // 默认构造函数，不做任何操作
                intermediate_state_integer() = default;
                // 拷贝构造函数，使用默认实现
                intermediate_state_integer(const intermediate_state_integer&) = default;

                // 分配 std::uint64_t 类型内存的函数
                static std::uint64_t * make_int64_memory(const std::size_t n) 
                {
                    // 分配 n 个 std::uint64_t 类型的内存空间
                    auto memory = new std::uint64_t[n];
                    // 将分配的内存空间初始化为 0
                    std::memset(memory, 0, n * sizeof(std::uint64_t));
                    return memory;
                }
            public:
                // 构造函数，接受一个 10 进制整数的字符串作为参数
                intermediate_state_integer(std::string_view source)
                {
                    // 对输入字符串进行合法性测试
                    legitimacy_testing(source);

                    // 移除字符串的前缀零，并截取去除符号后的部分
                    std::string copy = remove_prefix_zeros(std::string(source.begin() + !std::isdigit(source[0]), source.end()));
                    // 若处理后的字符串为空，直接返回
                    if (copy.empty())
                        return;
                    
                    // 临时变量，用于存储当前部分的数值
                    std::uint64_t temporary = 0;
                    // 移位变量，用于计算当前字符的权重
                    std::uint64_t shift = 1;
                    // 分配内存空间，用于存储中间态整数
                    buffer = make_int64_memory(std::max(2, (int)copy.length() / std::numeric_limits<std::uint64_t>::digits10 + 1));

                    // 逆序遍历字符串
                    for(auto it = copy.rbegin(); it != copy.rend(); ++it)
                    {
                        // 若移位变量达到 10^19，将临时变量存储到 buffer 中，并重置临时变量和移位变量
                        if (shift == cast_standard::overflow_bound)
                        {
                            buffer[length++] = temporary;
                            temporary = 0, shift = 1;
                        }

                        // 计算当前字符的数值，并累加到临时变量中
                        temporary += (*it - '0') * shift;
                        // 移位变量乘以 10
                        shift *= 10;
                    }

                    // 若临时变量不为 0，将其存储到 buffer 中
                    if(temporary != 0)
                        buffer[length++] = temporary;
                }

                // 析构函数，释放分配的内存
                ~intermediate_state_integer()
                {
                    if(buffer != nullptr)
                        delete[] buffer;
                }

            private:
                // 带借位减法函数，用于计算两个 64 位无符号整数的减法，并处理借位
                inline static std::uint64_t sub_with_retreat(std::uint64_t a, std::uint64_t b, bool& retreat) 
                {
                    // 先将借位加到减数上
                    b += retreat;
                    // 判断是否需要借位
                    retreat = a < b;
                    // 根据是否借位返回计算结果
                    return retreat ? cast_standard::overflow_bound - (b - a) :  a - b;
                }

                // 短比较函数，用于比较中间态整数和转换标准数的大小
                std::strong_ordering short_compare(const cast_standard& standard)
                {
                    // 先比较两者的长度
                    if(auto r = standard.length() <=> length; r != std::strong_ordering::equal)
                        return r;

                    // 若长度相同，从高位到低位逐位比较
                    for(int i = length - 1; i >= 0; --i)
                    {
                        if(auto r = standard.buffer[i] <=> buffer[i]; r != std::strong_ordering::equal)
                            return r;
                    }
                    
                    // 若所有位都相同，返回相等
                    return std::strong_ordering::equal;
                }

                // 短减法函数，用于从中间态整数中减去转换标准数
                void short_isub(const cast_standard& standard)
                {
                    // 借位标志
                    bool retreat = false;
                    // 索引变量
                    std::size_t index = 0;

                    // 从低位到高位逐位相减
                    for(; index < standard.length(); ++index)
                        buffer[index] = sub_with_retreat(buffer[index], standard.buffer[index], retreat);
                    
                    // 处理剩余的借位
                    for(; retreat; ++index)
                        buffer[index] = sub_with_retreat(buffer[index], 0, retreat);

                    // 移除高位的零
                    for(int i = length - 1; buffer[i] == 0 && i >= 0; --i, --length)
                        /* pass */;
                }

                // 设置指定位为 1 的函数
                inline void bitset(std::uint8_t * memory, std::size_t index) {
                    memory[index / 8] |= 1 << (index % 8);
                }
            public:
                // 将中间态整数转换为 2^64 进制表示的函数
                void convert_to_binary_system(vinteger& result)
                {
                    // 若中间态整数的 buffer 为空，直接返回
                    if(buffer == nullptr)
                        return;

                    // 创建转换标准数对象
                    cast_standard standard;
                    // 比较中间态整数和转换标准数的大小
                    std::partial_ordering relation = short_compare(standard);

                    // 若中间态整数小于转换标准数，将转换标准数乘以 2 并重新比较
                    while(relation == std::partial_ordering::less)
                    {
                        standard.mul2();
                        relation = short_compare(standard);
                    }
                    
                    // 进行短除法，直到转换标准数为 2^64
                    // 如果中间态整数开始就小于转换标准数, 不进行循环
                    while(standard.power_of_2() > 64 || (standard.power_of_2() == 64 && relation != std::partial_ordering::greater))
                    {
                        // 若中间态整数大于转换标准数，将转换标准数除以 2 并重新比较
                        if(relation == std::partial_ordering::greater)
                        {
                            standard.div2();
                            relation = short_compare(standard);
                            continue;
                        }
                        // buffer 为空，分配内存
                        else if(result.__buffer == nullptr)
                            result.__change_capacity(standard.binary_length(), false, true);
                        
                        // 设置结果的指定位为 1
                        bitset((std::uint8_t*)result.__buffer, standard.power_of_2());
                        
                        // 更新结果的位数
                        if(std::abs(result.__bit_length) < (std::int32_t)standard.power_of_2())
                            result.__bit_length = (std::int32_t)standard.bit_width() * (sign ? -1 : 1);

                        // 若中间态整数等于转换标准数，结束转换
                        if (relation == std::partial_ordering::equivalent)
                            return;

                        // 标记为大于
                        relation = std::partial_ordering::greater;
                        // 从中间态整数中减去转换标准数
                        short_isub(standard);
                    }

                    // 若结果的 buffer 为空，分配内存
                    if(result.__buffer == nullptr)
                        result.__change_capacity(1, false, true);

                    // 处理剩余部分小于2^64的部分
                    if(relation == std::partial_ordering::greater)
                        result.__buffer[0] = buffer[1] * cast_standard::overflow_bound + buffer[0];

                    // 如果若中间态整数开始就小于转换标准数更新结果的位数
                    if(result.__bit_length == 0)
                        result.__bit_length = std::bit_width(result.__buffer[0]) * (sign ? -1 : 1);
                }

                // 获取指定位的值的函数
                inline bool bitget(const std::uint8_t * memory, std::size_t index) {
                    return memory[index / 8] & (1 << (index % 8));
                }
            private:
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
                    buffer = make_int64_memory(std::max(std::size_t((source.value_length() * 64 * 0.3010) / 8 + 1), std::size_t(2)));
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

        using __computing_unit_type = std::uint_fast64_t;
        using __CUtype = __computing_unit_type;
        constexpr static std::size_t __CUtype_bit_lenght = sizeof(__CUtype) * 8;

        __CUtype * __buffer = nullptr;
        std::int32_t __bit_length = 0;
        std::uint32_t __capacity = 0;


        void __change_capacity(std::uint32_t new_capacity, bool keep_value = false, bool initial = false)
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

        void __try_reserve(std::size_t unit_count)
        {
            if(unit_count > __capacity)
                __change_capacity(unit_count, true);
            else if(value_length() == 0)
                clear();
        }

        void __capacity_adaptive() 
        {
            if(__capacity > value_length())
                __change_capacity(value_length());
        }


        
        void __initialization_by_cinteger(std::int64_t x)
        {
            if(x == 0)
                return;

            __change_capacity(1);
            __buffer[0] = (std::uintmax_t)std::abs(x);
            __bit_length = std::bit_width(__buffer[0]) * (x < 0 ? -1 : 1);
        }

        void __initialization_by_cinteger(std::uint64_t x)
        {
            __change_capacity(1);
            __buffer[0] = x;
            __bit_length = std::bit_width(__buffer[0]);
        }
    public:
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


    public:
        vinteger(std::string_view source) {
            vinteger::intermediate_state_integer(source).convert_to_binary_system(*this);
        }


        vinteger(const vinteger& source) { 
            operator =(source);
        }

        vinteger(vinteger&& source) {
            operator =(std::move(source));
        }



        vinteger& operator=(const vinteger& source)
        {
            if(this == &source)
                return *this;

            __change_capacity(source.value_length());
            std::memcpy(__buffer, source.__buffer, source.value_length() * sizeof(__CUtype));
            __bit_length = source.__bit_length;

            return *this;
        }

        vinteger& operator=(vinteger&& source)
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


        ~vinteger() {
            clear();
        }


        

        inline bool empty() const {
            return __bit_length == 0;
        }

        inline int sign() const 
        {
            if(__bit_length == 0)
                return 0;

            return __bit_length > 0 ? 1 : -1;
        }

        inline std::size_t value_bit_width() const {
            return std::abs(__bit_length);
        }

        inline std::size_t value_length() const {
            return std::abs(__bit_length) / __CUtype_bit_lenght + bool(std::abs(__bit_length) % __CUtype_bit_lenght);
        }

        

        inline std::uint64_t* value_data() {
            return __buffer;
        }

        inline const std::uint64_t* value_data() const {
            return __buffer;
        }
        
        void clear()
        {
            if(__buffer)
                delete[] __buffer;

            __buffer = nullptr;
            __bit_length = 0;
            __capacity = 0;
        }





        // ****** cmp operations ******
        friend std::strong_ordering operator <=>(const vinteger&, const vinteger&);

    private:
        inline static int __int_sign(const std::int64_t x) {
            if(x > 0)
                return 1;
            
            return x ? -1 : 0;
        }

        inline static int __int_sign(const std::uint64_t x) {
            return x ? 1 : 0;
        }

        inline static std::int64_t __set_int_sign(const std::uint64_t x, int sign) {
            return sign > 0 ? x : -((std::int64_t)x);
        }

        std::strong_ordering __compare_template(const vinteger& a, const std::int64_t b)
        {
            if(auto r = a.sign() <=> __int_sign(b) ; r != std::strong_ordering::equal || (a.empty() && b == 0))
                return r;

            if(a.value_bit_width() >= 63)
                return std::strong_ordering::greater;

            return __set_int_sign(a.__buffer[0], a.sign()) <=> b;
        }

        std::strong_ordering __compare_template(const vinteger& a, const std::uint64_t b)
        {
            if(auto r = a.sign() <=> __int_sign(b); r != std::strong_ordering::equal || (a.empty() && b == 0)) 
                return r;

            if(a.value_bit_width() >= __CUtype_bit_lenght)
                return std::strong_ordering::greater;

            return a.__buffer[0] * a.sign() <=> b;
        }
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
        vinteger& operator <<=(const std::size_t shift)
        {
            if(shift == 0)
                return *this;

            const std::size_t length = value_length();
            const std::size_t shift_unit = shift / __CUtype_bit_lenght;
            const std::size_t shift_bit = shift % __CUtype_bit_lenght;
            
            std::uint64_t overflow = 0;

            __bit_length += __set_int_sign(shift, sign());
            __try_reserve(value_length());

            for(std::size_t i = 0; i < length; ++i)
            {
                const std::uint64_t temp = __buffer[i];
                __buffer[i] = overflow | (temp << shift_bit);
                overflow = temp >> (__CUtype_bit_lenght - shift_bit);
            }
            
            if(shift_unit)
            {
                std::memmove(__buffer + shift_unit, __buffer, length * sizeof(__CUtype));
                std::memset(__buffer, 0, shift_unit * sizeof(__CUtype));
            }

            if(overflow)
                __buffer[value_length() - 1] = overflow;

            return *this;
        }

        vinteger operator<<(const std::size_t shift) const
        {
            vinteger result(*this);
            result <<= shift;
            return result;
        }
        

        
        vinteger& operator >>=(const std::size_t shift)
        {
            if(shift == 0)
                return *this;

            const std::size_t length = value_length();
            const std::size_t shift_unit = shift / __CUtype_bit_lenght;
            const std::size_t shift_bit = shift % __CUtype_bit_lenght;

            std::uint64_t underflow = 0;
            for(int i = length - 1; i >= 0; --i)
            {
                const std::uint64_t temp = __buffer[i];
                __buffer[i] = underflow | (temp >> shift_bit);
                underflow = temp << (__CUtype_bit_lenght - shift_bit);
            }

            if(shift_unit)
                std::memmove(__buffer, __buffer + shift_unit, length * sizeof(__CUtype));

            __bit_length -= __set_int_sign(shift, sign());
            __try_reserve(value_length());

            return *this;
        }

        vinteger operator>>(const std::size_t shift) const
        {
            vinteger result(*this);
            result >>= shift;
            return result;
        }



        // ****** arithmetic operations ******
    private:
        // adder_context 类，用于处理高精度整数加法和减法运算的上下文信息
        struct adder_context
        {
            vinteger* output = nullptr;

            // 指向绝对值较大的 vinteger 对象的指针
            const vinteger* max_vint = nullptr;
            // 指向绝对值较小的 vinteger 对象的指针
            const vinteger* min_vint = nullptr;

            // 绝对值较大的 vinteger 对象的存储长度
            std::size_t max_lenght = 0;
            // 绝对值较小的 vinteger 对象的存储长度
            std::size_t min_lenght = 0;

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
                max_lenght = max_v.value_length(), min_lenght = min_v.value_length();
            }

            // 处理两个操作数位数相同且为减法运算的情况
            // 参数 a 和 b 为参与运算的两个 vinteger 对象
            // 返回值：1 表示 |a| > |b|；-1 表示 |a| < |b|；0 表示 |a| = |b|
            int init_case_for_same_space_size_with_miuns_mode(const vinteger& a, const vinteger& b)
            {
                // 从高位到低位逐位比较两个操作数
                for(int i = a.value_length() - 1; i >= 0; --i)
                {
                    // 如果 a 的当前位大于 b 的当前位，a 为绝对值较大的操作数
                    if(a.__buffer[i] > b.__buffer[i])
                    {
                        max_vint = &a, min_vint = &b;
                        max_lenght = min_lenght = i + 1;
                        return 1;
                    }
                    // 如果 a 的当前位小于 b 的当前位，b 为绝对值较大的操作数
                    else if(a.__buffer[i] < b.__buffer[i])
                    {
                        max_vint = &b, min_vint = &a;
                        max_lenght = min_lenght = i + 1;
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

            // 全加器函数，用于处理加法运算中的进位
            // 参数 a 和 b 为要相加的两个 64 位无符号整数
            // 参数 carry 为进位标志，引用传递
            // 返回值：相加后的结果
            inline static __CUtype full_adder(__CUtype a, __CUtype b, bool& carry) 
            {
                // 计算相加结果
                __CUtype c = a + b + carry;
                // 判断是否产生进位
                carry = (c < a) || (c < b) || (carry && c == 0);
                return c;
            }

            // 全减器函数，用于处理减法运算中的借位
            // 参数 a 和 b 为要相减的两个 64 位无符号整数
            // 参数 retreat 为借位标志，引用传递
            // 返回值：相减后的结果
            inline static __CUtype full_subtractor(__CUtype a, __CUtype b, bool& retreat)
            {
                // 计算相减结果
                __CUtype diff = a - b - retreat;
                // 判断是否产生借位
                retreat = a < b || (a == b && retreat);
                return diff;
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
                for(std::size_t i = 0; i < min_lenght; ++i) 
                {
                    if constexpr(Mode)
                        output->__buffer[i] = full_adder(max_vint->__buffer[i], min_vint->__buffer[i], carry_or_retreat);
                    else
                        output->__buffer[i] = full_subtractor(max_vint->__buffer[i], min_vint->__buffer[i], carry_or_retreat);
                }

                return carry_or_retreat;
            }

            // 处理进位
            // 参数 x 为要处理的 64 位无符号整数
            // 参数 carry 为进位标志，引用传递
            // 返回值：处理后的结果
            inline static std::uint64_t carry_handle(__CUtype x, bool& carry) 
            {
                // 加上进位
                __CUtype c = x + carry;
                // 判断是否产生进位
                carry = c < x && carry;
                return c;
            }

            // 处理借位
            // 参数 x 为要处理的 64 位无符号整数
            // 参数 retreat 为借位标志，引用传递
            // 返回值：处理后的结果
            inline static std::uint64_t retreat_handle(__CUtype x, bool& retreat)
            {
                // 减去借位
                __CUtype diff = x - retreat;
                // 判断是否产生借位
                retreat = x == 0 && retreat;
                return diff;
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
                std::size_t i = min_lenght;
                
                // 处理溢出部分，直到没有进位或借位
                for(; i < max_lenght && carry_or_retreat; ++i)
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
                    output->__buffer[max_lenght] = carry_or_retreat;
                    // 更新结果的位数和符号
                    output->__bit_length = __set_int_sign(max_vint->value_bit_width() + 1, sign);
                }
                // 没有进位
                else
                {
                    // 复制剩余部分
                    std::memmove(output->__buffer + i, max_vint->__buffer + i, (max_lenght - i) * sizeof(__CUtype));

                    // 减法运算且最高位为 0
                    if(mode < 0 && output->__buffer[max_lenght - 1] == 0)
                    {
                        if(max_lenght > 1)
                            output->__bit_length = __set_int_sign(std::bit_width(output->__buffer[max_lenght - 2]) + (max_lenght - 2) * 64, sign);
                        else
                            output->clear();
                    }
                    // 其他情况
                    else
                        output->__bit_length = __set_int_sign(std::bit_width(output->__buffer[max_lenght - 1]) + (max_lenght - 1) * 64, sign);
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
                    output->__try_reserve(mode > 0 ? max_lenght + 1 : max_lenght);

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


    public:
        vinteger operator-() const
        {
            vinteger result(*this);
            result.__bit_length = -result.__bit_length;
            return result;
        }

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

        

        vinteger& operator+=(const vinteger& b)
        {
            vinteger::adder_context(*this, b, *this, 1);
            return *this;
        }

        vinteger& operator-=(const vinteger& b)
        {
            vinteger::adder_context(*this, b, *this, -1);
            return *this;
        }


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
        static vinteger __karatsuba(const vinteger& x, const vinteger& y) 
        {
            if (x.value_bit_width() < 32 && y.value_bit_width() < 32) {
                // 如果数字足够小，使用普通乘法
                return vinteger(static_cast<__CUtype>(x.__buffer[0]) * static_cast<__CUtype>(y.__buffer[0]));
            }

            // 计算数字的位数
            std::size_t n = std::max(x.value_bit_width(), y.value_bit_width());
            std::size_t m = n / 2;

            // 拆分数字
            vinteger a = x >> m;
            vinteger b = x - (a << m);
            vinteger c = y >> m;
            vinteger d = y - (c << m);

            // 递归计算三个乘积
            vinteger ac = __karatsuba(a, c);
            vinteger bd = __karatsuba(b, d);
            vinteger z2 = __karatsuba(a + b, c + d);

            // 计算中间项
            vinteger ad_plus_bc = z2 - ac - bd;

            // 合并结果
            return (ac << (2 * m)) + (ad_plus_bc << m) + bd;
        }

    public:
        friend vinteger operator*(const vinteger&, const vinteger&);

        template<std::integral T>
        friend vinteger operator*(const vinteger&, const T);

        template<std::integral T>
        friend vinteger operator*(const T, const vinteger&);


        std::string to_string() const
        {
            if(empty())
                return "0";

            return vinteger::intermediate_state_integer(*this).convert_to_string();
        }

        operator std::string() const {
            return to_string();
        }
    };




    std::strong_ordering operator <=>(const vinteger& a, const vinteger& b)
    {
        if(auto r = a.__bit_length <=> b.__bit_length; r != std::strong_ordering::equal || (a.empty() && b.empty()))
            return r;

        for(int i = a.value_length() - 1; i >= 0; --i)
            if(auto r = a.__buffer[i] <=> b.__buffer[i]; r != std::strong_ordering::equal)
                return r;

        return std::strong_ordering::equal;
    }

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





    vinteger operator+(const vinteger& a, const vinteger& b) 
    {
        vinteger c;
        vinteger::adder_context(a, b, c, 1);
        return c;
    }

    vinteger operator-(const vinteger& a, const vinteger& b) 
    {
        vinteger c;
        vinteger::adder_context(a, b, c, -1);
        return c;
    }

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



    vinteger operator*(const vinteger& a, const vinteger& b) {
        return vinteger::__karatsuba(a, b);
    }

    template<std::integral T>
    vinteger operator*(const vinteger& a, const T b) {
        return vinteger::__karatsuba(a, vinteger(b));
    }

    template<std::integral T>
    vinteger operator*(const T a, const vinteger& b) {
        return vinteger::__karatsuba(vinteger(a), b);
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

    std::uint64_t vinteger::cast_standard::overflow_bound = vinteger::cast_standard::init_overflow_bound();
    std::uint64_t vinteger::cast_standard::half_overflow_bound = vinteger::cast_standard::overflow_bound / 2;
}


int main()
{
    algae::vinteger a, b;
    std::cin >> a >> b;
    std::cout << a + b;
}