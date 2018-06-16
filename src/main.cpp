#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>


#include "file_io.h"
#include "sps_pps.h"

using namespace nsp_file_io;

class CH264FileReader : public CFileIO {
public:
	CH264FileReader();
	virtual ~CH264FileReader();

	int init(const std::string& file);
	int uninit();

	int open_file();
	int close_file();

	int read_file(unsigned char*& buf, const int32_t size);

private:
	std::string        m_in_file;
};

CH264FileReader::CH264FileReader(){}
CH264FileReader::~CH264FileReader(){}

int CH264FileReader::init(const std::string& file){
	m_in_file = file;
	int ret = open_file();

	return 0;
}

int CH264FileReader::uninit(){
	close_file();
	return 0;
}

int CH264FileReader::open_file(){
	int ret = io_fd_open_in_file(m_in_file);
	if (ret != 0){
		std::cout << "Open input file " << m_in_file << " failed!" << endl;
		return -1;
	}
	return 0;
}

int CH264FileReader::close_file(){
	int ret = io_fd_close();
	if (ret != 0) {
		std::cout << "Close file failed!" << endl;
		return -1;
	}
	return 0;
}

int CH264FileReader::read_file(unsigned char*& buf, const int32_t size){
	buf = new unsigned char [size];
	int ret = io_fd_read_size(buf, size);
	if (ret < 0){
		std::cout << "read file failed" << endl;
		return -1;
	}
	return 0;
}





int main(int argc,char** argv) {
	std::string in_file;
	int32_t     read_size = 0;

	// Check input parameter
    char ch;
    while((ch = getopt(argc, argv, "i:s:h")) != -1){
        switch(ch){
            case 'i':
                printf("option is %c,  content is : %s\n", ch, optarg);
				in_file = optarg;
                break;
            case 's':
                printf("option is %c,  content is : %s\n", ch, optarg);
			    read_size = atoi(optarg);
                break; 
            case 'h':
                printf("option is %c\n", ch);
                printf("-i <input_file> -s <read_size>\n");
                printf("-h help\n");
                exit(0);
            default:
                printf("other option: %c\n", ch);
                exit(0);
        }
    }

	unsigned char *data_buffer = NULL;
	CH264FileReader *ins_file_reader = new CH264FileReader;

	ins_file_reader->init(in_file);	
	ins_file_reader->read_file(data_buffer, read_size);


	// obtain sps and pps  
	ParseContext h264_pc = {0};
	int ret = h264get_sps_and_pps(&h264_pc, data_buffer, read_size);
	if (ret < 0) {
		std::cout << "Cann't find sps and pps , over!" << endl;
		return -1;
	}

	// Parse SPS 
	uint8_t *sps_buffer = new uint8_t[h264_pc.sps_size];
	memcpy(sps_buffer, data_buffer+h264_pc.sps_start_pos, h264_pc.sps_size);
	
	get_bit_context  buffer;
	SPS h264_sps_context;

	memset(&buffer, 0, sizeof(get_bit_context));
	buffer.buf      = sps_buffer+5;
	buffer.buf_size = h264_pc.sps_size;
	h264dec_seq_parameter_set(&buffer, &h264_sps_context);
		
	std::cout << "Pic width : " << h264_get_width(&h264_sps_context) << endl;
	std::cout << "Pic height: " << h264_get_height(&h264_sps_context) << endl;

	delete sps_buffer;


	// Parse PPS 
	PPS h264_pps_context;
	uint8_t *pps_buffer = new uint8_t[h264_pc.pps_size];
	memcpy(pps_buffer, data_buffer+h264_pc.pps_start_pos, h264_pc.pps_size);

	memset(&buffer, 0, sizeof(get_bit_context));
	buffer.buf      = pps_buffer+5;
	buffer.buf_size = h264_pc.pps_size;

	h264dec_picture_parameter_set(&buffer, &h264_pps_context);

	delete pps_buffer;


    // Release 
	ins_file_reader->uninit();	
	if (data_buffer != NULL){
		delete data_buffer;
		data_buffer = NULL;
	}
	delete ins_file_reader;

	return 0;
}
