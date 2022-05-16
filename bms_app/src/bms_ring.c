#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "bms_ring.h"
/*
 *  Description:
 *    Create a ring buffer for <data_size> characters.
 *  Param: i_buf_size, the size of buffer;
 *  Return: ring descriptor or NULL in case of error;
 *  Note: NONE
 */
lxz_ring_t * lxz_ring_f_create(sint32 i_buf_size)
{
    lxz_ring_t * p_new_ring = NULL;

    p_new_ring = (lxz_ring_t *)malloc(sizeof(lxz_ring_t));
    if (p_new_ring == NULL)
    {
        return p_new_ring;
    }
    
    memset (p_new_ring, 0, sizeof(lxz_ring_t));
    p_new_ring->pt_data_buf = (uint08 *)malloc(i_buf_size);
    if (p_new_ring->pt_data_buf == NULL)
    {
        free(p_new_ring);
        p_new_ring = NULL;
        return p_new_ring;
    }

    p_new_ring->it_buf_size = i_buf_size;
	p_new_ring->it_pos_read = 0;
	p_new_ring->it_pos_write = 0;
    memset(p_new_ring->pt_data_buf, 0, i_buf_size);

    return p_new_ring;
}

/*
 *  Description:
 *    Deletes a ring buffer and then set ring descriptor to NULL.
 *  Param: p_dst_ring, pointer to a ring descriptor;
 *  Return: NONE;
 *  Note: NONE
 */
void lxz_ring_f_delete(lxz_ring_t * p_dst_ring)
{
    if (p_dst_ring != NULL)
    {
        if (p_dst_ring->pt_data_buf != NULL)
        {
            free(p_dst_ring->pt_data_buf);
            p_dst_ring->pt_data_buf = NULL;
        }

        free(p_dst_ring);
        p_dst_ring = NULL;
    }
}

/*
 *  Description:
 *    Deletes all read elements from the buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Return: NONE;
 *  Note: NONE
 */
void lxz_ring_f_clear(lxz_ring_t * p_dst_ring)
{
    if (p_dst_ring != NULL)
    {
        p_dst_ring->it_pos_read = 0;
        p_dst_ring->it_pos_write= 0;
    }
}

/*
 *  Description:
 *    Put characters into ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_data_buf, the pointer of buffer;
 *  Param: i_data_len, length of data;
 *  Return: i_len_write, the length of data has written;
 *  Note: NONE
 */
sint32 lxz_ring_f_write(lxz_ring_t * p_dst_ring, uint08 * p_data_buf, sint32 i_data_len)
{
    sint32 i_len_write = 0;
    
    if (p_dst_ring != NULL)
    {
        while (i_len_write < i_data_len)
        {
            sint32 i_len_write1 = 0;

            i_len_write1 = lxz_ring_f_writechar(p_dst_ring, p_data_buf[i_len_write]);
            if (i_len_write1 == 0)
            {
                break;
            }
            
            i_len_write++;
        }
    }
    return i_len_write;
}

/*
 *  Description:
 *    Get a character from ring buffer(but not clear data).
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_char_buf, a buffer for saving character;
 *  Return: i_len_read, 0, fail;1, success.
 *  Note: NONE
 */
sint32 lxz_ring_f_preread_getfirstchar(lxz_ring_t * p_dst_ring, uint08 * p_char_buf)
{
    sint32 i_len_read = 0;

    if (p_dst_ring != NULL)
    {
        p_dst_ring->it_pos_preread = p_dst_ring->it_pos_read;
        if (p_dst_ring->it_pos_preread != p_dst_ring->it_pos_write)
        {
            /* 读取数，仅更新预读指针 */
            *p_char_buf = p_dst_ring->pt_data_buf[p_dst_ring->it_pos_preread];
            if (p_dst_ring->it_pos_preread + 1 < p_dst_ring->it_buf_size)
            {
                p_dst_ring->it_pos_preread++;
            }
            else
            {
                p_dst_ring->it_pos_preread = 0;
            }

            i_len_read = 1;
        }
    }

    return i_len_read;
}

