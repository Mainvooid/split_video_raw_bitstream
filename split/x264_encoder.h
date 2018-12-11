#pragma once

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "x264.h"
#include "opencv2/opencv.hpp"

struct x264_encoder {
	x264_param_t       param;
	char               preset[20];
	char               tune[20];
	char               profile[20];
	x264_t*            h;
	x264_picture_t     pic_in;
	x264_picture_t     pic_out;
	long               colorspace;
	x264_nal_t*        nal;
	int                iframe;
	int                iframe_size;
	int                inal;
};

class x264Encoder
{
public:

	x264Encoder();

	x264Encoder(int videoWidth, int videoHeight, int channel, int fps);

	~x264Encoder();

	/**
	 * @brief 创建X264编码器.
	 * @param videoWidth  视频宽度
	 * @param videoHeight 视频高度
	 * @param fps 帧率
	 * @return 成功返回true, 失败返回false.
	 */
	void Create(int videoWidth, int videoHeight, int channel = 3, int fps = 30);

	/** 
	 * @brief 编码一帧并返回帧数据长度.
	 * @param frame 输入的一帧图像.
	 * @return 返回编码后的数据长度, 0表示编码失败.
	 */
	int EncodeOneFrame(const cv::Mat& frame);

	/**  
	 * @brief 获取编码后的帧数据.
	 * @brief: EncodeOneFrame 获取帧数据长度后调用.
	 * @return 返回x264裸码数据.
	 */
	uchar* GetEncodedFrame() const;

	/**   
	 * @brief 销毁X264编码器.
	 */
	void Destory();

	// 编码器是否可用
	bool IsValid() const;

private:

	void Init();

public:
	int m_width;
	int m_height;
	int m_channel;
	int m_fps;

protected:

	int m_widthstep;
	int m_lumaSize;
	int m_chromaSize;
	
	x264_encoder*  m_encoder;//编码器对象
};