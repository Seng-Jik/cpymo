#include "cpymo_tool_prelude.h"
#include "cpymo_tool_image.h"
#include "cpymo_tool_asset_filter.h"

typedef struct {
    const char *name, *desc;

    uint16_t imagesize_w, imagesize_h;
    bool use_mask, play_video, screen_fit_support, forced_audio_convert;
    const char
        *bgformat,
        *charaformat,
        *charamaskformat,
        *platform;
    uint8_t audio_support;
} cpymo_tool_convert_spec;

#define OGG 0x01
#define MP3 0x02
#define WAV 0x04

static const cpymo_tool_convert_spec cpymo_tool_convert_specs[] = {
    { "s60v3", "PyMO for Symbian S60v3", 320, 240,
      true, true, false, true,
      "jpg", "jpg", "jpg", "s60v3",
      OGG | MP3 | WAV },

    { "s60v5", "PyMO for Symbian S60v5", 540, 360,
      true, true, false, true,
      "jpg", "jpg", "jpg", "s60v5",
      OGG | MP3 | WAV },

    { "3ds", "CPyMO for Nintendo 3DS", 400, 240,
      false, true, true, false,
      "jpg", "png", NULL, "pygame",
      OGG | MP3 | WAV },

    { "pymo", "PyMO", 800, 600,
      false, true, false, false,
      "jpg", "png", NULL, "pygame",
      OGG | WAV },

    { "psp", "CPyMO for Sony Playstation Portable", 480, 272,
      false, false, false, true,
      "jpg", "png", NULL, "pygame",
      OGG | MP3 | WAV },

    { "wii", "CPyMO for Nintendo Wii", 640, 480,
      false, false, false, true,
      "jpg", "png", NULL, "pygame",
      OGG | WAV },
};

#undef OGG
#undef MP3
#undef WAV

struct cpymo_tool_convert_image_processor_userdata
{
    float scale_ratio;
    bool write_mask;
};

static error_t cpymo_tool_convert_image_processor(
    cpymo_tool_asset_filter_io *io,
    void *userdata)
{
    // load
    cpymo_tool_image image;

    error_t err;
    if (io->input_is_package) {
        err = cpymo_tool_image_load_from_memory(
            &image, io->input.package.data_move_in,
            io->input.package.len, false);
    }
    else {
        char *path;
        err = cpymo_tool_asset_filter_get_input_file_name(&path, io);
        CPYMO_THROW(err);

        err = cpymo_tool_image_load_from_file(
            &image, path, false, NULL);
        free(path);
    }

    CPYMO_THROW(err);

    if (io->input_mask_file_buf_movein) {
        err = cpymo_tool_image_load_attach_mask_from_memory(
            &image, io->input_mask_file_buf_movein, io->input_mask_len);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Failed to attach mask for %s:%s\n",
                io->asset_type,
                cpymo_error_message(err));
        }
    }

    // resize
    const struct cpymo_tool_convert_image_processor_userdata *u =
        (struct cpymo_tool_convert_image_processor_userdata*)userdata;

    cpymo_tool_image resized;
    err = cpymo_tool_image_resize(&resized, &image,
        (size_t)(u->scale_ratio * (float)image.width),
        (size_t)(u->scale_ratio * (float)image.height));
    cpymo_tool_image_free(image);

    // TODO: write

    // clean
    CLEAN:
    if (io->input_is_package) {
        if (io->input.package.data_move_in)
            free(io->input.package.data_move_in);
    }

    if (io->input_mask_file_buf_movein)
        free(io->input_mask_file_buf_movein);

    return CPYMO_ERR_SUCC;
}
