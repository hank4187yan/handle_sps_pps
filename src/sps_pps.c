/***************************************************************************************

 ***************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> /* for uint32_t, etc */
#include "sps_pps.h"

/* report level */
#define RPT_ERR (1) // error, system error
#define RPT_WRN (2) // warning, maybe wrong, maybe OK
#define RPT_INF (3) // important information
#define RPT_DBG (4) // debug information

static int rpt_lvl = RPT_WRN; /* report level: ERR, WRN, INF, DBG */

/* report micro */
#define RPT(lvl, ...) \
    do { \
        if(lvl <= rpt_lvl) { \
            switch(lvl) { \
                case RPT_ERR: \
                    fprintf(stderr, "\"%s\" line %d [err]: ", __FILE__, __LINE__); \
                    break; \
                case RPT_WRN: \
                    fprintf(stderr, "\"%s\" line %d [wrn]: ", __FILE__, __LINE__); \
                    break; \
                case RPT_INF: \
                    fprintf(stderr, "\"%s\" line %d [inf]: ", __FILE__, __LINE__); \
                    break; \
                case RPT_DBG: \
                    fprintf(stderr, "\"%s\" line %d [dbg]: ", __FILE__, __LINE__); \
                    break; \
                default: \
                    fprintf(stderr, "\"%s\" line %d [???]: ", __FILE__, __LINE__); \
                    break; \
                } \
                fprintf(stderr, __VA_ARGS__); \
                fprintf(stderr, "\n"); \
        } \
    } while(0)

/********************************************
*define here
********************************************/
#define SPS_PPS_DEBUG
#undef  SPS_PPS_DEBUG

#define MAX_LEN 32


/********************************************
*functions
********************************************/
/**
 *  @brief Function get_1bit()   读1个bit
 *  @param[in]     h     get_bit_context structrue
 *  @retval        0: success, -1 : failure
 *  @pre  
 *  @post 
 */
static int get_1bit(void *h)
{
    get_bit_context *ptr = (get_bit_context *)h;
    int ret = 0;
    uint8_t *cur_char = NULL;
    uint8_t shift;

    if(NULL == ptr)
    {
        RPT(RPT_ERR, "NULL pointer");
        ret = -1;
        goto exit;
    }

    cur_char = ptr->buf + (ptr->bit_pos >> 3);
    shift = 7 - (ptr->cur_bit_pos);
    ptr->bit_pos++;
    ptr->cur_bit_pos = ptr->bit_pos & 0x7;
    ret = ((*cur_char) >> shift) & 0x01;
    
exit:
    return ret;
}


/**
 *  @brief Function get_bits()  读n个bits，n不能超过32
 *  @param[in]     h     get_bit_context structrue
 *  @param[in]     n     how many bits you want?     
 *  @retval        0: success, -1 : failure
 *  @pre  
 *  @post 
 */
static int get_bits(void *h, int n)
{
    get_bit_context *ptr = (get_bit_context *)h;
    uint8_t temp[5] = {0};
    uint8_t *cur_char = NULL;
    uint8_t nbyte;
    uint8_t shift;
    uint32_t result;
    uint64_t ret = 0;

    if(NULL == ptr)
    {
        RPT(RPT_ERR, "NULL pointer");
        ret = -1;
        goto exit;      
    }
    
    if(n > MAX_LEN)
    {  
        n = MAX_LEN;
    }

    if((ptr->bit_pos + n) > ptr->total_bit)
    {
        n = ptr->total_bit- ptr->bit_pos;
    }
        
    cur_char = ptr->buf+ (ptr->bit_pos>>3);
    nbyte = (ptr->cur_bit_pos + n + 7) >> 3;
    shift = (8 - (ptr->cur_bit_pos + n))& 0x07;

    if(n == MAX_LEN)
    {
        RPT(RPT_DBG, "12(ptr->bit_pos(:%d) + n(:%d)) > ptr->total_bit(:%d)!!! ",\
                ptr->bit_pos, n, ptr->total_bit);
        RPT(RPT_DBG, "0x%x 0x%x 0x%x 0x%x", (*cur_char), *(cur_char+1),*(cur_char+2),*(cur_char+3));
    }

    memcpy(&temp[5-nbyte], cur_char, nbyte);
    ret = (uint32_t)temp[0] << 24;
    ret = ret << 8;
    ret = ((uint32_t)temp[1]<<24)|((uint32_t)temp[2] << 16)\
                        |((uint32_t)temp[3] << 8)|temp[4];

    ret = (ret >> shift) & (((uint64_t)1<<n) - 1);

    result = ret;
    ptr->bit_pos += n;
    ptr->cur_bit_pos = ptr->bit_pos & 0x7;
    
exit:
    return result;
}


/**
 *  @brief Function parse_codenum() 指数哥伦布编码解析，参考h264标准第9节
 *  @param[in]     buf     
 *  @retval        code_num
 *  @pre  
 *  @post 
 */
static int parse_codenum(void *buf)
{
    uint8_t leading_zero_bits = -1;
    uint8_t b;
    uint32_t code_num = 0;
    
    for(b=0; !b; leading_zero_bits++)
    {
        b = get_1bit(buf);
    }
    
    code_num = ((uint32_t)1 << leading_zero_bits) - 1 + get_bits(buf, leading_zero_bits);

    return code_num;
}

/**
 *  @brief Function parse_ue() 指数哥伦布编码解析 ue(),参考h264标准第9节
 *  @param[in]     buf       sps_pps parse buf     
 *  @retval        code_num
 *  @pre  
 *  @post 
 */
