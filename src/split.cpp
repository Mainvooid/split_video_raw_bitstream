// split.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include <iostream>
#include <fstream>
#include "opencv2/opencv.hpp"
#include "x264_encoder.h"
#include "cmdline.h"

/**
解析命令行获取解析字符串数组.
*/
void cmdline_parser(int argc, char* argv[],std::string result[]) {
	// 创建命令行解析器
	cmdline::parser m_parser;
	/*
	添加参数
	长名称,短名称(\0表示没有短名称),参数描写,是否必须,默认值.
	cmdline::range()限制范围,cmdline::oneof<>()限制可选值.
	通过调用不带类型的add方法,定义bool值(通过调用exsit()方法来推断).
	*/
	m_parser.add<std::string>("input_fname", 'i', "input video file name.", true, "test.mp4");
	m_parser.add<std::string>("output_fname", 'o', "output .h264 file name.", true, "test.h264");
	m_parser.add<std::string>("idx_fname", 'b', "output frame buffer size(char 4).", true, "test.idx");

	//执行解析
	m_parser.parse_check(argc, argv);

	// 获取输入的參数值
	result[0] = m_parser.get<std::string>("input_fname");
	result[1] = m_parser.get<std::string>("output_fname");
	result[2] = m_parser.get<std::string>("idx_fname");
}

int main(int argc, char* argv[])
{
	std::string param[3];
	cmdline_parser(argc, argv, param);

	std::string input_fname = param[0];// "test.mp4";//输入video
	std::string output_fname = param[1]; //"test.h264";//输出.h264裸码文件
	std::string idx_fname= param[2];//"test.idx";//.idx 表示每帧数据长度的uint文件

	cv::Mat frame;//视频帧
	x264Encoder x264;//x264编码器
	uint8_t* buf=NULL;//编码后的帧数据
	int bufsize;//帧数据长度
	cv::VideoCapture cap(input_fname);//视频读取器

	int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
	int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	int fps = static_cast<int>(cap.get(cv::CAP_PROP_FPS));
	bool is_color = true;//是否彩色视频
	int channel = is_color ? 3 : 1;
	int total_frame_num = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));//视频帧数

	//创建编码器
	x264.Create(frame_width, frame_height, channel, fps);

	if (!cap.isOpened())
	{
		const std::string what = "Open video error!";
		throw std::runtime_error(what);
	}
	std::ofstream ofs_bufsize;
	std::ofstream ofs_buf;
	ofs_bufsize.open(idx_fname, std::ofstream::binary);
	ofs_buf.open(output_fname, std::ofstream::binary);
	if (!(ofs_bufsize.is_open() && ofs_buf.is_open())) 
	{
		const std::string what = "Open ofstream error!";
		throw std::runtime_error(what);
	}
	for (int i = 0; i < total_frame_num; i++) {
		//读帧
		if (!cap.read(frame)) {
			break;
		}
		//获取帧缓冲区大小
		bufsize = x264.EncodeOneFrame(frame);
		if (bufsize == 0) {
			const std::string what = "EncodeOneFrame failue!";
			throw std::runtime_error(what);
		}
		if (bufsize > 0) {
			//获取编码后的帧
			buf = x264.GetEncodedFrame();
		}
		ofs_bufsize.write(reinterpret_cast<const char*>(&bufsize),sizeof(int));
		ofs_buf.write(reinterpret_cast<const char*>(buf), bufsize);
	}
	ofs_bufsize.close();
	ofs_buf.close();
}
