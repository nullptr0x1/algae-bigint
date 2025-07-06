import re

def process_cpp_code(input_text):
    # 删除注释
    text = re.sub(r'//.*$', '', input_text, flags=re.M)
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    
    lines = text.split('\n')
    processed_lines = []
    prev_was_preprocessor = False
    
    for line in lines:
        stripped_line = line.rstrip()  # 移除行尾空白(包括原换行符)

        if stripped_line == '':
            continue
        
        # 判断是否为预处理指令
        current_is_preprocessor = stripped_line.strip().startswith('#')
        
        # 处理上一行的换行符
        if processed_lines and not prev_was_preprocessor and current_is_preprocessor:
            processed_lines[-1] += '\n'
        
        # 添加当前行(预处理指令保留换行符，其他行暂不添加)
        processed_lines.append(stripped_line + ('\n' if current_is_preprocessor else ''))
        prev_was_preprocessor = current_is_preprocessor
    
    return ''.join(processed_lines)

with open("main.cpp", 'r', encoding='utf-8') as f:
    s = f.read()

with open("omain.cpp", 'w+', encoding='utf-8') as f:
    f.write(process_cpp_code(s))