static int parse_ue(void *buf)
{
    return parse_codenum(buf);
}


/**
 *  @brief Function parse_se() 指数哥伦布编码解析 se(), 参考h264标准第9节
 *  @param[in]     buf       sps_pps parse buf     
 *  @retval        code_num
 *  @pre  
 *  @post 
 */
static int parse_se(void *buf)
{   
    int ret = 0;
    int code_num;

    code_num = parse_codenum(buf);
    ret = (code_num + 1) >> 1;
    ret = (code_num & 0x01)? ret : -ret;

    return ret;
}


/**
 *  @brief Function get_bit_context_free()  申请的get_bit_context结构内存释放
 *  @param[in]     buf       get_bit_context buf    
 *  @retval        none
 *  @pre  
 *  @post 
 */
static void get_bit_context_free(void *buf)
{
    get_bit_context *ptr = (get_bit_context *)buf;

    if(ptr)
    {
        if(ptr->buf)
        {
            free(ptr->buf);
        }

        free(ptr);
    }
}


/**
 *  @brief Function de_emulation_prevention()  解竞争代码
 *  @param[in]     buf       get_bit_context buf    
 *  @retval        none
 *  @pre  
 *  @post 
 *  @note:  
 *      调试时总是发现vui.time_scale值特别奇怪，总是16777216，后来查询原因如下:
 *      http://www.cnblogs.com/eustoma/archive/2012/02/13/2415764.html
 *      H.264编码时，在每个NAL前添加起始码 0x000001，解码器在码流中检测到起始码，当前NAL结束。
 *      为了防止NAL内部出现0x000001的数据，h.264又提出'防止竞争 emulation prevention"机制，
 *      在编码完一个NAL时，如果检测出有连续两个0x00字节，就在后面插入一个0x03。
 *      当解码器在NAL内部检测到0x000003的数据，就把0x03抛弃，恢复原始数据。
 *      0x000000  >>>>>>  0x00000300
 *      0x000001  >>>>>>  0x00000301
 *      0x000002  >>>>>>  0x00000302
 *      0x000003  >>>>>>  0x00000303
 */
static void *de_emulation_prevention(void *buf)
{
    get_bit_context *ptr = NULL;
    get_bit_context *buf_ptr = (get_bit_context *)buf;
    int i = 0, j = 0;
    uint8_t *tmp_ptr = NULL;
    int tmp_buf_size = 0;
    int val = 0;

    if(NULL == buf_ptr)
    {
        RPT(RPT_ERR, "NULL ptr");
        goto exit;
    }

    ptr = (get_bit_context *)malloc(sizeof(get_bit_context));
    if(NULL == ptr)
    {
        RPT(RPT_ERR, "NULL ptr");
        goto exit;       
    }

    memcpy(ptr, buf_ptr, sizeof(get_bit_context));

    ptr->buf = (uint8_t *)malloc(ptr->buf_size);
    if(NULL == ptr->buf)
    {
        RPT(RPT_ERR, "NULL ptr");
        goto exit;
    }

    memcpy(ptr->buf, buf_ptr->buf, buf_ptr->buf_size);

    tmp_ptr = ptr->buf;
    tmp_buf_size = ptr->buf_size;
    for(i=0; i<(tmp_buf_size-2); i++)
    {
        /*检测0x000003*/
        val = (tmp_ptr[i]^0x00) + (tmp_ptr[i+1]^0x00) + (tmp_ptr[i+2]^0x03);
        if(val == 0)
        {
            /*剔除0x03*/
            for(j=i+2; j<tmp_buf_size-1; j++)
            {
                tmp_ptr[j] = tmp_ptr[j+1];
            }

            /*相应的bufsize要减小*/
            ptr->buf_size--;
        }
    }

    /*重新计算total_bit*/
    ptr->total_bit = ptr->buf_size << 3;
    
    return (void *)ptr;
    
exit:
    get_bit_context_free(ptr);    
    return NULL;
}


/**
 *  @brief Function get_bit_context_free()  VUI_parameters 解析，原理参考h264标准
 *  @param[in]     buf       get_bit_context buf   
 *  @param[in]     vui_ptr   vui解析结果
 *  @retval        0: success, -1 : failure
 *  @pre  
 *  @post 
 */
