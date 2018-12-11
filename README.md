## 项目说明
利用opencv读取视频,重新编码为x264裸码流,并且生成保存每帧数据buffer size的二进制文件.

#### input
ffmpeg支持的各种格式的视频.

#### output
- .h264 裸码二进制文件.
- .idx  保存帧数据长度大小的二进制文件,每帧4字节保存int类型.


用于某些不依靠码流分析,需要直接读取视频的场景.

---

#### 相关环境
- vs2017
- opencv
- x264

#### 目录结构
```
├─bin               可执行命令行程序(手动移动生成的split.exe至此)
├─opencv            opencv目录
│  ├─include
│  │  └─opencv2
│  └─x64
│      └─vc15
│          ├─bin
│          └─lib
├─src             主要源码目录
├─x264              x264目录
│  ├─bin
│  ├─include
│  └─lib
```
.props属性表文件根据以上结构指定相关相对路径.第三方依赖库可在属性表中自行指定.

#### 使用说明
```bash
./bin/split.exe -i test.mp4 -o test.h264 -b test.idx
```

