## 项目说明
**利用opencv读取视频,重新编码为x264裸码流,并且生成保存每帧数据buffer size的二进制文件.**

**The video is read using opencv, re-encoded into an x264 raw-bitstream, and a binary file is created that holds the size of the data buffer per frame.**

#### input
ffmpeg支持的各种格式的视频.

Various formats of video supported by ffmpeg.

#### output
- .h264 裸码二进制文件.
- .idx  保存帧数据长度大小的二进制文件,每帧4字节保存int类型.

用于某些不依靠码流分析,需要直接读取视频的场景.

For some scenes that do not rely on stream analysis and need to read the video directly.

---

#### 相关环境
- vs2017
- opencv
- x264

#### 目录结构
```
├─bin               可执行命令行程序(手动移动生成的split.exe至此)
├─src               主要源码目录
├─opencv            opencv目录(另行下载)
│  ├─include
│  │  └─opencv2
│  └─x64
│      └─vc15
│          ├─bin
│          └─lib
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