static int vui_parameters_set(void *buf, vui_parameters_t *vui_ptr)
{
    int ret = 0;
    int SchedSelIdx = 0;

    if(NULL == vui_ptr || NULL == buf)
    {
        RPT(RPT_ERR,"ERR null pointer\n");
        ret = -1;
        goto exit;
    }

    vui_ptr->aspect_ratio_info_present_flag = get_1bit(buf);
    if(vui_ptr->aspect_ratio_info_present_flag)
    {
        vui_ptr->aspect_ratio_idc = get_bits(buf, 8);
        if(vui_ptr->aspect_ratio_idc == Extended_SAR)
        {
            vui_ptr->sar_width = get_bits(buf, 16);
            vui_ptr->sar_height = get_bits(buf, 16);
        }
    }

    vui_ptr->overscan_info_present_flag = get_1bit(buf);
    if(vui_ptr->overscan_info_present_flag)
    {
        vui_ptr->overscan_appropriate_flag = get_1bit(buf);
    }

    vui_ptr->video_signal_type_present_flag = get_1bit(buf);
    if(vui_ptr->video_signal_type_present_flag)
    {
        vui_ptr->video_format = get_bits(buf, 3);
        vui_ptr->video_full_range_flag = get_1bit(buf);
        
        vui_ptr->colour_description_present_flag = get_1bit(buf);
        if(vui_ptr->colour_description_present_flag)
        {
            vui_ptr->colour_primaries = get_bits(buf, 8);
            vui_ptr->transfer_characteristics = get_bits(buf, 8);
            vui_ptr->matrix_coefficients = get_bits(buf, 8);
        }
    }

    vui_ptr->chroma_loc_info_present_flag = get_1bit(buf);
    if(vui_ptr->chroma_loc_info_present_flag)
    {
        vui_ptr->chroma_sample_loc_type_top_field = parse_ue(buf);
        vui_ptr->chroma_sample_loc_type_bottom_field = parse_ue(buf);
    }

    vui_ptr->timing_info_present_flag = get_1bit(buf);
    if(vui_ptr->timing_info_present_flag)
    {
        vui_ptr->num_units_in_tick = get_bits(buf, 32);
        vui_ptr->time_scale = get_bits(buf, 32);
        vui_ptr->fixed_frame_rate_flag = get_1bit(buf);
    } 

    vui_ptr->nal_hrd_parameters_present_flag = get_1bit(buf);
    if(vui_ptr->nal_hrd_parameters_present_flag)
    {
        vui_ptr->cpb_cnt_minus1 = parse_ue(buf);
        vui_ptr->bit_rate_scale = get_bits(buf, 4);
        vui_ptr->cpb_size_scale = get_bits(buf, 4);

        for(SchedSelIdx=0; SchedSelIdx<=vui_ptr->cpb_cnt_minus1; SchedSelIdx++)
        {
            vui_ptr->bit_rate_value_minus1[SchedSelIdx] = parse_ue(buf);
            vui_ptr->cpb_size_value_minus1[SchedSelIdx] = parse_ue(buf);
            vui_ptr->cbr_flag[SchedSelIdx] = get_1bit(buf);
        }

        vui_ptr->initial_cpb_removal_delay_length_minus1 = get_bits(buf, 5);
        vui_ptr->cpb_removal_delay_length_minus1 = get_bits(buf, 5);
        vui_ptr->dpb_output_delay_length_minus1 = get_bits(buf, 5);
        vui_ptr->time_offset_length = get_bits(buf, 5);
    }
    

    vui_ptr->vcl_hrd_parameters_present_flag = get_1bit(buf);
    if(vui_ptr->vcl_hrd_parameters_present_flag)
    {
        vui_ptr->cpb_cnt_minus1 = parse_ue(buf);
        vui_ptr->bit_rate_scale = get_bits(buf, 4);
        vui_ptr->cpb_size_scale = get_bits(buf, 4);
        
        for(SchedSelIdx=0; SchedSelIdx<=vui_ptr->cpb_cnt_minus1; SchedSelIdx++)
        {
            vui_ptr->bit_rate_value_minus1[SchedSelIdx] = parse_ue(buf);
            vui_ptr->cpb_size_value_minus1[SchedSelIdx] = parse_ue(buf);
            vui_ptr->cbr_flag[SchedSelIdx] = get_1bit(buf);
        }

        vui_ptr->initial_cpb_removal_delay_length_minus1 = get_bits(buf, 5);
        vui_ptr->cpb_removal_delay_length_minus1 = get_bits(buf, 5);
        vui_ptr->dpb_output_delay_length_minus1 = get_bits(buf, 5);
        vui_ptr->time_offset_length = get_bits(buf, 5);        
    }

    if(vui_ptr->nal_hrd_parameters_present_flag \
           || vui_ptr->vcl_hrd_parameters_present_flag)
    {
        vui_ptr->low_delay_hrd_flag = get_1bit(buf);
    }

    vui_ptr->pic_struct_present_flag = get_1bit(buf);
    
    vui_ptr->bitstream_restriction_flag = get_1bit(buf);
    if(vui_ptr->bitstream_restriction_flag)
    {
        vui_ptr->motion_vectors_over_pic_boundaries_flag = get_1bit(buf);
        vui_ptr->max_bytes_per_pic_denom = parse_ue(buf);
        vui_ptr->max_bits_per_mb_denom = parse_ue(buf);
        vui_ptr->log2_max_mv_length_horizontal= parse_ue(buf);
        vui_ptr->log2_max_mv_length_vertical = parse_ue(buf);
        vui_ptr->num_reorder_frames = parse_ue(buf);
        vui_ptr->max_dec_frame_buffering = parse_ue(buf);
    }

exit:
    return ret;
}


/**
 *  @brief Function sps_info_print() 调试信息打印
 *  @param[in]     sps_ptr      sps信息结构体指针  
 *  @retval        
 *  @pre  
 *  @post 
 */
