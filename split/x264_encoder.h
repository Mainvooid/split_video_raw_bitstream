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
	 * @brief ����X264������.
	 * @param videoWidth  ��Ƶ���
	 * @param videoHeight ��Ƶ�߶�
	 * @param fps ֡��
	 * @return �ɹ�����true, ʧ�ܷ���false.
	 */
	void Create(int videoWidth, int videoHeight, int channel = 3, int fps = 30);

	/** 
	 * @brief ����һ֡������֡���ݳ���.
	 * @param frame �����һ֡ͼ��.
	 * @return ���ر��������ݳ���, 0��ʾ����ʧ��.
	 */
	int EncodeOneFrame(const cv::Mat& frame);

	/**  
	 * @brief ��ȡ������֡����.
	 * @brief: EncodeOneFrame ��ȡ֡���ݳ��Ⱥ����.
	 * @return ����x264��������.
	 */
	uchar* GetEncodedFrame() const;

	/**   
	 * @brief ����X264������.
	 */
	void Destory();

	// �������Ƿ����
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
	
	x264_encoder*  m_encoder;//����������
};