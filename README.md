# neye

一个文件系统监视工具，当文件发生变化时自动运行命令。

## 安装

### 从源码安装

```bash
git clone https://github.com/ztysth/neye/settings
cd neye
./install.sh
```

### 手动安装

```bash
make all
sudo make install
```

## 使用方法

### 命令行使用

```bash
# 使用默认设置监视当前目录
neye .

# 监视特定目录并运行自定义命令
neye -r "make sim" /path/to/project

# 监视特定文件扩展名并设置阈值
neye -e ".c .h .v" -t 20 -r "make" /path/to/project

# 忽略特定模式
neye -i "build *.log" /path/to/project
```

### 配置文件

创建 `~/.config/neye/config.ini`：

```ini
[watch]
path = ~/projects/npc
command = make sim
extensions = .c .cpp .h .hpp .v .vh .mk .py .sh
line_threshold = 40
ignore_patterns = obj_dir dump.vcd prog.txt .git *.o *.so *.d *.exe build
```

然后运行：

```bash
neye -c ~/.config/neye/config.ini
```

## 命令行选项

- `-c, --config FILE`: 配置文件路径（默认：~/.config/neye/config.ini）
- `-e, --extensions EXT`: 要监视的文件扩展名（例如：.c .cpp .h）
- `-t, --threshold NUM`: 行数变化阈值（默认：40）
- `-i, --ignore PATTERN`: 忽略模式（空格分隔）
- `-r, --run COMMAND`: 触发时运行的命令
- `-h, --help`: 显示帮助信息

## 配置文件格式

```ini
[watch]
path = /path/to/watch
command = make sim
extensions = .c .cpp .h .v .vh
line_threshold = 30
ignore_patterns = build *.o .git
```

### 配置选项

- **path**: 要监视的目录（必需）
- **command**: 检测到变化时执行的命令
- **extensions**: 要监视的文件扩展名列表（空格分隔）
- **line_threshold**: 触发命令所需的最小行数变化
- **ignore_patterns**: 要忽略的模式列表（空格分隔）

## 使用示例

### C/C++项目

```bash
neye -e ".c .cpp .h" -t 10 -r "make" ~/my_project
```

### Verilog项目

```bash
neye -e ".v .vh" -r "make sim" ~/verilog_project
```

### Python项目

```bash
neye -e ".py" -r "python main.py" ~/python_project
```

### NPC项目集成

```bash
# 监视NPC项目，自动运行make sim
neye -e ".c .cpp .h .v .vh" -r "make sim" /path/to/npc
```

## 停止neye

### 正常停止neye

输入以下命令停止neye：
- `neye quit`
- `neye exit`

### 强制停止neye

如果neye没有响应正常停止命令，可以使用：
- `pkill neye` - 强制终止所有neye进程

**注意**：使用`pkill`后可能需要手动清理PID文件：
```bash
rm -f /tmp/neye.pid
```

## 后台运行

### 基本后台运行

```bash
# 在后台启动neye，不占用当前终端
neye . &

# 继续使用shell做其他工作
ls, cd, vim 等

# 需要退出时，在同一终端输入
neye quit
```

### 持久后台运行

```bash
# 关闭终端后继续运行
nohup neye . > /tmp/neye.log 2>&1 &

# 查看日志
tail -f /tmp/neye.log

# 需要停止时
pkill neye
```

**注意**：使用 `&` 后台运行时，必须在同一终端窗口输入 `neye quit`。如果关闭了终端，neye进程会被终止。

## 卸载

```bash
sudo make uninstall
```

## 系统要求

- 支持inotify的Linux系统
- GCC编译器
- make工具

## 许可证

本项目主体代码采用木兰宽松许可证第二版 (Mulan PSL v2)。

### 第三方依赖

本项目包含以下第三方库：

- **inih**: INI文件解析库，采用New BSD许可证
  - 版权所有: (c) 2009, Ben Hoyt
  - 许可证文件: `LICENSE_INIH`
  - 项目地址: https://github.com/benhoyt/inih