/*SPS 信息打印，调试使用*/
void sps_info_print(SPS* sps_ptr)
{
    if(NULL != sps_ptr)
    {
        RPT(RPT_DBG, "profile_idc: %d", sps_ptr->profile_idc);    
        RPT(RPT_DBG, "constraint_set0_flag: %d", sps_ptr->constraint_set0_flag); 
        RPT(RPT_DBG, "constraint_set1_flag: %d", sps_ptr->constraint_set1_flag); 
        RPT(RPT_DBG, "constraint_set2_flag: %d", sps_ptr->constraint_set2_flag); 
        RPT(RPT_DBG, "constraint_set3_flag: %d", sps_ptr->constraint_set3_flag); 
        RPT(RPT_DBG, "reserved_zero_4bits: %d", sps_ptr->reserved_zero_4bits);
        RPT(RPT_DBG, "level_idc: %d", sps_ptr->level_idc); 
        RPT(RPT_DBG, "seq_parameter_set_id: %d", sps_ptr->seq_parameter_set_id); 
        RPT(RPT_DBG, "chroma_format_idc: %d", sps_ptr->chroma_format_idc);
        RPT(RPT_DBG, "separate_colour_plane_flag: %d", sps_ptr->separate_colour_plane_flag); 
        RPT(RPT_DBG, "bit_depth_luma_minus8: %d", sps_ptr->bit_depth_luma_minus8);    
        RPT(RPT_DBG, "bit_depth_chroma_minus8: %d", sps_ptr->bit_depth_chroma_minus8); 
        RPT(RPT_DBG, "qpprime_y_zero_transform_bypass_flag: %d", sps_ptr->qpprime_y_zero_transform_bypass_flag); 
        RPT(RPT_DBG, "seq_scaling_matrix_present_flag: %d", sps_ptr->seq_scaling_matrix_present_flag); 
        //RPT(RPT_INF, "seq_scaling_list_present_flag:%d", sps_ptr->seq_scaling_list_present_flag); 
        RPT(RPT_DBG, "log2_max_frame_num_minus4: %d", sps_ptr->log2_max_frame_num_minus4);
        RPT(RPT_DBG, "pic_order_cnt_type: %d", sps_ptr->pic_order_cnt_type);
        RPT(RPT_DBG, "num_ref_frames: %d", sps_ptr->num_ref_frames);
        RPT(RPT_DBG, "gaps_in_frame_num_value_allowed_flag: %d", sps_ptr->gaps_in_frame_num_value_allowed_flag);
        RPT(RPT_DBG, "pic_width_in_mbs_minus1: %d", sps_ptr->pic_width_in_mbs_minus1);
        RPT(RPT_DBG, "pic_height_in_map_units_minus1: %d", sps_ptr->pic_height_in_map_units_minus1); 
        RPT(RPT_DBG, "frame_mbs_only_flag: %d", sps_ptr->frame_mbs_only_flag);
        RPT(RPT_DBG, "mb_adaptive_frame_field_flag: %d", sps_ptr->mb_adaptive_frame_field_flag);
        RPT(RPT_DBG, "direct_8x8_inference_flag: %d", sps_ptr->direct_8x8_inference_flag);
        RPT(RPT_DBG, "frame_cropping_flag: %d", sps_ptr->frame_cropping_flag);
        RPT(RPT_DBG, "frame_crop_left_offset: %d", sps_ptr->frame_crop_left_offset);
        RPT(RPT_DBG, "frame_crop_right_offset: %d", sps_ptr->frame_crop_right_offset);
        RPT(RPT_DBG, "frame_crop_top_offset: %d", sps_ptr->frame_crop_top_offset);
        RPT(RPT_DBG, "frame_crop_bottom_offset: %d", sps_ptr->frame_crop_bottom_offset);    
        RPT(RPT_DBG, "vui_parameters_present_flag: %d", sps_ptr->vui_parameters_present_flag);

/*
        if(sps_ptr->vui_parameters_present_flag)
        {
            RPT(RPT_DBG, "aspect_ratio_info_present_flag: %d", sps_ptr->vui_parameters.aspect_ratio_info_present_flag);
            RPT(RPT_DBG, "aspect_ratio_idc: %d", sps_ptr->vui_parameters.aspect_ratio_idc);
            RPT(RPT_DBG, "sar_width: %d", sps_ptr->vui_parameters.sar_width);
            RPT(RPT_DBG, "sar_height: %d", sps_ptr->vui_parameters.sar_height);
            RPT(RPT_DBG, "overscan_info_present_flag: %d", sps_ptr->vui_parameters.overscan_info_present_flag);
            RPT(RPT_DBG, "overscan_info_appropriate_flag: %d", sps_ptr->vui_parameters.overscan_appropriate_flag);
            RPT(RPT_DBG, "video_signal_type_present_flag: %d", sps_ptr->vui_parameters.video_signal_type_present_flag);
            RPT(RPT_DBG, "video_format: %d", sps_ptr->vui_parameters.video_format);
            RPT(RPT_DBG, "video_full_range_flag: %d", sps_ptr->vui_parameters.video_full_range_flag);
            RPT(RPT_DBG, "colour_description_present_flag: %d", sps_ptr->vui_parameters.colour_description_present_flag);
            RPT(RPT_DBG, "colour_primaries: %d", sps_ptr->vui_parameters.colour_primaries);
            RPT(RPT_DBG, "transfer_characteristics: %d", sps_ptr->vui_parameters.transfer_characteristics);
            RPT(RPT_DBG, "matrix_coefficients: %d", sps_ptr->vui_parameters.matrix_coefficients);
            RPT(RPT_DBG, "chroma_loc_info_present_flag: %d", sps_ptr->vui_parameters.chroma_loc_info_present_flag);
            RPT(RPT_DBG, "chroma_sample_loc_type_top_field: %d", sps_ptr->vui_parameters.chroma_sample_loc_type_top_field);
            RPT(RPT_DBG, "chroma_sample_loc_type_bottom_field: %d", sps_ptr->vui_parameters.chroma_sample_loc_type_bottom_field);
            RPT(RPT_DBG, "timing_info_present_flag: %d", sps_ptr->vui_parameters.timing_info_present_flag);
            RPT(RPT_DBG, "num_units_in_tick: %d", sps_ptr->vui_parameters.num_units_in_tick);
            RPT(RPT_DBG, "time_scale: %d", sps_ptr->vui_parameters.time_scale);
            RPT(RPT_DBG, "fixed_frame_rate_flag: %d", sps_ptr->vui_parameters.fixed_frame_rate_flag);
            RPT(RPT_DBG, "nal_hrd_parameters_present_flag: %d", sps_ptr->vui_parameters.nal_hrd_parameters_present_flag);
            RPT(RPT_DBG, "cpb_cnt_minus1: %d", sps_ptr->vui_parameters.cpb_cnt_minus1);
            RPT(RPT_DBG, "bit_rate_scale: %d", sps_ptr->vui_parameters.bit_rate_scale);
            RPT(RPT_DBG, "cpb_size_scale: %d", sps_ptr->vui_parameters.cpb_size_scale);
            RPT(RPT_DBG, "initial_cpb_removal_delay_length_minus1: %d", sps_ptr->vui_parameters.initial_cpb_removal_delay_length_minus1);
            RPT(RPT_DBG, "cpb_removal_delay_length_minus1: %d", sps_ptr->vui_parameters.cpb_removal_delay_length_minus1);
            RPT(RPT_DBG, "dpb_output_delay_length_minus1: %d", sps_ptr->vui_parameters.dpb_output_delay_length_minus1);
            RPT(RPT_DBG, "time_offset_length: %d", sps_ptr->vui_parameters.time_offset_length);
            RPT(RPT_DBG, "vcl_hrd_parameters_present_flag: %d", sps_ptr->vui_parameters.vcl_hrd_parameters_present_flag);
            RPT(RPT_DBG, "low_delay_hrd_flag: %d", sps_ptr->vui_parameters.low_delay_hrd_flag);
            RPT(RPT_DBG, "pic_struct_present_flag: %d", sps_ptr->vui_parameters.pic_struct_present_flag);
            RPT(RPT_DBG, "bitstream_restriction_flag: %d", sps_ptr->vui_parameters.bitstream_restriction_flag);
            RPT(RPT_DBG, "motion_vectors_over_pic_boundaries_flag: %d", sps_ptr->vui_parameters.motion_vectors_over_pic_boundaries_flag);
            RPT(RPT_DBG, "max_bytes_per_pic_denom: %d", sps_ptr->vui_parameters.max_bytes_per_pic_denom);
            RPT(RPT_DBG, "max_bits_per_mb_denom: %d", sps_ptr->vui_parameters.max_bits_per_mb_denom);
            RPT(RPT_DBG, "log2_max_mv_length_horizontal: %d", sps_ptr->vui_parameters.log2_max_mv_length_horizontal);
            RPT(RPT_DBG, "log2_max_mv_length_vertical: %d", sps_ptr->vui_parameters.log2_max_mv_length_vertical);
            RPT(RPT_DBG, "num_reorder_frames: %d", sps_ptr->vui_parameters.num_reorder_frames);
            RPT(RPT_DBG, "max_dec_frame_buffering: %d", sps_ptr->vui_parameters.max_dec_frame_buffering);
        }
*/ 
    }
}


