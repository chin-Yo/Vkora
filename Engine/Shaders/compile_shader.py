import argparse
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile GLSL shaders using glslc')
parser.add_argument('--glslc', type=str, help='path to glslc executable')
parser.add_argument('--g', action='store_true', help='compile with debug symbols (-g)')
parser.add_argument('--include-dir', type=str, help='additional include directory (COMMON_DIR)', default=None)
parser.add_argument('input', nargs='?', help='specific shader file or directory to compile (default: current dir)', default=None)
args = parser.parse_args()

def findGlslc():
    print("==> 正在查找 glslc 可执行文件...")
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.glslc is not None and isExe(args.glslc):
        print(f"==> 使用用户指定的 glslc: {args.glslc}")
        return args.glslc

    exe_name = "glslc"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            print(f"==> 在 PATH 中找到 glslc: {full_path}")
            return full_path

    sys.exit("错误：无法在 PATH 中找到 glslc 可执行文件，也未通过 --glslc 参数指定")

file_extensions = (".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".rgen", ".rchit", ".rmiss", ".mesh", ".task")

glslc_path = findGlslc()
script_dir = os.path.dirname(os.path.realpath(__file__)).replace('\\', '/')

# 默认 COMMON_DIR 为脚本所在目录
common_dir = args.include_dir if args.include_dir else script_dir
print(f"==> COMMON_DIR (include): {common_dir}")

# 确定输入路径：文件 or 目录
input_path = args.input
if input_path is None:
    input_path = script_dir
    print("==> 未指定输入路径，使用当前脚本目录")
else:
    input_path = os.path.abspath(input_path)
    print(f"==> 输入路径: {input_path}")

# 收集要处理的文件
shader_files = []

if os.path.isfile(input_path):
    if input_path.endswith(file_extensions):
        shader_files.append(input_path)
        print(f"==> 将编译单个文件: {input_path}")
    else:
        sys.exit(f"错误：指定的文件 {input_path} 不是支持的着色器类型")
elif os.path.isdir(input_path):
    print(f"==> 扫描目录: {input_path}")
    for root, _, files in os.walk(input_path):
        for file in files:
            if file.endswith(file_extensions):
                shader_files.append(os.path.join(root, file))
else:
    sys.exit(f"错误：输入路径 {input_path} 既不是文件也不是目录")

if not shader_files:
    print("==> 未找到任何着色器文件，退出。")
    sys.exit(0)

total_files = len(shader_files)
success_files = 0

for input_file in shader_files:
    input_file = os.path.abspath(input_file).replace('\\', '/')
    output_file = input_file + ".spv"
    
    print(f"\n   发现着色器文件: {os.path.basename(input_file)}")
    print(f"   输入文件: {input_file}")
    print(f"   输出文件: {output_file}")

    cmd = [glslc_path, "-I", common_dir]

    if args.g:
        cmd.append("-g")
        print("   启用调试符号 (-g)")

    # 设置 target-env（glslc 使用 --target-env，但注意：glslc 通常自动推断，显式指定更安全）
    rel_path = os.path.relpath(input_file, script_dir).replace('\\', '/')
    root_dir_name = os.path.basename(os.path.dirname(input_file))

    if input_file.endswith((".rgen", ".rchit", ".rmiss")):
        cmd.extend(["--target-env", "vulkan1.2"])
        print("   光线追踪着色器，设置目标环境为 vulkan1.2")
    elif root_dir_name == "rayquery" and input_file.endswith(".frag"):
        cmd.extend(["--target-env", "vulkan1.2"])
        print("   光线查询着色器，设置目标环境为 vulkan1.2")
    elif input_file.endswith((".mesh", ".task")):
        cmd.extend(["--target-env", "spirv1.4"])
        print("   网格/任务着色器，设置目标环境为 spirv1.4")

    cmd.extend([input_file, "-o", output_file])

    print(f"   执行命令: {' '.join(cmd)}")
    
    try:
        res = subprocess.call(cmd)
    except Exception as e:
        print(f"   ✗ 启动 glslc 失败: {e}")
        sys.exit(1)

    if res == 0:
        success_files += 1
        print(f"   ✓ 编译成功 ({os.path.basename(input_file)})")
    else:
        print(f"   ✗ 编译失败 ({os.path.basename(input_file)}), 错误码: {res}")
        sys.exit(res)

print(f"\n==> 编译完成: 共处理 {total_files} 个文件，成功 {success_files} 个")
if total_files > 0 and success_files == total_files:
    print("==> 所有着色器编译成功!")