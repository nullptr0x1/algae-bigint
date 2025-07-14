# 主文件依赖的项目文件路径
# 此处无需排序, 程序会根据依赖项目文件的包含关系排序
extra_source_files = [
    "vinteger_adder.cpp",
    "vinteger_bit_operation.cpp",
    "vinteger_cast_for_string.cpp",
    "vinteger_compare.cpp",
    "vinteger_divider.cpp",
    "vinteger_multiplier.cpp",
    "vinteger.cpp",
    "vinteger.h"
]

# 主文件路径, 即含有main函数的文件路径
main_source_file = "main.cpp"

# 输出文件
output_file = "omain.cpp"

# 工作路径
work_path = ""

# 是否启用代码压缩，如果不启用，就只进行代码合并
compress = True

def test_combine():
    global extra_source_files
    global main_source_file
    global output_file
    global work_path
    global compress

    work_path = "combiner_test"
    extra_source_files = [
        "test_c1.cpp",
        "test_c2.cpp",
        "test_h.h",
        "test_h2.h",
        "test_h3.h"
    ]

    main_source_file = "test.cpp"
    output_file = "omain.cpp"
    compress = True

import pathlib

dependent_source_files = set[pathlib.Path]()
dependent_standard_library:dict[str, bool] =  {
    # c++26支持的c++表头
    "algorithm": False,
    "iomanip": False,
    "list": False,
    "ostream": False,
    "streambuf": False,
    "bitset": False,
    "ios": False,
    "locale": False,
    "queue": False,
    "string": False,
    "complex": False,
    "iosfwd": False,
    "map": False,
    "set": False,
    "typeinfo": False,
    "deque": False,
    "iostream": False,
    "memory": False,
    "sstream": False,
    "utility": False,
    "exception": False,
    "istream": False,
    "new": False,
    "stack": False,
    "valarray": False,
    "fstream": False,
    "iterator": False,
    "numeric": False,
    "stdexcept": False,
    "vector": False,
    "functional": False,
    "limits": False,
    "array": False,
    "condition_variable": False,
    "mutex": False,
    "scoped_allocator": False,
    "type_traits": False,
    "atomic": False,
    "forward_list": False,
    "random": False,
    "system_error": False,
    "typeindex": False,
    "chrono": False,
    "future": False,
    "ratio": False,
    "thread": False,
    "unordered_map": False,
    "initializer_list": False,
    "regex": False,
    "tuple": False,
    "unordered_set": False,
    "shared_mutex": False,
    "any": False,
    "execution": False,
    "memory_resource": False,
    "string_view": False,
    "variant": False,
    "charconv": False,
    "filesystem": False,
    "optional": False,
    "barrier": False,
    "concepts": False,
    "latch": False,
    "semaphore": False,
    "stop_token": False,
    "bit": False,
    "coroutine": False,
    "numbers": False,
    "source_location": False,
    "syncstream": False,
    "compare": False,
    "format": False,
    "ranges": False,
    "span": False,
    "version": False,
    "expected": False,
    "flat_set": False,
    "mdspan": False,
    "spanstream": False,
    "stdfloat": False,
    "flat_map": False,
    "generator": False,
    "print": False,
    "stacktrace": False,
    "debugging": False,
    "inplace_vector": False,
    "linalg": False,
    "rcu": False,
    "text_encoding": False,
    "hazard_pointer": False,
    "cassert": False,
    "clocale": False,
    "cstdarg": False,
    "cstring": False,
    "cctype": False,
    "cmath": False,
    "cstddef": False,
    "ctime": False,
    "cerrno": False,
    "csetjmp": False,
    "cstdio": False,
    "cwchar": False,
    "cfloat": False,
    "csignal": False,
    "cstdlib": False,
    "cwctype": False,
    "climits": False,
    "cfenv": False,
    "cinttypes": False,
    "cstdint": False,
    "cuchar": False,

    # c++26兼容的c表头
    "assert.h": False,
    "errno.h": False,
    "cfenv.h": False,
    "float.h": False,
    "inttypes.h": False,
    "limits.h": False,
    "locale.h": False,
    "math.h": False,
    "setjmp.h": False,
    "signal.h": False,
    "stdarg.h": False,
    "stddef.h": False,
    "stdint.h": False,
    "stdio.h": False,
    "stdlib.h": False,
    "string.h": False,
    "time.h": False,
    "uchar.h": False,
    "wchar.h": False,
    "wctype.h": False,
    "wctype.h": False,
    "stdatomic.h": False,

    "complex.h": False,
    "tgmath.h": False,

    "iso646.h": False,
    "stdalign.h": False,
    "stdbool.h": False
}

work_path = pathlib.Path()

