#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// struct ImageHandle;

// typedef struct ImageHandle* SF_HIMAGE;

// SF_HIMAGE sf_open_img();

// void sf_close_img(SF_HIMAGE img);

// size_t sf_img_read(SF_HIMAGE img, void* buf, size_t toRead);

int sf_run_fsrip(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
