#include "stm_lcd_touch_gt9271.h"
#include "test_allocator.h"
#include <assert.h>
#include <string.h>
typedef struct {
    uint8_t status, raw[80]; unsigned reads, acks, resets;
    stm_err_t read_error, point_error, ack_error, reset_error; int wrong_id;
} mock_t;
static stm_err_t read_reg(void *io, uint16_t reg,uint8_t *data,size_t len) {
    mock_t *m=io; ++m->reads;
    if (m->read_error) return m->read_error;
    if (reg==0x8140) { assert(len==4); memcpy(data,m->wrong_id ? "BAD!" : "9271",4); return STM_OK; }
    if (reg==0x814e) { assert(len==1); *data=m->status; return STM_OK; }
    assert(reg==0x814f && len<=sizeof m->raw);
    if (m->point_error) return m->point_error;
    memcpy(data,m->raw,len); return STM_OK;
}
static stm_err_t write_reg(void *io,uint16_t reg,const uint8_t *data,size_t len) {
    mock_t *m=io; assert(reg==0x814e && len==1 && *data==0); ++m->acks; return m->ack_error;
}
static void delay(void *io,uint32_t ms) { (void)io; assert(ms==30 || ms==50); }
static stm_err_t reset(void *io,int high) { mock_t *m=io; ++m->resets; assert(high==0 || high==1); return m->reset_error; }
static void set_points(mock_t *m) {
    memset(m->raw,0,sizeof m->raw); m->status=0x80 | 2;
    m->raw[0]=3; m->raw[1]=12; m->raw[3]=20; m->raw[8]=4; m->raw[9]=30; m->raw[11]=40;
}
int main(void) {
    mock_t m={0},m2={0}; lcd_touch_gt9271_handle_t touch=NULL,other=NULL;
    lcd_touch_gt9271_config_t cfg={.read_reg=read_reg,.delay_ms=delay,.reset=reset,.io=&m,.x_max=240,.y_max=320,.write_reg=write_reg},bad;
    lcd_touch_gt9271_point_t points[2]={{0}}; size_t count=999;
    assert(lcd_touch_gt9271_create(NULL,&touch)==STM_ERR_INVALID_ARG);
    assert(lcd_touch_gt9271_create(&cfg,NULL)==STM_ERR_INVALID_ARG);
    assert(lcd_touch_gt9271_delete(NULL)==STM_ERR_INVALID_ARG);
    assert(lcd_touch_gt9271_delete(&touch)==STM_OK);
    bad=cfg; bad.read_reg=NULL; assert(lcd_touch_gt9271_create(&bad,&touch)==STM_ERR_INVALID_CONFIG);
    bad=cfg; bad.x_max=0; assert(lcd_touch_gt9271_create(&bad,&touch)==STM_ERR_INVALID_CONFIG);
    bad=cfg; bad.mirror_x=2; assert(lcd_touch_gt9271_create(&bad,&touch)==STM_ERR_INVALID_CONFIG);
    bad=cfg; bad.delay_ms=NULL; assert(lcd_touch_gt9271_create(&bad,&touch)==STM_ERR_INVALID_CONFIG);
    test_alloc_fail=1; assert(lcd_touch_gt9271_create(&cfg,&touch)==STM_ERR_NO_MEM && !touch && !test_alloc_live);
    test_alloc_fail=0; assert(lcd_touch_gt9271_create(&cfg,&touch)==STM_OK && m.reads==0 && m.resets==0);
    lcd_touch_gt9271_handle_t saved=touch;
    assert(lcd_touch_gt9271_create(&cfg,&touch)==STM_ERR_INVALID_STATE && touch==saved);
    cfg.io=&m2; assert(lcd_touch_gt9271_create(&cfg,&other)==STM_OK && test_alloc_live==2);
    assert(lcd_touch_gt9271_get_data(NULL,points,2,&count)==STM_ERR_INVALID_ARG && count==0);
    assert(lcd_touch_gt9271_get_data(touch,NULL,1,&count)==STM_ERR_INVALID_ARG && count==0);
    assert(lcd_touch_gt9271_get_data(touch,NULL,0,&count)==STM_OK && count==0);
    assert(lcd_touch_gt9271_get_data(touch,points,2,NULL)==STM_ERR_INVALID_ARG);
    char id[5]; assert(lcd_touch_gt9271_read_id(touch,id)==STM_OK && !strcmp(id,"9271"));
    m.wrong_id=1; assert(lcd_touch_gt9271_read_id(touch,id)==STM_ERR_NOT_SUPPORTED && !strcmp(id,"BAD!")); m.wrong_id=0;
    m.read_error=STM_ERR_TIMEOUT; assert(lcd_touch_gt9271_read_id(touch,id)==STM_ERR_TIMEOUT && !id[0]); m.read_error=STM_OK;
    set_points(&m);
    assert(lcd_touch_gt9271_read_data(touch)==STM_OK);
    assert(lcd_touch_gt9271_get_data(touch,points,1,&count)==STM_OK && count==1 && points[0].x==12 && points[0].y==20);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==2 && points[1].x==30);
    assert(lcd_touch_gt9271_get_data(other,points,2,&count)==STM_OK && count==0);
    m.status=0; unsigned acks=m.acks;
    for (unsigned i=0;i<100;++i) { assert(lcd_touch_gt9271_read_data(touch)==STM_OK); assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==2); }
    assert(m.acks==acks);
    m.status=0x80; assert(lcd_touch_gt9271_read_data(touch)==STM_OK);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==0);
    set_points(&m); assert(lcd_touch_gt9271_read_data(touch)==STM_OK);
    m.read_error=STM_ERR_TIMEOUT; assert(lcd_touch_gt9271_read_data(touch)==STM_ERR_TIMEOUT);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==0); m.read_error=STM_OK;
    m.point_error=STM_ERR_CANCELLED; assert(lcd_touch_gt9271_read_data(touch)==STM_ERR_CANCELLED); m.point_error=STM_OK;
    m.status=0x80 | 15; assert(lcd_touch_gt9271_read_data(touch)==STM_ERR_VERIFY);
    m.ack_error=STM_ERR_IO; assert(lcd_touch_gt9271_read_data(touch)==STM_ERR_VERIFY);
    set_points(&m); m.point_error=STM_ERR_TIMEOUT;
    unsigned ack_before=m.acks; assert(lcd_touch_gt9271_read_data(touch)==STM_ERR_TIMEOUT && m.acks==ack_before+1);
    m.point_error=STM_OK; assert(lcd_touch_gt9271_read_data(touch)==STM_ERR_IO);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==0); m.ack_error=STM_OK;
    m.reset_error=STM_ERR_IO; assert(lcd_touch_gt9271_reset(touch)==STM_ERR_IO);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==0);
    m.reset_error=STM_OK; assert(lcd_touch_gt9271_reset(touch)==STM_OK);
    assert(lcd_touch_gt9271_delete(&touch)==STM_OK && !touch && test_alloc_live==1);
    cfg.io=&m; cfg.swap_xy=1; cfg.mirror_x=1; cfg.mirror_y=1;
    cfg.x_max=320; cfg.y_max=240;
    assert(lcd_touch_gt9271_create(&cfg,&touch)==STM_OK);
    set_points(&m); assert(lcd_touch_gt9271_read_data(touch)==STM_OK);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==2 && points[0].x==299 && points[0].y==227);
    m.raw[2]=0x7f; /* 越界点被过滤。 */
    assert(lcd_touch_gt9271_read_data(touch)==STM_OK);
    assert(lcd_touch_gt9271_get_data(touch,points,2,&count)==STM_OK && count==1);
    assert(lcd_touch_gt9271_delete(&touch)==STM_OK && lcd_touch_gt9271_delete(&other)==STM_OK && !test_alloc_live);
    assert(lcd_touch_gt9271_delete(&touch)==STM_OK);
    return 0;
}