/*
 *  Description:
 *    Get a character from ring buffer(but not clear data).
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_char_buf, a buffer for saving character;
 *  Return: i_len_read, 0, fail;1, success.
 *  Note: lxz_ring_f_preread_getfirstchar should be called and return 1.
 */
sint32 lxz_ring_f_preread_getnextchar(lxz_ring_t * p_dst_ring, uint08 * p_char_buf)
{
    sint32 i_len_read = 0;

    if (p_dst_ring != NULL)
    {
        if (p_dst_ring->it_pos_preread != p_dst_ring->it_pos_write)
        {
            /* 读取数，仅更新预读指针 */
            *p_char_buf = p_dst_ring->pt_data_buf[p_dst_ring->it_pos_preread];
            if (p_dst_ring->it_pos_preread + 1 < p_dst_ring->it_buf_size)
            {
                p_dst_ring->it_pos_preread++;
            }
            else
            {
                p_dst_ring->it_pos_preread = 0;
            }

            i_len_read = 1;
        }
    }

    return i_len_read;
}

/*
 *  Description:
 *    Get a character from ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_char_buf, a buffer for saving character;
 *  Return: i_len_read, 0, fail;1, success.
 *  Note: NONE
 */
sint32 lxz_ring_f_readchar(lxz_ring_t * p_dst_ring, uint08 * p_char_buf)
{
    sint32 i_len_read = 0;

    sint32 i_pos_tail = 0;
    sint32 i_len_readable1 = 0;
    sint32 i_len_readable2 = 0;

    if (p_dst_ring != NULL)
    {
        if (p_dst_ring->it_pos_read != p_dst_ring->it_pos_write)
        {
            /* 读取数并更新读指针 */
            *p_char_buf = p_dst_ring->pt_data_buf[p_dst_ring->it_pos_read];
            if (p_dst_ring->it_pos_read + 1 < p_dst_ring->it_buf_size)
            {
                p_dst_ring->it_pos_read++;
            }
            else
            {
                p_dst_ring->it_pos_read = 0;
            }
        }
    }

    return 0;
}

/*
 *  Description:
 *    Put a character into ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: u_char_value, a character;
 *  Return: i_len_write, 0, fail;1, success.
 *  Note: NONE
 */
sint32 lxz_ring_f_writechar(lxz_ring_t * p_dst_ring, uint08 u_char_value)
{
    sint32 i_len_write = 0;

    sint32 i_pos_tail = 0;
    sint32 i_nxt_pos_write = 0;

    if (p_dst_ring != NULL)
    {
        /* 计算下一个可写位置 */
        if (p_dst_ring->it_pos_write + 1 < p_dst_ring->it_buf_size)
        {
            i_nxt_pos_write = p_dst_ring->it_pos_write + 1;
        }
        else
        {
            i_nxt_pos_write = 0;
        }

        /* 如ring未满，写入指定字符 */
        if (i_nxt_pos_write != p_dst_ring->it_pos_read)
        {
            p_dst_ring->pt_data_buf[p_dst_ring->it_pos_write] = u_char_value;
            p_dst_ring->it_pos_write = i_nxt_pos_write;

            i_len_write = 1;
        }
    }
    
    return i_len_write;
}

/*
 *  Description:
 *    Get characters from ring buffer(but not clear data).
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_data_buf, the pointer of buffer;
 *  Param: i_data_len, the size of buffer;
 *  Return: i_len_read, the length of data has been read;
 *  Note: NONE
 */