class processor_context:
    class line_state_context:
        def __init__(self):
            self.is_invalid = False

            self.is_preprocessor = False
            self.is_redundant_include = False
            self.is_common_with_alnum = False

            self.is_multi_line_macros = False
            self.is_multi_line_comments = False
            self.is_multi_raw_string = ""

    def __init__(self, path:pathlib.Path):
        self.current_path = path

        with open(str(path), 'r', encoding="utf-8") as f:
            source = f.read()
            self.sources = source.split('\n')

        self.reliances = list[str]()
        self.result = str()

        self.last_line = self.line_state_context()
        self.current_line = self.line_state_context()
        self.last_line.is_invalid = True


    def try_process_empty_line(self, line:str):
        if len(line) != 0:
            return 
        
        if self.last_line.is_multi_line_macros:
            self.result += "\n\n"
        else:
            self.current_line = self.last_line

        return True
        
        


    def try_record_reliance(self, path:str):
        if path in dependent_standard_library:

            if dependent_standard_library[path]:
                return False
            
            dependent_standard_library[path] = True
            return True

        
        relative_path = pathlib.Path(path)
        absolute_path = (self.current_path.parent / relative_path).resolve()

        if absolute_path in dependent_source_files:
            self.reliances.append(absolute_path)
            return False

        raise RuntimeError(f"此文件未在依赖文件中: {absolute_path}")


    def try_process_include(self, line:str):
        line = line.lstrip()

        if not line.startswith("#include"):
            return True
        
        if (start := line.find('"', len("#include"))) >= 0:
            end = line.find('"', start + 1)
            return self.try_record_reliance(line[start + 1 : end])

        start = line.find('<', len("#include"))
        end = line.find('>', len("#include"))
        return self.try_record_reliance(line[start + 1 : end])

    def try_process_preprocessor(self, line:str):
        if len(line := line.strip()) == 0:
            return False

        if line[0] != '#':
            return False
        
        if not self.try_process_include(line):
            self.current_line.is_redundant_include = True
            return True
        
        elif line.endswith("\\"):
            self.current_line.is_multi_line_macros = True
        else:
            self.current_line.is_preprocessor = True

        LLIP = not(self.last_line.is_preprocessor or self.last_line.is_redundant_include or self.last_line.is_invalid)
        self.result += ('\n' if LLIP else '') + line

        return True
    
    
    def try_process_multi_line_macros(self, line:str):
        if self.last_line.is_multi_line_macros == False:
            return False
        
        line = line.rstrip()

        if not line.endswith('\\'):
            self.current_line.is_preprocessor = True
        else:
            self.current_line.is_multi_line_macros = True

        if len(line[:len(line) - 1].lstrip()) != 0:
            self.result += '\n' + line

        return True


    def process_conventional_line(self, line:str):
        if len(line := line.lstrip()) == 0:
            return

        while (index := line.find("/*")) >= 0:

            if (eindex := line.find("*/", index + 2)) < 0:
                self.current_line.is_multi_line_comments = True
                self.result += line[:index]
                return
            
            line = line[:index] + line[eindex + 2:]

        if len(line) == 0:
            return

        index = 0
        while (index := line.find("R\"", index)) >= 0:
            separate_sequence = ')' + line[index + 2:line.find('(', index + 2)] + '\"'

            if (eindex := line.find(separate_sequence, index + len(separate_sequence))) < 0:
                self.current_line.is_multi_raw_string = separate_sequence
                self.result += line
                return
        
            index = eindex + len(separate_sequence)
            
        if (index := line.find("//")) >= 0:
            line = line[:index]
        
        if len(line) == 0:
            return

        self.result += (' ' if self.last_line.is_common_with_alnum else '') + line
        self.current_line.is_common_with_alnum = line[-1].isalnum()


    def try_process_multi_line_comments(self, line:str):
        if not self.last_line.is_multi_line_comments:
            return False
        
        if (index := line.find("*/")) < 0:
            self.current_line.is_multi_line_comments = True
            return True
        
        self.current_line.is_multi_line_comments = False
        self.process_conventional_line(line[index + 2:])
        
        return True

    def try_process_multi_raw_string(self, line:str):
        if len(self.last_line.is_multi_raw_string) == 0:
            return False
        
        self.result += '\n'
        
        if (index := line.find(self.last_line.is_multi_raw_string)) < 0:
            self.result += line
            self.current_line.is_multi_raw_string = self.last_line.is_multi_raw_string
            return True

        index += len(self.last_line.is_multi_raw_string)
        self.result += line[:index]
        self.process_conventional_line(line[index:])
        return True

    def process_line(self, line:str) -> bool:
        if self.try_process_empty_line(line):
            return
        
        if self.last_line.is_preprocessor:
            self.result += '\n'

        if self.try_process_multi_line_macros(line):
            return
        
        if self.try_process_multi_line_comments(line):
            return
        
        if self.try_process_multi_raw_string(line):
            return
        
        if self.try_process_preprocessor(line):
            return

        self.process_conventional_line(line)


    def only_process_include(self, line:str):
        if len(line2 := line.strip()) == 0:
            ok = True
        elif line2[0] != '#':
            ok = True
        else:
            ok = self.try_process_include(line2)
        
        if ok:
            self.result += ('\n' if len(self.result) != 0 else '') + line


    def process(self):
        if len(self.result) != 0:
            return self.result

        line_count = 1
        for line in self.sources:
            if compress == False:
                self.only_process_include(line)
                continue

            try:
                self.process_line(line)
                self.last_line = self.current_line
                self.current_line = self.line_state_context()
                line_count += 1
            except RuntimeError as error:
                raise RuntimeError(f"文件{self.current_path}的第{line_count}行处理失败:\n\t{error}")
                
        return self.result
    

