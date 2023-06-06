import subprocess

# 打开backtrace文件
with open('backtrace', 'r') as f:
    lines = f.readlines()

# 遍历每一行地址
for line in lines:
    # 移除行尾的换行符
    address = line.strip()

    # 调用addr2line命令获取源代码文件和行号
    result = subprocess.run(['addr2line', '-e', '../target/kernel', address], stdout=subprocess.PIPE)

    # 解析addr2line输出，获取源代码文件和行号
    output = result.stdout.decode('utf-8').strip().split('\n')
    source_file = output[0]

    # 将源代码文件和行号信息附加到backtrace文件后面
    with open('backtrace', 'a') as f:
        f.write('\n' + source_file)