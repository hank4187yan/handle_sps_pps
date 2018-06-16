#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h> 

#include "file_io.h"

using namespace nsp_file_io;

CFileIO::CFileIO(){
	m_buffer = NULL;
}

CFileIO::~CFileIO() {
	m_buffer = NULL;
}





int CFileIO::io_fd_open_in_file(const std::string& file) {
	unsigned int mode   = 0;
	int          access = O_RDONLY;
	
	m_fd = open(file.c_str(), access, mode);
    if (m_fd == -1){
		printf("Open file %s failed\n", file.c_str());
		return -1;
	}	

	// new buffer
	m_buffer_size = IO_BUFFER_SIZE;
	m_buffer = (uint8_t*)malloc(m_buffer_size*sizeof(uint8_t));
	if (m_buffer == NULL){
		printf("Allocate memory failed!\n");
		return -1;
	}

	memset(m_buffer, 0, m_buffer_size*sizeof(uint8_t));
	m_buf_ptr = m_buf_end = m_buffer;

	m_pos_file    = 0;
	m_bytes_read  = 0;
	m_eof_reached = 0;
	m_error       = 0;

	return 0;
}


int CFileIO::io_fd_close() {
	int ret = close(m_fd);
	if (ret != 0){
		printf("Close file failed!\n");
		return -1;
	}


	// delete buffer
	if (m_buffer != NULL){
		free(m_buffer);
		m_buf_ptr = m_buf_end = NULL;
	}
	
	return 0;
}


int CFileIO::io_fd_read_size(unsigned char* buf, int size) {
	
	int ret = io_read(buf, size);
	if (ret != size) {
		return -1;
	}
	
	return IO_OK;	
}




int CFileIO::io_read(unsigned char*  buf, int size) {
	int len, size1;
	
	size1 = size;
	while(size > 0) {
		len = FFMIN(m_buf_end - m_buf_ptr, size);
		if ( len == 0){
			if (size > m_buffer_size){
				// bypass the buffer and read data directly into buf	
				printf("Uncomplete: bypass the buffer and read data directly into buf\n");
				return -1;
			} else {
				fill_buffer();
				len = m_buf_end - m_buf_ptr;
				if (len == 0){
					break;
				}
			}
		} else {
			memcpy(buf, m_buf_ptr, len);
			buf += len;
			m_buf_ptr += len;
			size -= len;
		}
		
	}	
	
	if (size1 == size){
		//if (io_feof()) return IO_ERROR_EOF; 	
	}

	return size1 - size;	
}

void CFileIO::fill_buffer(){
	int max_buffer_size = IO_BUFFER_SIZE;
	uint8_t *dst		= (m_buf_end - m_buffer +  max_buffer_size) <  m_buffer_size ?
							m_buf_end : m_buffer;
	int len				= m_buffer_size - (dst - m_buffer);

	/* no need to do anything if EOF already reached */
	if (m_eof_reached)
		return;

	len = read(m_fd, dst, len);
	if (len <= 0){
		m_eof_reached = 1;
		if (len < 0){
			m_error = len;
		}
	} else {
		m_pos_file += len;
		m_buf_ptr   = dst;
		m_buf_end   = dst + len;
		m_bytes_read += len;
	}
		
}


int CFileIO::io_seek() {

	return 0;	
}



int CFileIO::io_fd_open_out_file(const std::string& file) {
	unsigned int mode   = FILE_WRITE_MODE;
	int          access = FILE_WRITE_FLAGS;
	
	m_fd = open(file.c_str(), access, mode);
    if (m_fd == -1){
		printf("Open file %s failed\n", file.c_str());
		return -1;
	}	

	return 0;
}



int CFileIO::io_fd_write_size(unsigned char* frame_buf, int32_t size){
	write(m_fd, frame_buf, size);
	return 0;
}

