# im1281b_home_power
使用 IM1281B 模块和 Linux 系统(开发板)来监测家里的电源状态。
<br>
数据默认写入`/tmp/im1281b_data`，数据格式：
```bash
U=232.8
I=0.23
P=42.0
PF=0.801
T=35.5
F=50.0
# 错误时清空，写入 'ERROR'
```

### 参考/使用的项目:
- https://github.com/sigrokproject/libserialport
- https://github.com/tree-water/acac
- https://github.com/rxi/log.c

### 使用:
```bash
# 编译前请先编译或安装 libserialport
# 程序默认在 root 用户下使用
# 配置详见 im1281b_home_power.h
mkdir build
./build.sh
./build/im1281b_home /dev/ttyS0
```