/**
 *  @brief Function h264dec_seq_parameter_set()  h264 SPS infomation 解析
 *  @param[in]     buf       buf ptr, 需同步00 00 00 01 X7后传入  
 *  @param[in]     sps_ptr   sps指针，保存SPS信息
 *  @retval        0: success, -1 : failure
 *  @pre  
 *  @post 
 */
int h264dec_seq_parameter_set(void *buf_ptr, SPS *sps_ptr)
{
    SPS *sps = sps_ptr;
    int ret = 0;
    int profile_idc = 0;
    int i,j,last_scale, next_scale, delta_scale;
    void *buf = NULL;
    
    if(NULL == buf_ptr || NULL == sps)
    {
        RPT(RPT_ERR,"ERR null pointer\n");
        ret = -1;
        goto exit;
    }
    
    memset((void *)sps, 0, sizeof(SPS));

    buf = de_emulation_prevention(buf_ptr);
    if(NULL == buf)
    {
        RPT(RPT_ERR,"ERR null pointer\n");
        ret = -1;
        goto exit;  
    }
    
    sps->profile_idc          = get_bits(buf, 8);
    sps->constraint_set0_flag = get_1bit(buf);  
    sps->constraint_set1_flag = get_1bit(buf);  
    sps->constraint_set2_flag = get_1bit(buf);  
    sps->constraint_set3_flag = get_1bit(buf); 
    sps->reserved_zero_4bits  = get_bits(buf, 4);
    sps->level_idc            = get_bits(buf, 8);
    sps->seq_parameter_set_id = parse_ue(buf);
 
    profile_idc = sps->profile_idc;
    if( (profile_idc == 100) || (profile_idc == 110) || (profile_idc == 122) || (profile_idc == 244)
    || (profile_idc  == 44) || (profile_idc == 83) || (profile_idc  == 86) || (profile_idc == 118) ||\
    (profile_idc == 128))
    {
        sps->chroma_format_idc = parse_ue(buf);
        if(sps->chroma_format_idc == 3)
        {
            sps->separate_colour_plane_flag = get_1bit(buf);
        }

        sps->bit_depth_luma_minus8 = parse_ue(buf);
        sps->bit_depth_chroma_minus8 = parse_ue(buf);
        sps->qpprime_y_zero_transform_bypass_flag = get_1bit(buf);
        
        sps->seq_scaling_matrix_present_flag = get_1bit(buf);
        if(sps->seq_scaling_matrix_present_flag)
        {
            for(i=0; i<((sps->chroma_format_idc != 3)?8:12); i++)
            {
                sps->seq_scaling_list_present_flag[i] = get_1bit(buf);
                if(sps->seq_scaling_list_present_flag[i])
                {
                    if(i<6)
                    {
                        for(j=0; j<16; j++)
                        {
                            last_scale = 8;
                            next_scale = 8;
                            if(next_scale != 0)
                            {
                                delta_scale = parse_se(buf);
                                next_scale = (last_scale + delta_scale + 256)%256;
                                sps->UseDefaultScalingMatrix4x4Flag[i] = ((j == 0) && (next_scale == 0));
                            }
                            sps->ScalingList4x4[i][j] = (next_scale == 0)?last_scale:next_scale;
                            last_scale = sps->ScalingList4x4[i][j];
                        }
                    }
                    else
                    {
                        int ii = i-6;
                        next_scale = 8;
                        last_scale = 8;
                        for(j=0; j<64; j++)
                        {
                            if(next_scale != 0)
                            {
                                delta_scale = parse_se(buf);
                                next_scale = (last_scale + delta_scale + 256)%256;
                                sps->UseDefaultScalingMatrix8x8Flag[ii] = ((j == 0) && (next_scale == 0));
                            }
                            sps->ScalingList8x8[ii][j] = (next_scale == 0)?last_scale:next_scale;
                            last_scale = sps->ScalingList8x8[ii][j];
                        }
                    }
                }
            }
        }
    }

    sps->log2_max_frame_num_minus4 = parse_ue(buf);
    sps->pic_order_cnt_type = parse_ue(buf);
    if(sps->pic_order_cnt_type == 0)
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = parse_ue(buf);
    }
    else if(sps->pic_order_cnt_type == 1)
    {
        sps->delta_pic_order_always_zero_flag = get_1bit(buf);
        sps->offset_for_non_ref_pic = parse_se(buf);
        sps->offset_for_top_to_bottom_field = parse_se(buf);

        sps->num_ref_frames_in_pic_order_cnt_cycle = parse_ue(buf);
        for(i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            sps->offset_for_ref_frame_array[i] = parse_se(buf);
        }
    }

    sps->num_ref_frames = parse_ue(buf);
    sps->gaps_in_frame_num_value_allowed_flag = get_1bit(buf);
    sps->pic_width_in_mbs_minus1 = parse_ue(buf);
    sps->pic_height_in_map_units_minus1 = parse_ue(buf);
    
    sps->frame_mbs_only_flag = get_1bit(buf);
    if(!sps->frame_mbs_only_flag)
    {
        sps->mb_adaptive_frame_field_flag = get_1bit(buf);
    }

    sps->direct_8x8_inference_flag = get_1bit(buf);
    
    sps->frame_cropping_flag = get_1bit(buf);
    if(sps->frame_cropping_flag)
    {
        sps->frame_crop_left_offset = parse_ue(buf);
        sps->frame_crop_right_offset = parse_ue(buf);
        sps->frame_crop_top_offset = parse_ue(buf);
        sps->frame_crop_bottom_offset = parse_ue(buf);
    }

    sps->vui_parameters_present_flag = get_1bit(buf);
    if(sps->vui_parameters_present_flag)
    {
		// 解析 VUI 有异常 
        //vui_parameters_set(buf, &sps->vui_parameters);
    }

