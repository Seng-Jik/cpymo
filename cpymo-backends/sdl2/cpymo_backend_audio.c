#include <SDL.h>
#include <stdbool.h>
#include <cpymo_error.h>
#include <cpymo_backend_audio.h>
#include <cpymo_engine.h>

static bool audio_enabled;
extern cpymo_engine engine;

const static cpymo_backend_audio_info audio_info = {
	48000,
	cpymo_backend_audio_f32,
	2
};

static void cpymo_backend_audio_sdl_callback(void *userdata, Uint8 * stream, int len)
{
	cpymo_audio_copy_samples(stream, (size_t)len, &engine.audio);
}

const cpymo_backend_audio_info *cpymo_backend_audio_get_info(void)
{
	return audio_enabled ? &audio_info : NULL;
}

void cpymo_backend_audio_init()
{
	SDL_AudioSpec want;
	
	SDL_memset(&want, 0, sizeof(want));
	want.callback = &cpymo_backend_audio_sdl_callback;
	want.channels = 2;
	want.format = AUDIO_F32;
	want.freq = 48000;
	want.samples = 2940;
	
	if (SDL_OpenAudio(&want, NULL) == 0) {
		SDL_LockAudio();
		SDL_PauseAudio(0);
		audio_enabled = true;
	}
	else {
		audio_enabled = false;
		const char *err = SDL_GetError();
		SDL_Log("[Error] Audio device open failed: %s.", err);
	}
}

void cpymo_backend_audio_free()
{
	if (audio_enabled) {
		SDL_CloseAudio();
	}
}

void cpymo_backend_audio_lock(void)
{
	SDL_LockAudio();
}

void cpymo_backend_audio_unlock(void)
{
	SDL_UnlockAudio();
}