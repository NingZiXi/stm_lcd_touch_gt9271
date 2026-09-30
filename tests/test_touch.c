#include "stm_lcd_touch_gt9271.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
static uint8_t status, point[8], acknowledged;
static int read_fail, write_fail, invalid_id;
static int read_reg(void *io, uint16_t reg, uint8_t *buf, size_t len) {
    (void)io;
    if (read_fail) return -1;
    if (reg == 0x8140 && len == 4) { memcpy(buf, invalid_id ? "BAD!" : "9271", 4); return 0; }
    if (reg == 0x814e && len == 1) { *buf = status; return 0; }
    if (reg == 0x814f && len == 8) { memcpy(buf, point, len); return 0; }
    return -1;
}
static int write_reg(void *io, uint16_t reg, const uint8_t *buf, size_t len) {
    (void)io;
    if (write_fail) return -1;
    assert(reg == 0x814e && len == 1 && buf[0] == 0);
    ++acknowledged; return 0;
}
int main(void) {
    stm_lcd_touch_gt9271_t touch = {0};
    stm_lcd_touch_gt9271_point_t result = {0};
    stm_lcd_touch_gt9271_config_t cfg = { .read_reg=read_reg, .write_reg=write_reg, .x_max=800, .y_max=1280 };
    size_t count = 999;
    char id[5];
    assert(stm_lcd_touch_gt9271_read_data(&touch) == -1);
    assert(stm_lcd_touch_gt9271_new_i2c(&touch, &cfg) == 0);
    assert(stm_lcd_touch_gt9271_read_id(&touch, id) == 0 && !strcmp(id,"9271"));
    invalid_id=1; assert(stm_lcd_touch_gt9271_read_id(&touch,id) == -3); invalid_id=0;
    status=0x81; point[0]=2; point[1]=53; point[2]=0; point[3]=247; point[4]=0;
    assert(stm_lcd_touch_gt9271_read_data(&touch) == 0);
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==1);
    assert(result.x==53 && result.y==247 && result.id==2 && acknowledged==1);
    /* Polling faster than the controller must not introduce false releases. */
    status=0;
    for (unsigned i=0; i<100; ++i) {
        assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
        assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==1);
        assert(result.x==53 && result.y==247 && acknowledged==1);
    }
    status=0x81; point[1]=90;
    assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==1 && result.x==90);
    status=0x80;
    assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==0);
    assert(acknowledged==3);
    status=0;
    assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==0);
    assert(acknowledged==3);
    status=0x81; point[1]=53;
    assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
    read_fail=1; assert(stm_lcd_touch_gt9271_read_data(&touch)==-2); read_fail=0;
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==0);
    cfg.mirror_x=1; cfg.mirror_y=1; cfg.swap_xy=0;
    assert(stm_lcd_touch_gt9271_new_i2c(&touch,&cfg)==0);
    assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==1);
    assert(result.x==746 && result.y==1032);
    cfg.swap_xy=1; cfg.mirror_x=0; cfg.mirror_y=0; cfg.x_max=1280; cfg.y_max=800;
    assert(stm_lcd_touch_gt9271_new_i2c(&touch,&cfg)==0);
    assert(stm_lcd_touch_gt9271_read_data(&touch)==0);
    assert(stm_lcd_touch_gt9271_get_data(&touch,&result,1,&count)==0 && count==1);
    assert(result.x==247 && result.y==53);
    status=0x8f; assert(stm_lcd_touch_gt9271_read_data(&touch)==-3);
    write_fail=1; assert(stm_lcd_touch_gt9271_read_data(&touch)==-2);
    return 0;
}