#if SPS_PPS_DEBUG
    sps_info_print(sps);
#endif

exit:
    get_bit_context_free(buf);
    return ret;
}


/**
 *  @brief Function more_rbsp_data()  计算pps串最后一个为1的比特位及其后都是比特0的个数
 *  @param[in]     buf       get_bit_context structure
 *  @retval        
 *  @pre  
 *  @post 
 *  @note  这段代码来自网友的帮助，并没有验证，使用时需注意
 */
static int more_rbsp_data(void *buf)  
{  
    get_bit_context *ptr = (get_bit_context *)buf;  
    get_bit_context tmp;

    if(NULL == buf)
    {
        RPT(RPT_ERR, "NULL pointer, err");
        return -1;
    }

    memset(&tmp, 0, sizeof(get_bit_context));
    memcpy(&tmp, ptr, sizeof(get_bit_context));  
  
    for(tmp.bit_pos = ptr->total_bit - 1;tmp.bit_pos > ptr->bit_pos;tmp.bit_pos -= 2)
    {
        if(get_1bit(&tmp)) 
        {
            break;
        }
    }
          
    return tmp.bit_pos == ptr->bit_pos? 0:1;  
}  


/**
 *  @brief Function h264dec_picture_parameter_set()  h264 PPS infomation 解析
 *  @param[in]     buf       buf ptr, 需同步00 00 00 01 X8后传入  
 *  @param[in]     pps_ptr   pps指针，保存pps信息
 *  @retval        0: success, -1 : failure
 *  @pre  
 *  @post 
 */
