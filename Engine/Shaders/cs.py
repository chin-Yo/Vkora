import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all GLSL shaders')
parser.add_argument('--glslang', type=str, help='path to glslangvalidator executable')
parser.add_argument('--g', action='store_true', help='compile with debug symbols')
parser.add_argument('--dir', type=str, help='specific directory to compile', default=None)
args = parser.parse_args()

def findGlslang():
    print("==> 正在查找 glslangvalidator 可执行文件...")
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.glslang != None and isExe(args.glslang):
        print(f"==> 使用用户指定的 glslangvalidator: {args.glslang}")
        return args.glslang

    exe_name = "glslangvalidator"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            print(f"==> 在 PATH 中找到 glslangvalidator: {full_path}")
            return full_path

    sys.exit("错误：无法在 PATH 中找到 glslangvalidator 可执行文件，也未通过 --glslang 参数指定")

file_extensions = tuple([".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".rgen", ".rchit", ".rmiss", ".mesh", ".task"])

glslang_path = findGlslang()
dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')

print(f"==> 当前工作目录: {dir_path}")

# 如果指定了--dir参数，则只编译该目录
if args.dir:
    root_dirs = [os.path.join(dir_path, args.dir)]
    print(f"==> 指定了目录参数，仅编译: {root_dirs[0]}")
else:
    root_dirs = [dir_path]
    print("==> 未指定目录参数，编译当前目录及其子目录")

total_files = 0
success_files = 0

for root_dir in root_dirs:
    print(f"\n==> 开始扫描目录: {root_dir}")
    for root, dirs, files in os.walk(root_dir):
        print(f"--> 正在处理目录: {root}")
        for file in files:
            if file.endswith(file_extensions):
                total_files += 1
                input_file = os.path.join(root, file)
                output_file = input_file + ".spv"
                
                print(f"   发现着色器文件: {file}")
                print(f"   输入文件: {input_file}")
                print(f"   输出文件: {output_file}")

                add_params = ""
                if args.g:
                    add_params = "-g"
                    print("   启用调试符号 (-g)")

                # Ray tracing shaders require a different target environment           
                if file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
                   add_params = add_params + " --target-env vulkan1.2"
                   print("   光线追踪着色器，设置目标环境为 vulkan1.2")
                # Same goes for samples that use ray queries
                elif root.endswith("rayquery") and file.endswith(".frag"):
                    add_params = add_params + " --target-env vulkan1.2"
                    print("   光线查询着色器，设置目标环境为 vulkan1.2")
                # Mesh and task shader also require different settings
                elif file.endswith(".mesh") or file.endswith(".task"):
                    add_params = add_params + " --target-env spirv1.4"
                    print("   网格/任务着色器，设置目标环境为 spirv1.4")

                command = f"{glslang_path} -V {input_file} -o {output_file} {add_params}"
                print(f"   执行命令: {command}")
                
                res = subprocess.call(command, shell=True)
                if res == 0:
                    success_files += 1
                    print(f"   ✓ 编译成功 ({file})")
                else:
                    print(f"   ✗ 编译失败 ({file}), 错误码: {res}")
                    sys.exit(res)

print(f"\n==> 编译完成: 共处理 {total_files} 个文件，成功 {success_files} 个")
if total_files > 0 and success_files == total_files:
    print("==> 所有着色器编译成功!")