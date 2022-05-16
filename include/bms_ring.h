#ifndef __LXZ_RING_H__
#define __LXZ_RING_H__

typedef unsigned char uint08;
typedef int sint32;

typedef struct _lxz_ring_t
{
    uint08 * pt_data_buf;
    sint32   it_buf_size;
    sint32   it_pos_read;                                   
    sint32   it_pos_write;
    sint32   it_pos_preread;
	sint32   it_pps_Valid;      // 0 means invalid; Non-zero means valid
    unsigned long long  TimeStamp;
} lxz_ring_t;

typedef enum _LXZ_RING_OPTION_E
{
    E_LRO_BUFF_SIZE,
    E_LRO_FREE_SIZE,
    E_LRO_DATA_SIZE,
    E_LRO_NEXT_READ_SIZE,
    E_LRO_NEXT_WRITE_SIZE,
    E_LRO_INVALID =0xFF
} LXZ_RING_OPTION_E;
	
/*
 *  Description:
 *    Create a ring buffer for <data_size> characters.
 *  Param: i_buf_size, the size of buffer;
 *  Return: ring descriptor or NULL in case of error;
 *  Note: NONE
 */
lxz_ring_t * lxz_ring_f_create(sint32 i_buf_size);

/*
 *  Description:
 *    Get characters from ring buffer(but not clear data).
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_data_buf, the pointer of buffer;
 *  Param: i_data_len, the size of buffer;
 *  Return: i_len_read, the length of data has been read;
 *  Note: NONE
 */
sint32 lxz_ring_f_preread(lxz_ring_t * p_dst_ring, uint08 * p_data_buf, sint32 i_buf_size);

/*
 *  Description:
 *    Get characters from ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_data_buf, the pointer of buffer;
 *  Param: i_data_len, the size of buffer;
 *  Return: i_len_read, the length of data has been read;
 *  Note: NONE
 */
sint32 lxz_ring_f_read(lxz_ring_t * p_dst_ring, uint08 * p_data_buf, sint32 i_buf_size);

/*
 *  Description:
 *    Put characters into ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_data_buf, the pointer of buffer;
 *  Param: i_data_len, length of data;
 *  Return: i_len_write, the length of data has been written;
 *  Note: NONE
 */
sint32 lxz_ring_f_write(lxz_ring_t * p_dst_ring, uint08 * p_data_buf, sint32 i_date_len);

/*
 *  Description:
 *    Get a character from ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_char_buf, a buffer for saving character;
 *  Return: i_len_read, 0, fail;1, success.
 *  Note: NONE
 */
sint32 lxz_ring_f_readchar(lxz_ring_t * p_dst_ring, uint08 * p_char_buf);

/*
 *  Description:
 *    Put a character into ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: u_char_value, a character;
 *  Return: i_len_write, 0, fail;1, success.
 *  Note: NONE
 */
sint32 lxz_ring_f_writechar(lxz_ring_t * p_dst_ring, uint08 u_char_value);

/*
 *  Description:
 *    Get a character from ring buffer(but not clear data).
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_char_buf, a buffer for saving character;
 *  Return: i_len_read, 0, fail;1, success.
 *  Note: NONE
 */
sint32 lxz_ring_f_preread_getfirstchar(lxz_ring_t * p_dst_ring, uint08 * p_char_buf);

/*
 *  Description:
 *    Get a character from ring buffer(but not clear data).
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_char_buf, a buffer for saving character;
 *  Return: i_len_read, 0, fail;1, success.
 *  Note: lxz_ring_f_preread_getfirstchar should be called and return 1.
 */
sint32 lxz_ring_f_preread_getnextchar(lxz_ring_t * p_dst_ring, uint08 * p_char_buf);

/*
 *  Description:
 *    Get information according to the type.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: i_opt_type, the specified type;
 *  Return: i_ret_value, the length/size of the type.
 *  Note: None
 */
sint32 lxz_ring_f_getoption(lxz_ring_t * p_dst_ring, sint32 i_opt_type);

/*
 *  Description:
 *    Deletes all read elements from the buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Return: NONE;
 *  Note: NONE
 */
void lxz_ring_f_clear(lxz_ring_t * p_dst_ring);

/*
 *  Description:
 *    Deletes a ring buffer and then set ring descriptor to NULL.
 *  Param: p_dst_ring, pointer to a ring descriptor;
 *  Return: NONE;
 *  Note: NONE
 */
void lxz_ring_f_delete(lxz_ring_t * p_dst_ring);

#endif /* __LXZ_RING_H__ */