def input_pretreatment():
    global work_path
    global main_source_file
    global dependent_source_files
    global output_file

    if len(str(work_path := pathlib.Path(work_path))) == 0:
        work_path = pathlib.Path.cwd().absolute()
    elif not work_path.is_absolute():
        work_path = work_path.absolute()
    if not work_path.exists():
        raise RuntimeError(f"工作路径\"{work_path}\"不存在.")

    if len(str(main_source_file := pathlib.Path(main_source_file))) == 0:
        raise RuntimeError("主文件路径为空")
    if not main_source_file.is_absolute():
        main_source_file =(work_path / main_source_file).resolve()
    if not main_source_file.exists():
        raise RuntimeError(f"主文件路径\"{main_source_file}\"不存在, 当前工作路径为\"{work_path}\".")
        
    for extra_file in extra_source_files:

        if len(str(path := pathlib.Path(extra_file))) == 0:
            continue
        if not path.is_absolute():
            path = (work_path / path).resolve()
        if not path.exists():
            raise RuntimeError(f"额外工程文件\"{path}\"不存在, 当前工作路径为\"{work_path}\".")

        dependent_source_files.add(path)
    
    if len(str(output_file := pathlib.Path(output_file))) == 0:
        output_file = (work_path / (main_source_file.stem + '.output.cpp')).absolute()
    elif not output_file.is_absolute():
        output_file = (work_path / output_file).resolve()


def log_input():
    print(f"工作路径: {work_path}")
    print(f"主文件: {main_source_file}")

    print(f"额外依赖文件:")
    for it in dependent_source_files:
        print(f"\t{it}")

    print(f"输出文件: {output_file}")


    
def handling_dependency_order(original_list:list[processor_context]) -> list[processor_context]:
    order_processors = [x for x in original_list if len(x.reliances) == 0]
    original_list = [x for x in original_list if len(x.reliances) != 0]
    depend_counter = 1


    def dependent_on(reliance:pathlib.Path) -> bool:
        for it in order_processors:
            if reliance == it.current_path:
                return True
        return False

    def dependency(processor:processor_context) -> bool:
        if len(processor.reliances) > depend_counter:
            return False

        for reliance in processor.reliances:
            if not dependent_on(reliance):
                return False
            
        return True


    while len(original_list) != 0:
        order_processors.extend([x for x in original_list if dependency(x)])
        original_list = [x for x in original_list if not dependency(x)]
        depend_counter += 1

    return order_processors

def log_relianced(dependent_source_processors:list[processor_context]):
    print(f"输出排列和依赖关系:")

    for processor in dependent_source_processors:
        print(f"\t{processor.current_path}", end = ' ')
        
        if len(processor.reliances) == 0:
            print("\t无依赖")
            continue

        print("\n\t依赖于:")
        for reliance in processor.reliances:
            print(f"\t\t{reliance}")


def output_to_file(dependent_source_processors:list[processor_context]):
    with open(output_file, 'w+', encoding = "utf-8") as f:
        looped = False

        for processor in dependent_source_processors:
            if looped:
                f.write('\n')

            f.write(processor.result)
            looped = True

def main() -> None:
    input_pretreatment()
    log_input()

    main_source_processor = processor_context(main_source_file)
    main_source_processor.process()

    dependent_source_processors = list[processor_context]()
    for source in dependent_source_files:
        dependent_source_processors.append(processor_context(source))
        dependent_source_processors[-1].process()

    dependent_source_processors = handling_dependency_order(dependent_source_processors)
    dependent_source_processors.append(main_source_processor)

    log_relianced(dependent_source_processors)
    output_to_file(dependent_source_processors)

if __name__ == '__main__':
    main()