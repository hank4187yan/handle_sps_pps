#ifndef __FILE_IO_H__
#define __FILE_IO_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>


/*
 * The following shows the relationship between buffer, buf_ptr, buf_end, buf_size,
 * and pos, when reading and when writing (since file_io is used for both):
 *
 **********************************************************************************
 *                                   READING
 **********************************************************************************
 *
 *                            |              buffer_size              |
 *                            |---------------------------------------|
 *                            |                                       |
 *
 *                         buffer          buf_ptr       buf_end
 *                            +---------------+-----------------------+
 *                            |/ / / / / / / /|/ / / / / / /|         |
 *  read buffer:              |/ / consumed / | to be read /|         |
 *                            |/ / / / / / / /|/ / / / / / /|         |
 *                            +---------------+-----------------------+
 *
 *                                                         pos
 *              +-------------------------------------------+-----------------+
 *  input file: |                                           |                 |
 *              +-------------------------------------------+-----------------+
 *
 *
 **********************************************************************************
 *                                   WRITING
 **********************************************************************************
 *
 *                                          |          buffer_size          |
 *                                          |-------------------------------|
 *                                          |                               |
 *
 *                                       buffer              buf_ptr     buf_end
 *                                          +-------------------+-----------+
 *                                          |/ / / / / / / / / /|           |
 *  write buffer:                           | / to be flushed / |           |
 *                                          |/ / / / / / / / / /|           |
 *                                          +-------------------+-----------+
 *
 *                                         pos
 *               +--------------------------+-----------------------------------+
 *  output file: |                          |                                   |
 *               +--------------------------+-----------------------------------+
 *
 */


using namespace std;

#define FILE_WRITE_FLAGS O_WRONLY | O_CREAT | O_TRUNC 
#define FILE_WRITE_MODE S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH  



#define IO_BUFFER_SIZE     32768
#define MAX_READ_PKG_SIZE  2048 

#define FFMIN(a,b) ((a) > (b) ? (b) : (a)) 
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)

#define IO_OK          0
#define IO_ERROR_EOF   1

#define MOVTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define MOVBYTETAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

#define MOVBOXTYPE(a,b,c,d)  (a),(b),(c),(d)


namespace nsp_file_io {
class CFileIO {
public:
	CFileIO();
	virtual ~CFileIO();

	
public:
	int			io_fd_open_in_file(const std::string& file);
	int			io_fd_close();
	int			io_fd_read_size(unsigned char* buf, int size);
	int			io_fd_seek();


private:
	int             m_fd;
	int64_t			m_pos_file;
	int64_t         m_bytes_read;
	int             m_eof_reached;
	int             m_error;
	
		

public:
	int			io_read(unsigned char* buf, int size);
	int			io_seek();
	void        fill_buffer();

private:
	uint8_t			*m_buffer;
	uint8_t			*m_buf_ptr;
	uint8_t			*m_buf_end;
	int32_t			m_buffer_size; 



public:
	int			io_fd_open_out_file(const std::string& file);
	int			io_fd_write_size(unsigned char* frame_buf, int32_t size);


};

}
#endif
