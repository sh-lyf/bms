#ifndef __BMS_CHECKSUM_H__
#define __BMS_CHECKSUM_H__

uint16_t GetCRCCode(const uint8_t *pBuf, uint16_t iLen);
uint16_t GetCcittCode(const uint8_t *pBuf, uint32_t iLen);

#endif/*__BMS_CHECKSUM_H__*/
