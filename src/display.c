#include "display.h"

#include "queue.h"

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>
#include <gbm.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <rga/RgaApi.h>



typedef struct go2_display
{
    int fd;
    uint32_t connector_id;
    drmModeModeInfo mode;
    uint32_t width;
    uint32_t height;
    uint32_t crtc_id;
} go2_display_t;

typedef struct go2_surface
{
    go2_display_t* display;
    uint32_t gem_handle;
    uint64_t size;
    int width;
    int height;
    int stride;
    uint32_t format;
    int prime_fd;
    bool is_mapped;
    uint8_t* map;
} go2_surface_t;

typedef struct go2_frame_buffer
{
    go2_surface_t* surface;
    uint32_t fb_id;
} go2_frame_buffer_t;

typedef struct go2_presenter
{
    go2_display_t* display;
    uint32_t format;
    uint32_t background_color;
    go2_queue_t* freeFrameBuffers;
    go2_queue_t* usedFrameBuffers;
    pthread_mutex_t queueMutex;
    pthread_t renderThread;
    sem_t freeSem;
    sem_t usedSem;
    volatile bool terminating;
} go2_presenter_t;


go2_display_t* go2_display_create()
{
    int i;


    go2_display_t* result = malloc(sizeof(*result));
    if (!result)
    {
        printf("malloc failed.\n");
        goto out;
    }

    memset(result, 0, sizeof(*result));


    // Open device
    result->fd = open("/dev/dri/card0", O_RDWR);
    if (result->fd < 0)
    {
        printf("open /dev/dri/card0 failed.\n");
        goto err_00;
    }


    drmModeRes* resources = drmModeGetResources(result->fd);
    if (!resources) 
    {
        printf("drmModeGetResources failed: %s\n", strerror(errno));
        goto err_01;
    }


    // Find connector
    drmModeConnector* connector;
    for (i = 0; i < resources->count_connectors; i++)
    {
        connector = drmModeGetConnector(result->fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            break;
        }

        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector)
    {
        printf("DRM_MODE_CONNECTED not found.\n");
        goto err_02;
    }

    result->connector_id = connector->connector_id;


    // Find prefered mode
    drmModeModeInfo* mode;
    for (i = 0; i < connector->count_modes; i++)
    {
        drmModeModeInfo *current_mode = &connector->modes[i];
        if (current_mode->type & DRM_MODE_TYPE_PREFERRED)
        {
            mode = current_mode;
            break;
        }

        mode = NULL;
    }

    if (!mode) 
    {
        printf("DRM_MODE_TYPE_PREFERRED not found.\n");
        goto err_03;
    }

    result->mode = *mode;
    result->width = mode->hdisplay;
    result->height = mode->vdisplay;


    // Find encoder
    drmModeEncoder* encoder;
    for (i = 0; i < resources->count_encoders; i++)
    {
        encoder = drmModeGetEncoder(result->fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
        {
            break;
        }
        
        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }

    if (!encoder)
    {

        printf("could not find encoder!\n");
        goto err_03;
    }
    
    result->crtc_id = encoder->crtc_id;

    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    
    return result;


err_03:
    drmModeFreeConnector(connector);

err_02:
    drmModeFreeResources(resources);

err_01:
    close(result->fd);

err_00:
    free(result);

out:
    return NULL;
}


void go2_display_destroy(go2_display_t* display)
{
    close(display->fd);
    free(display);
}

int go2_display_width_get(go2_display_t* display)
{
    return display->width;
}

int go2_display_height_get(go2_display_t* display)
{
    return display->height;
}

void go2_display_present(go2_display_t* display, go2_frame_buffer_t* frame_buffer)
{
    int ret = drmModeSetCrtc(display->fd, display->crtc_id, frame_buffer->fb_id, 0, 0, &display->connector_id, 1, &display->mode);
    if (ret)
    {
        printf("drmModeSetCrtc failed.\n");        
    }
}


int go2_drm_format_get_bpp(uint32_t format)
{
    int result;

    switch(format)
    {
        case DRM_FORMAT_XRGB4444:
        case DRM_FORMAT_XBGR4444:
        case DRM_FORMAT_RGBX4444:
        case DRM_FORMAT_BGRX4444:

        case DRM_FORMAT_ARGB4444:
        case DRM_FORMAT_ABGR4444:
        case DRM_FORMAT_RGBA4444:
        case DRM_FORMAT_BGRA4444:

        case DRM_FORMAT_XRGB1555:
        case DRM_FORMAT_XBGR1555:
        case DRM_FORMAT_RGBX5551:
        case DRM_FORMAT_BGRX5551:

        case DRM_FORMAT_ARGB1555:
        case DRM_FORMAT_ABGR1555:
        case DRM_FORMAT_RGBA5551:
        case DRM_FORMAT_BGRA5551:

        case DRM_FORMAT_RGB565:
        case DRM_FORMAT_BGR565:
            result = 16;
            break;
            

        case DRM_FORMAT_RGB888:
        case DRM_FORMAT_BGR888:
            result = 24;
            break;


        case DRM_FORMAT_XRGB8888:
        case DRM_FORMAT_XBGR8888:
        case DRM_FORMAT_RGBX8888:
        case DRM_FORMAT_BGRX8888:

        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_BGRA8888:

        case DRM_FORMAT_XRGB2101010:
        case DRM_FORMAT_XBGR2101010:
        case DRM_FORMAT_RGBX1010102:
        case DRM_FORMAT_BGRX1010102:

        case DRM_FORMAT_ARGB2101010:
        case DRM_FORMAT_ABGR2101010:
        case DRM_FORMAT_RGBA1010102:
        case DRM_FORMAT_BGRA1010102:
            result = 32;
            break;


        default:
            printf("unhandled DRM FORMAT.\n");
            result = 0;
    }

    return result;
}

go2_surface_t* go2_surface_create(go2_display_t* display, int width, int height, uint32_t format)
{
    go2_surface_t* result = malloc(sizeof(*result));
    if (!result)
    {
        printf("malloc failed.\n");
        goto out;
    }

    memset(result, 0, sizeof(*result));


    struct drm_mode_create_dumb args = {0};
    args.width = width;
    args.height = height;
    args.bpp = go2_drm_format_get_bpp(format);
    args.flags = 0;

    int io = drmIoctl(display->fd, DRM_IOCTL_MODE_CREATE_DUMB, &args);
    if (io < 0)
    {
        printf("DRM_IOCTL_MODE_CREATE_DUMB failed.\n");
        goto out;
    }


    result->display = display;
    result->gem_handle = args.handle;
    result->size = args.size;
    result->width = width;
    result->height = height;
    result->stride = args.pitch;
    result->format = format;

    return result;

out:
    free(result);
    return NULL;
}

void go2_surface_destroy(go2_surface_t* surface)
{
    struct drm_mode_destroy_dumb args = { 0 };
    args.handle = surface->gem_handle;

    int io = drmIoctl(surface->display->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &args);
    if (io < 0)
    {
        printf("DRM_IOCTL_MODE_DESTROY_DUMB failed.\n");        
    }

    free(surface);
}

int go2_surface_width_get(go2_surface_t* surface)
{
    return surface->width;
}

int go2_surface_height_get(go2_surface_t* surface)
{
    return surface->height;
}

uint32_t go2_surface_format_get(go2_surface_t* surface)
{
    return surface->format;
}

int go2_surface_stride_get(go2_surface_t* surface)
{
    return surface->stride;
}

int go2_surface_prime_fd(go2_surface_t* surface)
{
    if (surface->prime_fd <= 0)
    {
        int io = drmPrimeHandleToFD(surface->display->fd, surface->gem_handle, DRM_RDWR | DRM_CLOEXEC, &surface->prime_fd);
        if (io < 0)
        {
            printf("drmPrimeHandleToFD failed.\n");
            goto out;
        }
    }

    return surface->prime_fd;

out:
    surface->prime_fd = 0;
    return 0;
}

void* go2_surface_map(go2_surface_t* surface)
{
    if (surface->is_mapped)
        return surface->map;


    int prime_fd = go2_surface_prime_fd(surface);
    surface->map = mmap(NULL, surface->size, PROT_READ | PROT_WRITE, MAP_SHARED, prime_fd, 0);
    if (surface->map == MAP_FAILED)
    {
        printf("mmap failed.\n");
        return NULL;
    }

    surface->is_mapped = true;
    return surface->map;
}

void go2_surface_unmap(go2_surface_t* surface)
{
    if (surface->is_mapped)
    {
        munmap(surface->map, surface->size);

        surface->is_mapped = false;
        surface->map = NULL;
    }
}


static uint32_t go2_rkformat_get(uint32_t drm_fourcc)
{
    switch (drm_fourcc)
    {
        case DRM_FORMAT_RGBA8888:
            return RK_FORMAT_RGBA_8888;
    
        case DRM_FORMAT_RGBX8888:
            return RK_FORMAT_RGBX_8888;
    
        case DRM_FORMAT_RGB888:
            return RK_FORMAT_RGB_888;
    
        case DRM_FORMAT_BGRA8888:
            return RK_FORMAT_BGRA_8888;
    
        case DRM_FORMAT_RGB565:
            return RK_FORMAT_RGB_565;

        case DRM_FORMAT_RGBA5551:
            return RK_FORMAT_RGBA_5551;
    
        case DRM_FORMAT_RGBA4444:
            return RK_FORMAT_RGBA_4444;

        case DRM_FORMAT_BGR888:
            return RK_FORMAT_BGR_888;
    
        default:
            printf("RKFORMAT not supported.\n");
            return 0;
    }
}

void go2_surface_blit(go2_surface_t* srcSurface, int srcX, int srcY, int srcWidth, int srcHeight,
                      go2_surface_t* dstSurface, int dstX, int dstY, int dstWidth, int dstHeight,
                      go2_rotation_t rotation)
{
    rga_info_t dst = { 0 };
    dst.fd = go2_surface_prime_fd(dstSurface);
    dst.mmuFlag = 1;
    dst.rect.xoffset = dstX;
    dst.rect.yoffset = dstY;
    dst.rect.width = dstWidth;
    dst.rect.height = dstHeight;
    dst.rect.wstride = dstSurface->stride / (go2_drm_format_get_bpp(dstSurface->format) / 8);
    dst.rect.hstride = dstSurface->height;
    dst.rect.format = go2_rkformat_get(dstSurface->format);

    rga_info_t src = { 0 };
    src.fd = go2_surface_prime_fd(srcSurface);
    src.mmuFlag = 1;

    switch (rotation)
    {
        case GO2_ROTATION_DEGREES_0:
            src.rotation = 0;
            break;

        case GO2_ROTATION_DEGREES_90:
            src.rotation = HAL_TRANSFORM_ROT_90;
            break;

        case GO2_ROTATION_DEGREES_180:
            src.rotation = HAL_TRANSFORM_ROT_180;
            break;

        case GO2_ROTATION_DEGREES_270:
            src.rotation = HAL_TRANSFORM_ROT_270;
            break;

        default:
            printf("rotation not supported.\n");
            return;
    }

    src.rect.xoffset = srcX;
    src.rect.yoffset = srcY;
    src.rect.width = srcWidth;
    src.rect.height = srcHeight;
    src.rect.wstride = srcSurface->stride / (go2_drm_format_get_bpp(srcSurface->format) / 8);
    src.rect.hstride = srcSurface->height;
    src.rect.format = go2_rkformat_get(srcSurface->format);

#if 0
    enum
    {
        CATROM    = 0x0,
        MITCHELL  = 0x1,
        HERMITE   = 0x2,
        B_SPLINE  = 0x3,
    };  /*bicubic coefficient*/
#endif
    src.scale_mode = 2;


    int ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
    {
        printf("c_RkRgaBlit failed.\n");        
    }
}


go2_frame_buffer_t* go2_frame_buffer_create(go2_surface_t* surface)
{
    go2_frame_buffer_t* result = malloc(sizeof(*result));
    if (!result)
    {
        printf("malloc failed.\n");
        return NULL;
    }

    memset(result, 0, sizeof(*result));


    result->surface = surface;

    const uint32_t handles[4] = {surface->gem_handle, 0, 0, 0};
    const uint32_t pitches[4] = {surface->stride, 0, 0, 0};
    const uint32_t offsets[4] = {0, 0, 0, 0};

    int ret = drmModeAddFB2(surface->display->fd,
        surface->width,
        surface->height,
        surface->format,
        handles,
        pitches,
        offsets,
        &result->fb_id,
        0);
    if (ret)
    {
        printf("drmModeAddFB2 failed.\n");
        free(result);
        return NULL;
    }

    return result;
}

void go2_frame_buffer_destroy(go2_frame_buffer_t* frame_buffer)
{
    int ret = drmModeRmFB(frame_buffer->surface->display->fd, frame_buffer->fb_id);
    if (ret)
    {
        printf("drmModeRmFB failed.\n");
    }

    free(frame_buffer);
}

go2_surface_t* go2_frame_buffer_surface_get(go2_frame_buffer_t* frame_buffer)
{
    return frame_buffer->surface;
}





#if 0
typedef struct go2_presenter
{
    go2_display_t* display;
    uint32_t format;
    uint32_t background_color;
    go2_queue_t* freeFrameBuffers;
    go2_queue_t* usedFrameBuffers;
    pthread_mutex_t queueMutex;
    pthread_t renderThread;
    sem_t freeSem;
    sem_t usedSem;
    volatile bool terminating;
} go2_presenter_t;
#endif

#define BUFFER_COUNT (3)

static void* go2_presenter_renderloop(void* arg)
{
    go2_presenter_t* presenter = (go2_presenter_t*)arg;
    go2_frame_buffer_t* prevFrameBuffer = NULL;

    presenter->terminating = false;
    while(!presenter->terminating)
    {
        sem_wait(&presenter->usedSem);
        if(presenter->terminating) break;


        pthread_mutex_lock(&presenter->queueMutex);

        if (go2_queue_count_get(presenter->usedFrameBuffers) < 1)
        {
            printf("no framebuffer available.\n");
            abort();
        }

        go2_frame_buffer_t* dstFrameBuffer = (go2_frame_buffer_t*)go2_queue_pop(presenter->usedFrameBuffers);

        pthread_mutex_unlock(&presenter->queueMutex);


        go2_display_present(presenter->display, dstFrameBuffer);

        if (prevFrameBuffer)
        {
            pthread_mutex_lock(&presenter->queueMutex);
            go2_queue_push(presenter->freeFrameBuffers, prevFrameBuffer);
            pthread_mutex_unlock(&presenter->queueMutex);

            sem_post(&presenter->freeSem);
        }

        prevFrameBuffer = dstFrameBuffer;            
    }


    return NULL;
}

go2_presenter_t* go2_presenter_create(go2_display_t* display, uint32_t format, uint32_t background_color)
{
    go2_presenter_t* result = malloc(sizeof(*result));
    if (!result)
    {
        printf("malloc failed.\n");
        return NULL;
    }

    memset(result, 0, sizeof(*result));


    result->display = display;
    result->format = format;
    result->background_color = background_color;
    result->freeFrameBuffers = go2_queue_create(BUFFER_COUNT);
    result->usedFrameBuffers = go2_queue_create(BUFFER_COUNT);

    int width = go2_display_width_get(display);
    int height = go2_display_height_get(display);

    for (int i = 0; i < BUFFER_COUNT; ++i)
    {
        go2_surface_t* surface = go2_surface_create(display, width, height, format);
        go2_frame_buffer_t* frameBuffer = go2_frame_buffer_create(surface);

        go2_queue_push(result->freeFrameBuffers, frameBuffer);
    }

 
    sem_init(&result->usedSem, 0, 0);
    sem_init(&result->freeSem, 0, BUFFER_COUNT);

    pthread_mutex_init(&result->queueMutex, NULL);

    pthread_create(&result->renderThread, NULL, go2_presenter_renderloop, result);

    return result;
}

void go2_presenter_destroy(go2_presenter_t* presenter)
{
    // TODO
    abort();
}

void go2_presenter_post(go2_presenter_t* presenter, go2_surface_t* surface, int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight, go2_rotation_t rotation)
{
    sem_wait(&presenter->freeSem);


    pthread_mutex_lock(&presenter->queueMutex);

    if (go2_queue_count_get(presenter->freeFrameBuffers) < 1)
    {
        printf("no framebuffer available.\n");
        abort();
    }

    go2_frame_buffer_t* dstFrameBuffer = go2_queue_pop(presenter->freeFrameBuffers);

    pthread_mutex_unlock(&presenter->queueMutex);


    go2_surface_t* dstSurface = go2_frame_buffer_surface_get(dstFrameBuffer);

    rga_info_t dst = { 0 };
    dst.fd = go2_surface_prime_fd(dstSurface);
    dst.mmuFlag = 1;
    dst.rect.xoffset = 0;
    dst.rect.yoffset = 0;
    dst.rect.width = go2_surface_width_get(dstSurface);
    dst.rect.height = go2_surface_height_get(dstSurface);
    dst.rect.wstride = go2_surface_stride_get(dstSurface) / (go2_drm_format_get_bpp(go2_surface_format_get(dstSurface)) / 8);
    dst.rect.hstride = go2_surface_height_get(dstSurface);
    dst.rect.format = go2_rkformat_get(go2_surface_format_get(dstSurface));
    dst.color = presenter->background_color;

    int ret = c_RkRgaColorFill(&dst);
    if (ret)
    {
        printf("c_RkRgaColorFill failed.\n");
    }


    go2_surface_blit(surface, srcX, srcY, srcWidth, srcHeight, dstSurface, dstX, dstY, dstWidth, dstHeight, rotation);


    pthread_mutex_lock(&presenter->queueMutex);
    go2_queue_push(presenter->usedFrameBuffers, dstFrameBuffer);
    pthread_mutex_unlock(&presenter->queueMutex);

    sem_post(&presenter->usedSem);
}