sint32 lxz_ring_f_preread(lxz_ring_t * p_dst_ring, uint08 * p_data_buf, sint32 i_buf_size)
{
    sint32 i_len_read = 0;

    sint32 i_pos_tail = 0;
    sint32 i_len_readable1 = 0;
    sint32 i_len_readable2 = 0;

    if (p_dst_ring != NULL)
    {
        if (p_dst_ring->it_pos_read <= p_dst_ring->it_pos_write)
        {
            /* 缓冲区中的数据量 */
            i_len_readable1 = p_dst_ring->it_pos_write - p_dst_ring->it_pos_read;

            /* 本次操作可预读数据的量 */
            if (i_buf_size <= i_len_readable1)
            {
                i_len_readable1 = i_buf_size;
            }

            /* 读取数,但不更新读指针 */
            if (i_len_readable1 > 0)
            {
                memcpy(p_data_buf, &(p_dst_ring->pt_data_buf[p_dst_ring->it_pos_read]), i_len_readable1);
                i_len_read = i_len_readable1;
            }
        }
        else
        {
            /* 缓冲区中的数据量 */
            i_len_readable1 = p_dst_ring->it_buf_size - p_dst_ring->it_pos_read;
            i_len_readable2 = p_dst_ring->it_pos_write;

            /* 本次操作可预读数据的量 */
            if (i_buf_size <= i_len_readable1)
            {
                /* 读取部分或全部第一块，不读第二块 */
                i_len_readable1 = i_buf_size;
                i_len_readable2 = 0;
            }
            else
            {
                if (i_buf_size <= i_len_readable1 + i_len_readable2)
                {
                    /* 读取完整的第一块，部分或全部第二块 */
                    i_len_readable2 = i_buf_size - i_len_readable1;
                }
            }
            
            /* 读取数据,但不更新读指针 */
            if (i_len_readable1 > 0)
            {
                memcpy(p_data_buf, &(p_dst_ring->pt_data_buf[p_dst_ring->it_pos_read]), i_len_readable1);
                i_len_read = i_len_readable1;
            }

            if (i_len_readable2 > 0)
            {
                memcpy(&(p_data_buf[i_len_read]), &(p_dst_ring->pt_data_buf[0]), i_len_readable2);
                i_len_read += i_len_readable2;
            }
        }
    }

    return i_len_read;
}

/*
 *  Description:
 *    Get characters from ring buffer.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: p_data_buf, the pointer of buffer;
 *  Param: i_data_len, the size of buffer;
 *  Return: i_len_read, the length of data has been read;
 *  Note: NONE
 */
sint32 lxz_ring_f_read(lxz_ring_t * p_dst_ring, uint08 * p_data_buf, sint32 i_buf_size)
{
    sint32 i_len_read = 0;
    
    sint32 i_pos_tail = 0;
    sint32 i_len_readable1 = 0;
    sint32 i_len_readable2 = 0;
    
    if (p_dst_ring != NULL)
    {
        if (p_dst_ring->it_pos_read <= p_dst_ring->it_pos_write)
        {
            /* 缓冲区中的数据量 */
            i_len_readable1 = p_dst_ring->it_pos_write - p_dst_ring->it_pos_read;
            
            /* 本次操作可预读数据的量 */
            if (i_buf_size <= i_len_readable1)
            {
                i_len_readable1 = i_buf_size;
            }
            
            /* 读取数并更新读指针 */
            if (i_len_readable1 > 0)
            {
                memcpy(p_data_buf, &(p_dst_ring->pt_data_buf[p_dst_ring->it_pos_read]), i_len_readable1);
                i_len_read = i_len_readable1;
                p_dst_ring->it_pos_read += i_len_read;
            }
        }
        else
        {
            /* 缓冲区中的数据量 */
            i_len_readable1 = p_dst_ring->it_buf_size - p_dst_ring->it_pos_read;
            i_len_readable2 = p_dst_ring->it_pos_write;
            
            /* 本次操作可预读数据的量 */
            if (i_buf_size <= i_len_readable1)
            {
                /* 读取部分或全部第一块，不读第二块 */
                i_len_readable1 = i_buf_size;
                i_len_readable2 = 0;
            }
            else
            {
                if (i_buf_size <= i_len_readable1 + i_len_readable2)
                {
                    /* 读取完整的第一块，部分或全部第二块 */
                    i_len_readable2 = i_buf_size - i_len_readable1;
                }
            }
            
            /* 读取数据并更新读指针 */
            if (i_len_readable1 > 0)
            {
                memcpy(p_data_buf, &(p_dst_ring->pt_data_buf[p_dst_ring->it_pos_read]), i_len_readable1);
                i_len_read = i_len_readable1;
                p_dst_ring->it_pos_read += i_len_read;
            }
            
            if (i_len_readable2 > 0)
            {
                memcpy(&(p_data_buf[i_len_read]), &(p_dst_ring->pt_data_buf[0]), i_len_readable2);
                i_len_read += i_len_readable2;
                p_dst_ring->it_pos_read = i_len_readable2;
            }
        }
    }
    
    return i_len_read;
}

