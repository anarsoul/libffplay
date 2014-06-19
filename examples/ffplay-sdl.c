/*
 * Copyright (c) 2014 Vasily Khoruzhick <anarsoul@gmail.com>
 *
 * This file is part of libffplay.
 *
 * libffplay is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libffplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libffplay; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define SDL_AUDIO_BUFFER_SIZE 1024
#define FF_ALLOC_EVENT    (SDL_USEREVENT)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

#include <errno.h>
#include <math.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_thread.h>

#include "libffplay.h"

struct myeventmgr {
	libffplay_eventmgr_t eventmgr;
	libffplay_ctx_t *ctx;
};

struct myvideomgr {
	libffplay_videomgr_t videomgr;
	libffplay_ctx_t *ctx;
	SDL_Surface *screen;
	SDL_cond *alloc_cond;
	SDL_mutex *alloc_mutex;
	size_t old_w, old_h;
	SDL_Rect rect;
};

static void (*ffplay_audio_cb) (void *opaque, void *buf, size_t size);
static void *ffplay_opaque;

static void sdl_audio_callback(void *opaque, Uint8 * stream, int len)
{
	ffplay_audio_cb(ffplay_opaque, (void *)stream, (size_t) len);
}

static int sdl_open_device(libffplay_audiomgr_t * self, int freq, int channels,
			   int bits_per_sample, void *opaque,
			   void (*audio_cb) (void *, void *, size_t))
{
	SDL_AudioSpec wanted_spec, spec;

	ffplay_audio_cb = audio_cb;
	ffplay_opaque = opaque;

	wanted_spec.channels = channels;
	wanted_spec.freq = freq;
	if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
		return -1;
	}
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = sdl_audio_callback;
	wanted_spec.userdata = NULL;
	if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		fprintf(stderr,
			"No more channel combinations to try, audio open failed\n");
		return -1;
	}

	if (spec.format != AUDIO_S16SYS) {
		fprintf(stderr,
			"SDL advised audio format %d is not supported!\n",
			spec.format);
		return -1;
	}
	if (spec.channels != wanted_spec.channels) {
		fprintf(stderr,
			"SDL advised channel count %d is not supported!\n",
			spec.channels);
		return -1;
	}

	SDL_PauseAudio(0);

	return spec.size;
}

static int sdl_close_device(libffplay_audiomgr_t *self)
{
	SDL_CloseAudio();
	return 0;
}

static void sdl_send_event(libffplay_eventmgr_t *eventmgr, int event_type, void *data)
{
	struct myeventmgr *myeventmgr = (struct myeventmgr *)eventmgr;
	double *ts;
	static double last_ts = 0.0;
	SDL_Event event;

	switch (event_type) {
	case LFP_EVENT_TIMESTAMP:
		ts = data;
		if (fabs(last_ts - *ts) > 0.05) {
			printf("time: %7.2f of %7.2f\r", *ts, libffplay_stream_length(myeventmgr->ctx));
			fflush(stdout);
			last_ts = *ts;
		}
		break;
	case LFP_EVENT_EOF:
		printf("\n\nGonna quit!\n");
		fflush(stdout);
		event.type = FF_QUIT_EVENT;
		event.user.data1 = NULL;
		SDL_PushEvent(&event);
		break;
	}
}

int sdl_alloc_picture_planes(libffplay_videomgr_t *self, libffplay_picture_t *pic, size_t width, size_t height)
{
	SDL_Event event;
	struct myvideomgr *myvideomgr = (struct myvideomgr *)self;
	if (pic->allocated) {
		SDL_FreeYUVOverlay(pic->userdata);
		pic->allocated = 0;
		pic->userdata = NULL;
	}
	pic->width = width;
	pic->height = height;
        event.type = FF_ALLOC_EVENT;
        event.user.data1 = pic;
        SDL_PushEvent(&event);

	SDL_LockMutex(myvideomgr->alloc_mutex);
	SDL_CondWait(myvideomgr->alloc_cond, myvideomgr->alloc_mutex);
	SDL_UnlockMutex(myvideomgr->alloc_mutex);
	if (!pic->userdata) {
		fprintf(stderr, "Failed to allocate overlay\n");
		return -ENOMEM;
	}
	pic->allocated = 1;

	fprintf(stderr, "%s: %dx%d\n", __func__, (int)pic->width, (int)pic->height);

	return 0;
}

void sdl_lock_picture(libffplay_videomgr_t *self, libffplay_picture_t *pic)
{
	SDL_Overlay *bmp = pic->userdata;

	if (!pic->allocated)
		return;

	SDL_LockYUVOverlay(bmp);

        pic->planes[0] = bmp->pixels[0];
        pic->planes[1] = bmp->pixels[2];
        pic->planes[2] = bmp->pixels[1];

        pic->pitches[0] = bmp->pitches[0];
        pic->pitches[1] = bmp->pitches[2];
        pic->pitches[2] = bmp->pitches[1];
}

void sdl_unlock_picture(libffplay_videomgr_t *self, libffplay_picture_t *pic)
{
	SDL_Overlay *bmp = pic->userdata;

	if (!pic->allocated)
		return;

	SDL_UnlockYUVOverlay(bmp);
}


void sdl_free_picture_planes(libffplay_videomgr_t *self, libffplay_picture_t *pic)
{
	SDL_Overlay *bmp = pic->userdata;

	if (!pic->allocated)
		return;

	SDL_FreeYUVOverlay(bmp);
	pic->userdata = NULL;
	pic->allocated = 0;
}

static inline void calc_display_rect(libffplay_videomgr_t *self, libffplay_picture_t *pic)
{
	struct myvideomgr *myvideomgr = (struct myvideomgr *)self;
	SDL_Rect *rect = &myvideomgr->rect;
	float aspect_ratio;
	size_t width, height, x, y;

	if (pic->sar_num == 0)
		aspect_ratio = 0;
	else
		aspect_ratio = (float)pic->sar_num / (float)pic->sar_den;

	if (aspect_ratio <= 0.0)
		aspect_ratio = 1.0;

	aspect_ratio *= (float)pic->width / (float)pic->height;

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = myvideomgr->screen->h;
	width = ((int)rint(height * aspect_ratio));
	if (width > myvideomgr->screen->w) {
		width = myvideomgr->screen->w;
		height = ((int)rint(width / aspect_ratio));
	}
	x = (myvideomgr->screen->w - width) / 2;
	y = (myvideomgr->screen->h - height) / 2;
	rect->x = x;
	rect->y = y;
	rect->w = width > 1 ? width : 1;
	rect->h = height > 1 ? height : 1;
	printf("%dx%d-%dx%d\n",
		rect->x,
		rect->y,
		rect->w,
		rect->h);
}

void sdl_display_picture(libffplay_videomgr_t *self, libffplay_picture_t *pic)
{
	struct myvideomgr *myvideomgr = (struct myvideomgr *)self;
	SDL_Overlay *bmp = pic->userdata;

	if (!pic->allocated)
		return;

	if (pic->width != myvideomgr->old_w || pic->height != myvideomgr->old_h) {
		calc_display_rect(self, pic);
		myvideomgr->old_w = pic->width;
		myvideomgr->old_h = pic->height;
	}
        SDL_DisplayYUVOverlay(bmp, &myvideomgr->rect);
}

libffplay_audiomgr_t audiomgr = {
	.open_device = sdl_open_device,
	.close_device = sdl_close_device,
};


struct myvideomgr videomgr = {
	.videomgr = {
		.alloc_picture_planes = sdl_alloc_picture_planes,
		.lock_picture = sdl_lock_picture,
		.unlock_picture = sdl_unlock_picture,
		.free_picture_planes = sdl_free_picture_planes,
		.display_picture = sdl_display_picture,
	},
};

struct myeventmgr eventmgr = {
	.eventmgr = {
		.send_event = sdl_send_event
	},
};

int main(int argc, char *argv[])
{
	double remaining_time = 0.0;
	SDL_Surface *screen;
	libffplay_ctx_t *ctx;
	int paused = 0;
	int flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Failed to init SDL\n");
		exit(EXIT_FAILURE);
	}
	screen = SDL_SetVideoMode(640, 480, 0, flags);
	if (!screen) {
		fprintf(stderr, "Failed to open SDL screen\n");
		exit(EXIT_FAILURE);
	}
	ctx = libffplay_init();

	libffplay_set_audiomgr(ctx, &audiomgr);

	eventmgr.ctx = ctx;
	libffplay_set_eventmgr(ctx, (libffplay_eventmgr_t *)&eventmgr);

	videomgr.ctx = ctx;
	videomgr.screen = screen;
	videomgr.alloc_mutex = SDL_CreateMutex();
	videomgr.alloc_cond = SDL_CreateCond();
	videomgr.old_h = 0;
	videomgr.old_w = 0;
	libffplay_set_videomgr(ctx, (libffplay_videomgr_t *)&videomgr);

	libffplay_play(ctx, argv[1]);

	/* Event loop */
	for (;;) {
		SDL_Event event;
		SDL_PumpEvents();
		while (!SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_ALLEVENTS)) {
			if (remaining_time > 0.0)
				SDL_Delay(remaining_time * 1000);
			if (!paused)
				libffplay_video_refresh(ctx, &remaining_time);
			else
				remaining_time = 0.01;
			SDL_PumpEvents();
		}
		switch (event.type) {
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_SPACE:
				paused = !paused;
				libffplay_toggle_pause(ctx);
				break;
			case SDLK_LEFT:
				libffplay_seek(ctx, -5.0, LFP_SEEK_CUR);
				break;
			case SDLK_RIGHT:
				libffplay_seek(ctx, 5.0, LFP_SEEK_CUR);
				break;
			default:
				break;
			}
			break;
		case FF_ALLOC_EVENT:
			{
				libffplay_picture_t *pic = event.user.data1;
				SDL_LockMutex(videomgr.alloc_mutex);
				pic->userdata = SDL_CreateYUVOverlay(pic->width, pic->height, SDL_YV12_OVERLAY, screen);
				SDL_CondSignal(videomgr.alloc_cond);
				SDL_UnlockMutex(videomgr.alloc_mutex);
			}
			break;
		case SDL_QUIT:
		case FF_QUIT_EVENT:
			libffplay_stop(ctx);
			libffplay_deinit(ctx);
			SDL_Quit();
			exit(0);
			break;
		}
	}

	return 0;
}
