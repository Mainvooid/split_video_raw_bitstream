
#include "x264_encoder.h"

/**
��Ĭ��ֵ,���������ݽ����Ż�.
film����Ӱ���������ͣ�
animation��������
grain����Ҫ����������grainʱ�ã�
stillimage����̬ͼ�����ʱʹ�ã�
psnr��Ϊ���psnr�����Ż��Ĳ�����
ssim��Ϊ���ssim�����Ż��Ĳ�����
fastdecode�����Կ��ٽ���Ĳ�����
zerolatency�����ӳ٣�������֡,������Ҫ�ǳ��͵��ӳٵ������.
*/
#define ENCODER_TUNE "zerolatency"

/**
��Ĭ��ֵ,�޶�����������ĵȼ�.
���ָ���˵ȼ�������ȡ��ȫ������ѡ�����ʹ�õȼ�ѡ����Եõ����õļ����ԡ�
��ʹ�õȼ�ѡ����޷�ʹ������ѹ��(--qp 0 or --crf 0)��
�����Ĳ���������֧���ض��ȼ��Ļ�������Ҫָ���ȼ�ѡ�
�����������֧�ָߵȼ����Ͳ���Ҫָ���ȼ�ѡ���ˡ�
ѡ��:baseline,main,high,high10,high422,high444
*/
#define ENCODER_PROFILE "baseline"

/**
Ĭ��ֵ: medium
ѡ��Ԥ��������Ҫ�ۺϿ���ѹ��Ч���ͱ����ٶȡ�
���쵽�����У�ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow,placebo��
*/
#define ENCODER_PRESET "slower"

/**
ɫ�ʿռ��������
*/
#define ENCODER_COLORSPACE X264_CSP_I420

/**
�ڴ��ʼ����0
*/
#define CLEAR(x) (memset((&x),0,sizeof(x)))

x264Encoder::x264Encoder()
{
	Init();
}

x264Encoder::x264Encoder(int videoWidth, int videoHeight, int channel, int fps)
{
	Init();
	Create(videoWidth, videoHeight, channel, fps);
}

x264Encoder::~x264Encoder()
{
	Destory();
}

void x264Encoder::Init()
{
	m_width = 0;
	m_height = 0;
	m_channel = 0;
	m_widthstep = 0;
	m_fps = 30;
	m_lumaSize = 0;
	m_chromaSize = 0;
	m_encoder = NULL;
}

