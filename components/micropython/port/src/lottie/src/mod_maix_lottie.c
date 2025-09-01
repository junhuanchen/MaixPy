/*
 * Maix.lottie 单例子模块
 * 依赖 thorvg_capi.h 及 lcd 驱动
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "thorvg_capi.h"
#include "lcd.h"

/* ---------- 内部单例上下文 ---------- */
typedef struct {
    int w, h;
    uint32_t *argb;
    uint16_t *rgb565;
    Tvg_Canvas *canvas;
    Tvg_Animation *anim;
    Tvg_Paint *pic;
    bool inited;
} lottie_ctx_t;

static lottie_ctx_t gl = {0};

/* ---------- 工具函数 ---------- */
static void _ensure_inited(void) {
    if (gl.inited) return;
    lcd_t *lcd = &lcd_mcu;
    gl.w = lcd->get_width();
    gl.h = lcd->get_height();

    gl.argb   = (uint32_t *)malloc(gl.w * gl.h * 4);
    gl.rgb565 = (uint16_t *)malloc(gl.w * gl.h * 2);
    if (!gl.argb || !gl.rgb565) {
        free(gl.argb); free(gl.rgb565);
        mp_raise_OSError(MP_ENOMEM);
    }
    memset(gl.argb, 0, gl.w * gl.h * 4);
    memset(gl.rgb565, 0, gl.w * gl.h * 2);

    tvg_engine_init(0);
    gl.canvas = tvg_swcanvas_create();
    tvg_swcanvas_set_target(gl.canvas, gl.argb,
                            gl.w, gl.w, gl.h, TVG_COLORSPACE_ARGB8888);
    gl.inited = true;
}

static void _deinit(void) {
    if (!gl.inited) return;
    if (gl.anim)  { tvg_animation_del(gl.anim);  gl.anim  = NULL; }
    if (gl.canvas){ tvg_canvas_destroy(gl.canvas); gl.canvas = NULL; }
    tvg_engine_term();
    free(gl.argb);  gl.argb  = NULL;
    free(gl.rgb565); gl.rgb565 = NULL;
    gl.inited = false;
}

/* ---------- MicroPython 函数 ---------- */

void tvg_log_cb(char* str, uint32_t len)
{
    mp_printf(&mp_plat_print, str);
}


/* 1. init() */
STATIC mp_obj_t maix_lottie_init(void) {

    tvg_set_log_callback((void*)tvg_log_cb);
    _ensure_inited();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_lottie_init_obj, maix_lottie_init);

/* 2. uninit() */
STATIC mp_obj_t maix_lottie_uninit(void) {
    _deinit();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_lottie_uninit_obj, maix_lottie_uninit);

/* 3. load(json_str) */
STATIC mp_obj_t maix_lottie_load(mp_obj_t json_in) {
    if (!gl.inited) mp_raise_OSError(MP_EINVAL);
    size_t len;
    const char *data = mp_obj_str_get_data(json_in, &len);

    if (gl.anim) { tvg_animation_del(gl.anim); gl.anim = NULL; }
    if (gl.pic) { tvg_canvas_remove(gl.canvas, gl.pic); gl.pic = NULL; }

    gl.anim = tvg_animation_new();
    gl.pic = tvg_animation_get_picture(gl.anim);
    tvg_picture_load_data(gl.pic, data, len, "lottie+json", NULL, true);
    tvg_picture_set_size(gl.pic, gl.w, gl.h);
    tvg_canvas_push(gl.canvas, gl.pic);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(maix_lottie_load_obj, maix_lottie_load);

/* 4. total() -> int */
STATIC mp_obj_t maix_lottie_total(void) {
    if (!gl.anim) return mp_obj_new_int(0);
    float total = 0;
    tvg_animation_get_total_frame(gl.anim, &total);
    return mp_obj_new_int((int)total);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_lottie_total_obj, maix_lottie_total);

typedef union {
    uint32_t u32; // argb
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    } __attribute__((packed)) ch;
} rgba_t;

/* 5. view(frame) -> bytes(rgb565) */
STATIC mp_obj_t maix_lottie_view(mp_obj_t frame_in) {
    if (!gl.inited || !gl.anim) mp_raise_OSError(MP_EINVAL);
    int frame = mp_obj_get_int(frame_in);

    // mp_printf(&mp_plat_print, "[MAIXPY]: s tvg_canvas_sync %p %p %d\n", gl.canvas, gl.anim, frame);
    tvg_animation_set_frame(gl.anim, frame);
    tvg_canvas_update(gl.canvas);
    tvg_canvas_draw(gl.canvas, false); // , true
    tvg_canvas_sync(gl.canvas);

    /* ARGB8888 -> RGB565(BE) */
    for (uint32_t i = 0, s = gl.w * gl.h; i != s; ++i) {
        rgba_t *c = (rgba_t *)(gl.argb + i);
        uint16_t rgb = (c->ch.b >> 3) | ((c->ch.g & 0xFC) << 3) | ((c->ch.r & 0xF8) << 8);
        // uint32_t c = gl.argb[i];
        // uint8_t r = (c >> 16) & 0xFF;
        // uint8_t g = (c >> 8)  & 0xFF;
        // uint8_t b =  c        & 0xFF;
        // uint16_t rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        gl.rgb565[i] = (rgb >> 8) | (rgb << 8);  // swap endian
    }
    if (lcd) lcd->draw_picture(0, 0, gl.w, gl.h, (uint8_t *)gl.rgb565);
    // return mp_obj_new_bytes((const byte *)gl.rgb565, gl.w * gl.h * 2);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(maix_lottie_view_obj, maix_lottie_view);

/* ---------- 模块表 ---------- */
STATIC const mp_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lottie) },
    { MP_ROM_QSTR(MP_QSTR_init),   MP_ROM_PTR(&maix_lottie_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_uninit), MP_ROM_PTR(&maix_lottie_uninit_obj) },
    { MP_ROM_QSTR(MP_QSTR_load),   MP_ROM_PTR(&maix_lottie_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_total),  MP_ROM_PTR(&maix_lottie_total_obj) },
    { MP_ROM_QSTR(MP_QSTR_view),   MP_ROM_PTR(&maix_lottie_view_obj) },
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

const mp_obj_type_t Maix_lottie_type = {
    .base = { &mp_type_type },
    .name = MP_QSTR_lottie,
    .locals_dict = (mp_obj_dict_t*)&locals_dict
};

/*
import lcd, time
from Maix import lottie
import KPU
print(os.listdir())
lottie.init()
while True:
    for p in ['/flash/angry.json', '/flash/crying.json', '/flash/scene.json', '/flash/laughing.json']:
        with open(p) as f:
            tmp = f.read()
            lottie.load(tmp)
        total = lottie.total()
        KPU.memtest()
        for i in range(0, 1):
            for f in range(total):
                tmp = time.ticks_ms()
                lottie.view(f)
                #print(time.ticks_ms() - tmp)
                #utime.sleep_ms(1000//fps)

*/
