
#include "x264_encoder.h"

/**
无默认值,对输入内容进行优化.
film：电影、真人类型；
animation：动画；
grain：需要保留大量的grain时用；
stillimage：静态图像编码时使用；
psnr：为提高psnr做了优化的参数；
ssim：为提高ssim做了优化的参数；
fastdecode：可以快速解码的参数；
zerolatency：零延迟，不缓存帧,用在需要非常低的延迟的情况下.
*/
#define ENCODER_TUNE "zerolatency"

/**
无默认值,限定编码输出流的等级.
如果指定了等级，它将取代全部其它选项，所以使用等级选项可以得到良好的兼容性。
但使用等级选项，就无法使用无损压缩(--qp 0 or --crf 0)。
如果你的播放器仅能支持特定等级的话，就需要指定等级选项。
大多数播放器支持高等级，就不需要指定等级选项了。
选项:baseline,main,high,high10,high422,high444
*/
#define ENCODER_PROFILE "baseline"

/**
默认值: medium
选择预设配置需要综合考虑压缩效果和编码速度。
按快到慢排列：ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow,placebo，
*/
#define ENCODER_PRESET "slower"

/**
色彩空间采样类型
*/
#define ENCODER_COLORSPACE X264_CSP_I420

/**
内存初始化置0
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

	//分配结构体内存
	m_encoder = (x264_encoder *)malloc(sizeof(x264_encoder));
	if (!m_encoder) {
		const std::string what = "Cannot malloc x264_encoder !\n";
		throw std::runtime_error(what);
	}
	//初始化内存
	CLEAR(*m_encoder);
	m_encoder->iframe = 0;
	m_encoder->iframe_size = 0;

	strcpy_s(m_encoder->preset, ENCODER_PRESET);
	strcpy_s(m_encoder->tune, ENCODER_TUNE);

	//初始化编码器
	CLEAR(m_encoder->param);
	x264_param_default(&m_encoder->param);

	ret = x264_param_default_preset(&m_encoder->param, m_encoder->preset, m_encoder->tune);
	if (ret < 0) {
		const std::string what = "x264_param_default_preset error!\n";
		throw std::runtime_error(what);
	}

	//cpuFlags 自动选择最佳超前线程缓冲区大小.
	m_encoder->param.i_threads = X264_SYNC_LOOKAHEAD_AUTO;
	
	//视频选项
	m_encoder->param.i_csp = X264_CSP_I420;  // 采样格式
	m_encoder->param.i_width = m_width;      // 要编码的图像的宽度
	m_encoder->param.i_height = m_height;    // 要编码的图像的高度
	m_encoder->param.i_frame_total = 0;      // 要编码的总帧数，不知道用0
	m_encoder->param.i_keyint_max = 10 * fps;// 关键帧间隔
	
	//流参数
	m_encoder->param.i_bframe = 5;
	m_encoder->param.b_open_gop = 0;
	m_encoder->param.i_bframe_pyramid = 0;
	m_encoder->param.i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

	//log参数，不需要打印编码信息时直接注释掉
	m_encoder->param.i_log_level = X264_LOG_NONE;

	m_encoder->param.i_fps_num = fps;  // 码率分子
	m_encoder->param.i_fps_den = 1;    // 码率分母

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
	//打开编码器
	m_encoder->h = x264_encoder_open(&m_encoder->param);
	m_encoder->colorspace = ENCODER_COLORSPACE;

	//初始化pic
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