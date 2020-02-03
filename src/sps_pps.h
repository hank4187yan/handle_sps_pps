/***************************************************************************************
 头文件

 ***************************************************************************************/
#ifndef _sps_pps_H_
#define _sps_pps_H_

#if defined (__cplusplus)
    extern "C" {
#endif

#define SPS_PPS_DEBUG 1


enum error_code {
	E_OK = 0,

	E_SPS = 10000,
	E_SPS_OBTAIN,

	E_PPS = 20000,
	E_PPS_OBTAIN,
};



/* libavcodec/h264.h */

/* NAL unit types */
enum H264_NALU_TYPE {
    H264_NAL_SLICE           = 1,
    H264_NAL_DPA             = 2,
    H264_NAL_DPB             = 3,
    H264_NAL_DPC             = 4,
    H264_NAL_IDR_SLICE       = 5,
    H264_NAL_SEI             = 6,
    H264_NAL_SPS             = 7,
    H264_NAL_PPS             = 8,
    H264_NAL_AUD             = 9,
    H264_NAL_END_SEQUENCE    = 10,
    H264_NAL_END_STREAM      = 11,
    H264_NAL_FILLER_DATA     = 12,
    H264_NAL_SPS_EXT         = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
};


typedef struct ParseContext{
    const uint8_t *buffer;
    uint32_t  buffer_size;
	uint32_t  buffer_pos;
	
	uint32_t sps_start_pos;
	uint32_t sps_size;

	uint32_t pps_start_pos;
	uint32_t pps_size;

	uint32_t state;

    int frame_start_found;
} ParseContext;

#define END_NOT_FOUND (-100)
/* END */


/***
 * Sequence parameter set 
 * 可参考H264标准第7节和附录D E
 */


#define Extended_SAR 255
typedef struct vui_parameters{
    int aspect_ratio_info_present_flag; //0 u(1) 
    int aspect_ratio_idc;               //0 u(8) 
    int sar_width;                      //0 u(16) 
    int sar_height;                     //0 u(16) 
    int overscan_info_present_flag;     //0 u(1) 
    int overscan_appropriate_flag;      //0 u(1) 
    int video_signal_type_present_flag; //0 u(1) 
    int video_format;                   //0 u(3) 
    int video_full_range_flag;          //0 u(1) 
    int colour_description_present_flag; //0 u(1) 
    int colour_primaries;                //0 u(8) 
    int transfer_characteristics;        //0 u(8) 
    int matrix_coefficients;             //0 u(8) 
    int chroma_loc_info_present_flag;     //0 u(1) 
    int chroma_sample_loc_type_top_field;  //0 ue(v) 
    int chroma_sample_loc_type_bottom_field; //0 ue(v) 
    int timing_info_present_flag;          //0 u(1) 
    uint32_t num_units_in_tick;           //0 u(32) 
    uint32_t time_scale;                 //0 u(32) 
    int fixed_frame_rate_flag;           //0 u(1) 
    int nal_hrd_parameters_present_flag; //0 u(1)
    int cpb_cnt_minus1;                 //0 ue(v)
    int bit_rate_scale;                 //0 u(4)
    int cpb_size_scale;                 //0 u(4)
    int bit_rate_value_minus1[16];      //0 ue(v)
    int cpb_size_value_minus1[16];      //0 ue(v)
    int cbr_flag[16];                   //0 u(1)
    int initial_cpb_removal_delay_length_minus1; //0 u(5)
    int cpb_removal_delay_length_minus1;         //0 u(5)
    int dpb_output_delay_length_minus1;         //0 u(5)
    int time_offset_length;                      //0 u(5)
    int vcl_hrd_parameters_present_flag;         //0 u(1)
    int low_delay_hrd_flag;                      //0 u(1)
    int pic_struct_present_flag;                 //0 u(1)
    int bitstream_restriction_flag;              //0 u(1)
    int motion_vectors_over_pic_boundaries_flag;  //0 ue(v)
    int max_bytes_per_pic_denom;                  //0 ue(v)
    int max_bits_per_mb_denom;                    //0 ue(v)
    int log2_max_mv_length_horizontal;            //0 ue(v)
    int log2_max_mv_length_vertical;              //0 ue(v)
    int num_reorder_frames;                       //0 ue(v)
    int max_dec_frame_buffering;                  //0 ue(v)
}vui_parameters_t;
 
typedef struct SPS{
    int profile_idc;
    int constraint_set0_flag;
    int constraint_set1_flag;
    int constraint_set2_flag;
    int constraint_set3_flag;
    int reserved_zero_4bits;
    int level_idc;
    int seq_parameter_set_id;						//ue(v)
    int	chroma_format_idc;							//ue(v)
    int	separate_colour_plane_flag;					//u(1)
    int	bit_depth_luma_minus8;						//0 ue(v) 
    int	bit_depth_chroma_minus8;					//0 ue(v) 
    int	qpprime_y_zero_transform_bypass_flag;		//0 u(1) 
    int seq_scaling_matrix_present_flag;			//0 u(1)
    int	seq_scaling_list_present_flag[12];
    int	UseDefaultScalingMatrix4x4Flag[6];
    int	UseDefaultScalingMatrix8x8Flag[6];
    int	ScalingList4x4[6][16];
    int	ScalingList8x8[6][64];
    int log2_max_frame_num_minus4;					//0	ue(v)
    int pic_order_cnt_type;						//0 ue(v)
    int log2_max_pic_order_cnt_lsb_minus4;				//
    int	delta_pic_order_always_zero_flag;           //u(1)
    int	offset_for_non_ref_pic;                     //se(v)
    int	offset_for_top_to_bottom_field;            //se(v)
    int	num_ref_frames_in_pic_order_cnt_cycle;    //ue(v)	
    int	offset_for_ref_frame_array[16];           //se(v)
    int num_ref_frames;                           //ue(v)
    int	gaps_in_frame_num_value_allowed_flag;    //u(1)
    int	pic_width_in_mbs_minus1;                //ue(v)
    int	pic_height_in_map_units_minus1;         //u(1)
    int	frame_mbs_only_flag;  	                //0 u(1) 
    int	mb_adaptive_frame_field_flag;           //0 u(1) 
    int	direct_8x8_inference_flag;              //0 u(1) 
    int	frame_cropping_flag;                    //u(1)
    int	frame_crop_left_offset;                //ue(v)
    int	frame_crop_right_offset;                //ue(v)
    int	frame_crop_top_offset;                  //ue(v)
    int	frame_crop_bottom_offset;	            //ue(v)
    int vui_parameters_present_flag;            //u(1)
    vui_parameters_t vui_parameters;
}SPS;

/***
 * Picture parameter set
 */
typedef struct PPS{
    int pic_parameter_set_id;
    int seq_parameter_set_id;
    int entropy_coding_mode_flag;
    int pic_order_present_flag;
    int num_slice_groups_minus1;
    int slice_group_map_type;
    int run_length_minus1[32];
    int top_left[32];
    int bottom_right[32];
    int slice_group_change_direction_flag;
    int slice_group_change_rate_minus1;
    int pic_size_in_map_units_minus1;
    int slice_group_id[32];
    int num_ref_idx_10_active_minus1;
    int num_ref_idx_11_active_minus1;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;
    int deblocking_filter_control_present_flag;
    int constrained_intra_pred_flag;
    int redundant_pic_cnt_present_flag;
    int transform_8x8_mode_flag;
    int pic_scaling_matrix_present_flag;
    int pic_scaling_list_present_flag[32];
    int second_chroma_qp_index_offset;
    int	UseDefaultScalingMatrix4x4Flag[6];
    int	UseDefaultScalingMatrix8x8Flag[6];
    int ScalingList4x4[6][16];
    int ScalingList8x8[2][64];
}PPS;

typedef struct get_bit_context{
    uint8_t *buf;         /*指向SPS start*/
    int     buf_size;     /*SPS 长度*/
    int     bit_pos;      /*bit已读取位置*/
    int     total_bit;    /*bit总长度*/
    int     cur_bit_pos;  /*当前读取位置*/
}get_bit_context;


/*  @brief Function sps_info_print() 调试信息打印
 *  @param[in]     sps_ptr      sps信息结构体指针  
 *  @retval        
 *  @pre   
 *  @post  
 */
void sps_info_print(SPS* sps_ptr);


/*  @brief Function h264dec_seq_parameter_set()  h264 SPS infomation 解析
 *  @param[in]     buf       get_bit_context结构的buf ptr, 需同步00 00 00 01 X7后传入  
 *  @param[in]     sps_ptr   sps指针，保存SPS信息
 *  @retval        0: success, -1 : failure
 *  @pre   *  @post
 *  @note:
 *      用法:(伪代码)
 *          get_bit_context buffer;    //申请一个get_bit_context结构buffer
 *          SPS sps_buffer;            //申请一个SPS结构sps_buffer
 *          .......
 *          buffer.buf = SPS_start     //es头标识为00 00 00 01 x7以后的数据，不要这前5字节
 *          buffer.buf_size = SPS_len  //SPS待分析数据长度
 *          ......
 *          h264dec_seq_parameter_set(&buffer, &sps_buffer); 
 *
 */
int h264dec_seq_parameter_set(void *buf, SPS *sps_ptr);

/*  @brief Function h264dec_picture_parameter_set()  h264 PPS infomation 解析
 *  @param[in]     buf       buf ptr, 需同步00 00 00 01 X8后传入  
 *  @param[in]     pps_ptr   pps指针，保存pps信息
 *  @retval        0: success, -1 : failure
 *  @pre   *  @post 
 *  @note: 用法参考sps解析
 */
int h264dec_picture_parameter_set(void *buf, PPS *pps_ptr);


int h264_get_width(SPS *sps_ptr);

int h264_get_height(SPS *sps_ptr);

int h264_get_format(SPS *sps_ptr);

int h264_get_framerate(float *framerate,SPS *sps_ptr);

// for mpeg-2
// sequence header
typedef struct _sequence_header_
{
    unsigned int sequence_header_code; // 0x000001b3

    unsigned int frame_rate_code : 4;
    unsigned int aspect_ratio_information : 4;
    unsigned int vertical_size_value : 12;
    unsigned int horizontal_size_value : 12;

    unsigned int marker_bit : 2;
    unsigned int bit_rate_value : 30;
    
}sequence_header;

// sequence extension
typedef struct _sequence_extension_
{
    unsigned int sequence_header_code; // 0x000001b5

    unsigned int marker_bit : 1;
    unsigned int bit_rate_extension: 12;
    unsigned int vertical_size_extension : 2;
    unsigned int horizontal_size_extension : 2;
    unsigned int chroma_format : 2;
    unsigned int progressive_sequence : 1;
    unsigned int profile_and_level_indication : 8;
    unsigned int extension_start_code : 4;
}sequence_extension;


void memcpy_sps_data(uint8_t *dst,uint8_t *src,int len);

//查找SPS 和 PPS（开始）位置 和 大小
//
//几种状态state：
//2 - 找到1个0
//1 - 找到2个0
//0 - 找到大于等于3个0
//4 - 找到2个0和1个1，即001（即找到了起始码）
//5 - 找到至少3个0和1个1，即0001等等（即找到了起始码）
//7 - 初始化状态
//>=8 - 找到2个Slice Header
//
//关于起始码startcode的两种形式：3字节的0x000001和4字节的0x00000001
//3字节的0x000001只有一种场合下使用，就是一个完整的帧被编为多个slice的时候，
//包含这些slice的nalu使用3字节起始码。其余场合都是4字节的。
//
int h264get_sps_and_pps(ParseContext *pc, const uint8_t *buf, int buf_size);


#if defined (__cplusplus)
}
#endif

#endif

/*_*/
