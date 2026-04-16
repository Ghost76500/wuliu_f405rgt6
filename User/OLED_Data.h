#ifndef __OLED_DATA_H
#define __OLED_DATA_H

#include <stdint.h>

/*中文字符字节宽度*/
#define OLED_CHN_CHAR_WIDTH			3		//UTF-8编码格式给3，GB2312编码格式给2

/*字模基本单元*/
typedef struct 
{
	char Index[OLED_CHN_CHAR_WIDTH + 1];	//汉字索引
	uint8_t Data[32];						//字模数据
} ChineseCell_t;

/*ASCII字模数据声明*/
extern const uint8_t OLED_F8x16[][16];
extern const uint8_t OLED_F6x8[][6];
/* 支持可选的大字号字模（宽16，高64），每个字符占用128字节：Width * (Height/8) = 16 * 8 = 128 */
extern const uint8_t OLED_F16x64[][128];

/*汉字字模数据声明*/
extern const ChineseCell_t OLED_CF16x16[];

/*图像数据声明*/
extern const uint8_t Diode[];
/*按照上面的格式，在这个位置加入新的图像数据声明*/
//...
extern const uint8_t Battery3[];
extern const uint8_t windows[];
extern const uint8_t zero[];
extern const uint8_t one[];
extern const uint8_t two[];
extern const uint8_t three[];
extern const uint8_t up[];

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