int h264dec_picture_parameter_set(void *buf_ptr, PPS *pps_ptr)
{
    PPS *pps = pps_ptr;
    int ret = 0;
    void *buf = NULL;
    int iGroup = 0;
    int i,j,last_scale, next_scale, delta_scale;

    if(NULL == buf_ptr || NULL == pps_ptr)
    {
        RPT(RPT_ERR, "NULL pointer\n");
        ret = -1;
        goto exit;
    }

    memset((void *)pps, 0, sizeof(PPS));

    buf = de_emulation_prevention(buf_ptr);
    if(NULL == buf)
    {
        RPT(RPT_ERR,"ERR null pointer\n");
        ret = -1;
        goto exit;  
    }

    pps->pic_parameter_set_id = parse_ue(buf);
    pps->seq_parameter_set_id = parse_ue(buf);
    pps->entropy_coding_mode_flag = get_1bit(buf);
    pps->pic_order_present_flag = get_1bit(buf);
    
    pps->num_slice_groups_minus1 = parse_ue(buf);
    if(pps->num_slice_groups_minus1 > 0)
    {
        pps->slice_group_map_type = parse_ue(buf);
        if(pps->slice_group_map_type == 0)
        {
            for(iGroup=0; iGroup<=pps->num_slice_groups_minus1; iGroup++)
            {
                pps->run_length_minus1[iGroup] = parse_ue(buf);
            }
        }
        else if(pps->slice_group_map_type == 2)
        {
            for(iGroup=0; iGroup<=pps->num_slice_groups_minus1; iGroup++)
            {
                pps->top_left[iGroup] = parse_ue(buf);
                pps->bottom_right[iGroup] = parse_ue(buf);
            }
        }
        else if(pps->slice_group_map_type == 3 \
                ||pps->slice_group_map_type == 4\
                ||pps->slice_group_map_type == 5)
        {
            pps->slice_group_change_direction_flag = get_1bit(buf);
            pps->slice_group_change_rate_minus1 = parse_ue(buf);
        }
        else if(pps->slice_group_map_type == 6)
        {
            pps->pic_size_in_map_units_minus1 = parse_ue(buf);
            for(i=0; i<pps->pic_size_in_map_units_minus1; i++)
            {
                /*这地方可能有问题，对u(v)理解偏差*/
                pps->slice_group_id[i] = get_bits(buf, pps->pic_size_in_map_units_minus1);
            }
        }
    }

    pps->num_ref_idx_10_active_minus1 = parse_ue(buf);
    pps->num_ref_idx_11_active_minus1 = parse_ue(buf);
    pps->weighted_pred_flag = get_1bit(buf);
    pps->weighted_bipred_idc = get_bits(buf, 2);
    pps->pic_init_qp_minus26 = parse_se(buf); /*relative26*/
    pps->pic_init_qs_minus26 = parse_se(buf); /*relative26*/
    pps->chroma_qp_index_offset = parse_se(buf);
    pps->deblocking_filter_control_present_flag = get_1bit(buf);
    pps->constrained_intra_pred_flag = get_1bit(buf);
    pps->redundant_pic_cnt_present_flag = get_1bit(buf);

    if(more_rbsp_data(buf))
    {
        pps->transform_8x8_mode_flag = get_1bit(buf);
        pps->pic_scaling_matrix_present_flag = get_1bit(buf);
        if(pps->pic_scaling_matrix_present_flag)
        {
            for(i=0; i<6+2*pps->transform_8x8_mode_flag; i++)
            {
                pps->pic_scaling_list_present_flag[i] = get_1bit(buf);
                if(pps->pic_scaling_list_present_flag[i])
                {
                    if(i<6)
                    {
                        for(j=0; j<16; j++)
                        {
                            next_scale = 8;
                            last_scale = 8;
                            if(next_scale != 0)
                            {
                                delta_scale = parse_se(buf);
                                next_scale = (last_scale + delta_scale + 256)%256;
                                pps->UseDefaultScalingMatrix4x4Flag[i] = ((j == 0) && (next_scale == 0));
                            }
                            pps->ScalingList4x4[i][j] = (next_scale == 0)?last_scale:next_scale;
                            last_scale = pps->ScalingList4x4[i][j];
                        }
                    }
                    else
                    {
                        int ii = i-6;
                        next_scale = 8;
                        last_scale = 8;
                        for(j=0; j<64; j++)
                        {
                            if(next_scale != 0)
                            {
                                delta_scale = parse_se(buf);
                                next_scale = (last_scale + delta_scale + 256)%256;
                                pps->UseDefaultScalingMatrix8x8Flag[ii] = ((j == 0) && (next_scale == 0));
                            }
                            pps->ScalingList8x8[ii][j] = (next_scale == 0)?last_scale:next_scale;
                            last_scale = pps->ScalingList8x8[ii][j];
                        }
                    }
                }
            }

            pps->second_chroma_qp_index_offset = parse_se(buf);
        }
    }

exit:
    get_bit_context_free(buf);
    return ret;
}