void x264Encoder::Create(int videoWidth, int videoHeight, int channel, int fps)
{
	int ret;
	int imgSize;

	if (videoWidth <= 0 || videoHeight <= 0 || channel < 0 || fps <= 0) {
		const std::string what = "Wrong input param to create x264 encoder!\n";
		throw std::invalid_argument(what);
	}
	m_width = videoWidth;
	m_height = videoHeight;
	m_channel = channel;
	m_fps = fps;
	m_widthstep = videoWidth * channel;
	m_lumaSize = m_width * m_height;
	m_chromaSize = m_lumaSize / 4;
	imgSize = m_lumaSize * channel;

	//����ṹ���ڴ�
	m_encoder = (x264_encoder *)malloc(sizeof(x264_encoder));
	if (!m_encoder) {
		const std::string what = "Cannot malloc x264_encoder !\n";
		throw std::runtime_error(what);
	}
	//��ʼ���ڴ�
	CLEAR(*m_encoder);
	m_encoder->iframe = 0;
	m_encoder->iframe_size = 0;

	strcpy_s(m_encoder->preset, ENCODER_PRESET);
	strcpy_s(m_encoder->tune, ENCODER_TUNE);

	//��ʼ��������
	CLEAR(m_encoder->param);
	x264_param_default(&m_encoder->param);

	ret = x264_param_default_preset(&m_encoder->param, m_encoder->preset, m_encoder->tune);
	if (ret < 0) {
		const std::string what = "x264_param_default_preset error!\n";
		throw std::runtime_error(what);
	}

	//cpuFlags �Զ�ѡ����ѳ�ǰ�̻߳�������С.
	m_encoder->param.i_threads = X264_SYNC_LOOKAHEAD_AUTO;
	
	//��Ƶѡ��
	m_encoder->param.i_csp = X264_CSP_I420;  // ������ʽ
	m_encoder->param.i_width = m_width;      // Ҫ�����ͼ��Ŀ��
	m_encoder->param.i_height = m_height;    // Ҫ�����ͼ��ĸ߶�
	m_encoder->param.i_frame_total = 0;      // Ҫ�������֡������֪����0
	m_encoder->param.i_keyint_max = 10 * fps;// �ؼ�֡���
	
	//������
	m_encoder->param.i_bframe = 5;
	m_encoder->param.b_open_gop = 0;
	m_encoder->param.i_bframe_pyramid = 0;
	m_encoder->param.i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

	//log����������Ҫ��ӡ������Ϣʱֱ��ע�͵�
	m_encoder->param.i_log_level = X264_LOG_NONE;

	m_encoder->param.i_fps_num = fps;  // ���ʷ���
	m_encoder->param.i_fps_den = 1;    // ���ʷ�ĸ

	m_encoder->param.b_intra_refresh = 1;
	m_encoder->param.b_annexb = 1;
	m_encoder->param.rc.f_rf_constant = 24;
	m_encoder->param.rc.i_rc_method = X264_RC_CRF;

	/*----------------------------------------------------------------------------*/

	strcpy_s(m_encoder->profile, ENCODER_PROFILE);
	ret = x264_param_apply_profile(&m_encoder->param, m_encoder->profile);
	if (ret < 0) {
		const std::string what = "x264_param_apply_profile error!\n";
		throw std::runtime_error(what);
	}
	//�򿪱�����
	m_encoder->h = x264_encoder_open(&m_encoder->param);
	m_encoder->colorspace = ENCODER_COLORSPACE;

	//��ʼ��pic
	ret = x264_picture_alloc(&m_encoder->pic_in, m_encoder->colorspace, m_width, m_height);
	if (ret < 0) {
		const std::string what="x264_picture_alloc error!";
		throw std::runtime_error(what);
	}

	m_encoder->pic_in.img.i_csp = m_encoder->colorspace;
	m_encoder->pic_in.img.i_plane = 3;
	m_encoder->pic_in.i_type = X264_TYPE_AUTO;

	m_encoder->inal = 0;
	m_encoder->nal = (x264_nal_t *)calloc(2, sizeof(x264_nal_t));
	if (!m_encoder->nal) {
		const std::string what = "malloc x264_nal_t error!\n";
		throw std::runtime_error(what);
	}
	CLEAR(*(m_encoder->nal));
	return;
}

int x264Encoder::EncodeOneFrame(const cv::Mat& frame)
{
	if (frame.empty()) {
		return 0;
	}
	cv::Mat bgr(frame), yuv;

	if (1 == frame.channels()) {
		cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
	}
	cv::cvtColor(bgr, yuv, cv::COLOR_BGR2YUV_I420);

	memcpy(m_encoder->pic_in.img.plane[0], yuv.data, m_lumaSize);
	memcpy(m_encoder->pic_in.img.plane[1], yuv.data + m_lumaSize, m_chromaSize);
	memcpy(m_encoder->pic_in.img.plane[2], yuv.data + m_lumaSize + m_chromaSize, m_chromaSize);
	m_encoder->pic_in.i_pts = m_encoder->iframe++;

	m_encoder->iframe_size = x264_encoder_encode(m_encoder->h, &m_encoder->nal, &m_encoder->inal, &m_encoder->pic_in, &m_encoder->pic_out);

	return m_encoder->iframe_size;
}

uchar* x264Encoder::GetEncodedFrame() const
{
	return m_encoder->nal->p_payload;
}

void x264Encoder::Destory()
{
	if (m_encoder) {
		if (m_encoder->h) {
			x264_encoder_close(m_encoder->h);
			m_encoder->h = NULL;
		}
		free(m_encoder);
		m_encoder = NULL;
	}
}

bool x264Encoder::IsValid() const
{
	return ((m_encoder != NULL) && (m_encoder->h != NULL));
}