#pragma once

#include <stdint.h>


typedef struct go2_display go2_display_t;
typedef struct go2_surface go2_surface_t;
typedef struct go2_frame_buffer go2_frame_buffer_t;
typedef struct go2_presenter go2_presenter_t;

typedef enum go2_rotation
{
    GO2_ROTATION_DEGREES_0 = 0,
    GO2_ROTATION_DEGREES_90,
    GO2_ROTATION_DEGREES_180,
    GO2_ROTATION_DEGREES_270
} go2_rotation_t;

typedef struct go2_context_attributes
{
    int major;
    int minor;
    int red_bits;
    int green_bits;
    int blue_bits;
    int alpha_bits;
    int depth_bits;
    int stencil_bits;
} go2_context_attributes_t;

typedef struct go2_context go2_context_t;


#ifdef __cplusplus
extern "C" {
#endif

go2_display_t* go2_display_create();
void go2_display_destroy(go2_display_t* display);
int go2_display_width_get(go2_display_t* display);
int go2_display_height_get(go2_display_t* display);
void go2_display_present(go2_display_t* display, go2_frame_buffer_t* frame_buffer);


int go2_drm_format_get_bpp(uint32_t format);


go2_surface_t* go2_surface_create(go2_display_t* display, int width, int height, uint32_t format);
void go2_surface_destroy(go2_surface_t* surface);
int go2_surface_width_get(go2_surface_t* surface);
int go2_surface_height_get(go2_surface_t* surface);
uint32_t go2_surface_format_get(go2_surface_t* surface);
int go2_surface_stride_get(go2_surface_t* surface);
int go2_surface_prime_fd(go2_surface_t* surface);
void* go2_surface_map(go2_surface_t* surface);
void go2_surface_unmap(go2_surface_t* surface);
void go2_surface_blit(go2_surface_t* srcSurface, int srcX, int srcY, int srcWidth, int srcHeight,
                      go2_surface_t* dstSurface, int dstX, int dstY, int dstWidth, int dstHeight,
                      go2_rotation_t rotation);
                      

go2_frame_buffer_t* go2_frame_buffer_create(go2_surface_t* surface);
void go2_frame_buffer_destroy(go2_frame_buffer_t* frame_buffer);
go2_surface_t* go2_frame_buffer_surface_get(go2_frame_buffer_t* frame_buffer);


go2_presenter_t* go2_presenter_create(go2_display_t* display, uint32_t format, uint32_t background_color);
void go2_presenter_destroy(go2_presenter_t* presenter);
void go2_presenter_post(go2_presenter_t* presenter, go2_surface_t* surface, int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight, go2_rotation_t rotation);


go2_context_t* go2_context_create(go2_display_t* display, int width, int height, const go2_context_attributes_t* attributes);
void go2_context_destroy(go2_context_t* context);
void go2_context_make_current(go2_context_t* context);
void go2_context_swap_buffers(go2_context_t* context);
go2_surface_t* go2_context_surface_lock(go2_context_t* context);
void go2_context_surface_unlock(go2_context_t* context, go2_surface_t* surface);

#ifdef __cplusplus
}
#endif