// calculation width height and framerate
int h264_get_width(SPS *sps_ptr)
{
    return (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
}

int h264_get_height(SPS *sps_ptr)
{
    return (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16 * (2 - sps_ptr->frame_mbs_only_flag);
}

int h264_get_format(SPS *sps_ptr)
{
    return sps_ptr->frame_mbs_only_flag;
}


/*
1 - 23.98
2 - 24
3 - 25
4 - 29.97
5 - 30
6 - 50
7 - 59.94
8 - 60
9 - 6
10 - 8
11 - 12
12 - 15
13 - 10.
*/
int h264_get_framerate(float *framerate,SPS *sps_ptr)
{
    int fr;
    int fr_int;
    if(sps_ptr->vui_parameters.timing_info_present_flag)
    {
        if(sps_ptr->frame_mbs_only_flag)
        {
            *framerate = (float)sps_ptr->vui_parameters.time_scale / (float)sps_ptr->vui_parameters.num_units_in_tick;
            //fr_int = sps_ptr->vui_parameters.time_scale / sps_ptr->vui_parameters.num_units_in_tick;
        }else
        {
            *framerate = (float)sps_ptr->vui_parameters.time_scale / (float)sps_ptr->vui_parameters.num_units_in_tick / 2.0;
            //fr_int = sps_ptr->vui_parameters.time_scale / sps_ptr->vui_parameters.num_units_in_tick / 2;
        }
        fr_int = sps_ptr->vui_parameters.time_scale / sps_ptr->vui_parameters.num_units_in_tick / 2;
    }
    switch(fr_int)
    {
        case 23:// 23.98
            fr = 1;
            break;
        case 24:
            fr = 2;
            break;
        case 25:
            fr = 3;
            break;
        case 29://29.97
            fr = 4;
            break;
        case 30:
            fr = 5;
            break;
        case 50:
            fr = 6;
            break;
        case 59://59.94
            fr = 7;
            break;
        case 60:
            fr = 8;
            break;
        case 6:
            fr = 10;
            break;
        case 8:
            fr = 11;
            break;
        case 12:
            fr = 12;
            break;
        case 15:
            fr = 13;
            break;
        case 10:
            fr = 14;
            break;
            
        default:
            fr = 0;
            break;
    }

    return fr;
}


// for mpeg-2 
void memcpy_sps_data(uint8_t *dst,uint8_t *src,int len)
{
    int tmp;
    for(tmp = 0; tmp < len; tmp++)
    {
        //printf("0x%02x ", src[tmp]);
        dst[(tmp/4)*4+(3 - (tmp % 4))] = src[tmp];
    }
}
/*_*/



static int ff_startcode_find_candidate_c(const uint8_t *buf, int size)
{
    int i = 0;
    for (; i < size; i++)
        if (!buf[i])
            break;
    return i;
}




int h264get_sps_and_pps(ParseContext *pc, const uint8_t *buf, int buf_size){
	pc->buffer = buf;
	pc->buffer_size = buf_size;


	int32_t  is_avc = 0;
	int32_t  nal_length_size = 0;

    int i, j;
    uint32_t state;
    //ParseContext *pc = &h->parse_context;
    int next_avc= is_avc ? 0 : buf_size;

//    mb_addr= pc->mb_addr - 1;
    state = pc->state;
    if (state > 13)
        state = 7;

    if (is_avc && !nal_length_size)
        printf("AVC-parser: nal length size invalid\n");
    //
    //每次循环前进1个字节，读取该字节的值
    //根据此前的状态state做不同的处理
    //state取值为4,5代表找到了起始码
    //类似于一个状态机，简单画一下状态转移图：
    //                            +-----+
    //                            |     |
    //                            v     |
    // 7--(0)-->2--(0)-->1--(0)-->0-(0)-+
    // ^        |        |        |
    // |       (1)      (1)      (1)
    // |        |        |        |
    // +--------+        v        v
    //                   4        5
    //
    for (i = 0; i < buf_size; i++) {
    	//超过了
        if (i >= next_avc) {
            int nalsize = 0;
            i = next_avc;
            for (j = 0; j < nal_length_size; j++)
                nalsize = (nalsize << 8) | buf[i++];
            if (nalsize <= 0 || nalsize > buf_size - i) {
                printf("AVC-parser: nal size %d remaining %d\n", nalsize, buf_size - i);
                return buf_size;
            }
            next_avc = i + nalsize;
            state    = 5;
        }
        //初始state为7
        if (state == 7) {
        	//查找startcode的候选者？
        	//从一段内存中查找取值为0的元素的位置并返回
        	//增加i取值
            i += ff_startcode_find_candidate_c(buf + i, next_avc - i);
            //因为找到1个0，状态转换为2
            if (i < next_avc)
                state = 2;
        } else if (state <= 2) {       //找到0时候的state。包括1个0（状态2），2个0（状态1），或者3个及3个以上0（状态0）。
            if (buf[i] == 1)           //发现了一个1
                state ^= 5;            //状态转换关系：2->7, 1->4, 0->5。状态4代表找到了001，状态5代表找到了0001
            else if (buf[i])
                state = 7;             //恢复初始
            else                       //发现了一个0
                state >>= 1;           // 2->1, 1->0, 0->0
        } else if (state <= 5) {
        	//状态4代表找到了001，状态5代表找到了0001
        	//获取NALU类型
        	//NALU Header（1Byte）的后5bit
            int nalu_type = buf[i] & 0x1F;

            if (nalu_type == H264_NAL_SEI || nalu_type == H264_NAL_SPS ||
                nalu_type == H264_NAL_PPS || nalu_type == H264_NAL_AUD) {
                //SPS，PPS，SEI类型的NALU
                if (nalu_type == H264_NAL_SPS){
					pc->sps_start_pos = i - 4; 
				} else if (nalu_type == H264_NAL_PPS ){
					pc->pps_start_pos = i - 4; 
					pc->sps_size = pc->pps_start_pos - pc->sps_start_pos;
				}
					
            } else if (nalu_type == H264_NAL_SLICE || nalu_type == H264_NAL_DPA ||
                       nalu_type == H264_NAL_IDR_SLICE) {
            	//表示有slice header的NALU
            	//大于等于8的状态表示找到了两个帧头，但没有找到帧尾的状态
            	if (pc->sps_size != 0) {
					pc->pps_size = i - pc->pps_start_pos;
				
					state += 8;
					goto found;	
				}
				/*
                continue;
				*/
            }
            //上述两个条件都不满足，回归初始状态（state取值7）
            state = 7;
        } else {
			continue;
        }
    }
    pc->state = state;
	pc->frame_start_found = 0;
	pc->buffer_pos = i;

    if (is_avc)
        return next_avc;
    //没找到
    return END_NOT_FOUND;

found:
    pc->state             = 7;
    pc->frame_start_found = 1;
	pc->buffer_pos        = i;
    if (is_avc)
        return next_avc;
    //state=4时候，state & 5=4
    //找到的是001（长度为3），i减小3+1=4，标识帧结尾
    //state=5时候，state & 5=5
    //找到的是0001（长度为4），i减小4+1=5，标识帧结尾
    return i - (state & 5) - 5 * (state > 7);
}



