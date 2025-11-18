### 使用示例：

#### 1. 编译整个目录（默认当前脚本目录）：
```bash
python compile_shader.py
```

#### 2. 编译指定目录：
```bash
python compile_shader.py path/to/shaders/
```

#### 3. 编译单个着色器文件：
```bash
python compile_shader.py shaders/my_shader.frag
```

#### 4. 指定 glslc 路径和包含目录：
```bash
python compile_shader.py --glslc /path/to/glslc --include-dir ./common shaders/test.vert
```

#### 5. 启用调试符号：
```bash
python compile_shader.py --g shaders/
```