/*
 *  Description:
 *    Get information according to the type.
 *  Param: p_dst_ring, ring descriptor;
 *  Param: i_opt_type, the specified type;
 *  Return: i_ret_value, the length/size of the type.
 *  Note: None
 */
sint32 lxz_ring_f_getoption(lxz_ring_t * p_dst_ring, sint32 i_opt_type)
{
    sint32 i_ret_value = 0;

    if (p_dst_ring != NULL)
    {
        switch (i_opt_type)
        {
        case E_LRO_BUFF_SIZE:
            {
                i_ret_value = p_dst_ring->it_buf_size;
                break;
            }
        case E_LRO_FREE_SIZE:
            {
                if (p_dst_ring->it_pos_read != p_dst_ring->it_pos_write)
                {
                    if (p_dst_ring->it_pos_read < p_dst_ring->it_pos_write)
                    {
                        i_ret_value = p_dst_ring->it_buf_size - p_dst_ring->it_pos_write + p_dst_ring->it_pos_read - 1;
                    }
                    else
                    {
                        i_ret_value = p_dst_ring->it_pos_read - p_dst_ring->it_pos_write - 1;
                    }
                }
                else
                {
                    i_ret_value = p_dst_ring->it_buf_size -1;
                }
                break;
            }
        case E_LRO_DATA_SIZE:
            {
                if (p_dst_ring->it_pos_read != p_dst_ring->it_pos_write)
                {
                    if (p_dst_ring->it_pos_read < p_dst_ring->it_pos_write)
                    {
                        i_ret_value = p_dst_ring->it_pos_write - p_dst_ring->it_pos_read;
                    }
                    else
                    {
                        i_ret_value = p_dst_ring->it_buf_size - p_dst_ring->it_pos_read + p_dst_ring->it_pos_write;
                    }
                }
                break;
            }
        case E_LRO_NEXT_READ_SIZE:
            {
                if (p_dst_ring->it_pos_read != p_dst_ring->it_pos_write)
                {
                    if (p_dst_ring->it_pos_read < p_dst_ring->it_pos_write)
                    {
                        i_ret_value = p_dst_ring->it_pos_write - p_dst_ring->it_pos_read;
                    }
                    else
                    {
                        i_ret_value = p_dst_ring->it_buf_size - p_dst_ring->it_pos_read;
                    }
                }

                break;
            }
        case E_LRO_NEXT_WRITE_SIZE:
            {
                if (p_dst_ring->it_pos_read == p_dst_ring->it_pos_write)
                {
                    p_dst_ring->it_pos_read = 0;
                    p_dst_ring->it_pos_write = 0;

                    i_ret_value = p_dst_ring->it_buf_size - 1;
                }
                else
                {
                    if (p_dst_ring->it_pos_read < p_dst_ring->it_pos_write)
                    {
                        i_ret_value = p_dst_ring->it_buf_size - p_dst_ring->it_pos_write;
                        if (p_dst_ring->it_pos_read == 0)
                        {
                            i_ret_value = i_ret_value - 1;
                        }
                    }
                    else
                    {
                        i_ret_value = p_dst_ring->it_pos_read - p_dst_ring->it_pos_write - 1;
                    }
                }

                break;
            }
        default:
            {
                break;
            }
        }
    }

    return i_ret_value;
}


