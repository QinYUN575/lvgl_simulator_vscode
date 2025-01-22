#include "lvgl.h"
#include <stdio.h>
#include "lv_port_file.h"
#include "log_print.h"
#include "lv_port_mem.h"
#include "sysheap.h"

typedef struct{
    uint16_t min;
    uint16_t max;
    uint8_t  bpp;
    uint8_t  reserved[3];
}x_header_t;
typedef struct{
    uint32_t pos;
}x_table_t;
typedef struct{
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t  ofs_x;
    int8_t  ofs_y;
    uint8_t r;
}glyph_dsc_t;


static x_header_t __g_xbf_hd = {
    .min = 0x0009,
    .max = 0xffe5,
    .bpp = 4,
};
char *Font_buff = NULL;

static void * init_font(void)
{
	FILE *ff = fopen("/SD:/font/lv_font_chinese_18.bin", "r");
	if (ff == NULL)
	{
		CLOGE("Failed to open file for reading");
		return NULL;
	}
	fseek(ff, 0, SEEK_END);
	long lSize = ftell(ff);
	rewind(ff);
	CLOGD("Lsize %ld", lSize);
	static uint8_t first_in = 1;
	if (first_in == 1)
	{
		first_in = 0;
		Font_buff = (char *)exram_malloc(32, (sizeof(char) * lSize));
	}
	int br = fread(Font_buff, 1, lSize, ff);
	CLOGD("Bytes read %d", br);
	fclose(ff);
	return Font_buff;
}

void flash_copy_font_res(void)
{
    CLOGD("flash_copy_font_res");
    Font_buff = (char *)exram_malloc(32, 1858949);
    if (!Font_buff) {
        CLOGE("exram_malloc Failed");
        return;
    }
    memcpy(Font_buff, (void *)0x30C00000, 1858949);
    CLOGD("flash_copy_font_res Font_buff:%p", Font_buff);
}

static uint8_t *__user_font_getdata(int offset, int size){
    // uint32_t font_address = CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_RES_LV_FONT_CHINESE_ADDRESS + offset;
    
    if (!Font_buff) {
        init_font();
        // flash_copy_font_res();
    }
    return (uint8_t*)Font_buff + offset;

    // uint8_t *pFont = (uint8_t *)0x30C00000;
    // return pFont + offset;
}


static const uint8_t * __user_font_get_bitmap(const lv_font_t * font, uint32_t unicode_letter) {
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {
        return NULL;
    }
    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(unicode_offset, 4);
    if( p_pos[0] != 0 ) {
        uint32_t pos = p_pos[0];
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(pos, sizeof(glyph_dsc_t));
        return __user_font_getdata(pos+sizeof(glyph_dsc_t), gdsc->box_w*gdsc->box_h*__g_xbf_hd.bpp/8);
    }
    return NULL;
}


static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {
        return NULL;
    }
    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(unicode_offset, 4);
    if( p_pos[0] != 0 ) {
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(p_pos[0], sizeof(glyph_dsc_t));
        dsc_out->adv_w = gdsc->adv_w;
        dsc_out->box_h = gdsc->box_h;
        dsc_out->box_w = gdsc->box_w;
        dsc_out->ofs_x = gdsc->ofs_x;
        dsc_out->ofs_y = gdsc->ofs_y;
        dsc_out->bpp   = __g_xbf_hd.bpp;
        return true;
    }
    return false;
}


lv_font_t lv_font_chinese_18 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 32,
    .base_line = 0,
};
