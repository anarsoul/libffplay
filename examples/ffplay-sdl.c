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
#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

#include <stdio.h>

#include <SDL.h>
#include <SDL_thread.h>

#include "libffplay.h"

struct myeventmgr {
	libffplay_eventmgr_t eventmgr;
	libffplay_ctx_t *ctx;
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
	SDL_Event event;

	switch (event_type) {
	case LFP_EVENT_TIMESTAMP:
		ts = data;
		printf("time: %7.2f of %7.2f\r", *ts, libffplay_stream_length(myeventmgr->ctx));
		fflush(stdout);
		break;
	case LFP_EVENT_EOF:
		printf("Gonna quit!\n");
		fflush(stdout);
		event.type = FF_QUIT_EVENT;
		event.user.data1 = NULL;
		SDL_PushEvent(&event);
		break;
	}
}

libffplay_audiomgr_t audiomgr = {
	.open_device = sdl_open_device,
	.close_device = sdl_close_device,
};

libffplay_videomgr_t videomgr = {
};

struct myeventmgr eventmgr = {
	.eventmgr = { .send_event = sdl_send_event },
};

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	libffplay_ctx_t *ctx;
	int flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Failed to init SDL\n");
		exit(EXIT_FAILURE);
	}
	screen = SDL_SetVideoMode(320, 240, 0, flags);
	if (!screen) {
		fprintf(stderr, "Failed to open SDL screen\n");
		exit(EXIT_FAILURE);
	}
	ctx = libffplay_init();
	libffplay_set_audiomgr(ctx, &audiomgr);
	eventmgr.ctx = ctx;
	libffplay_set_eventmgr(ctx, (libffplay_eventmgr_t *)&eventmgr);
	libffplay_play(ctx, argv[1]);

	/* Event loop */
	for (;;) {
		SDL_Event event;
		SDL_PumpEvents();
		if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_ALLEVENTS)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
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
			case SDL_QUIT:
			case FF_QUIT_EVENT:
				libffplay_stop(ctx);
				SDL_Quit();
				exit(0);
				break;
			}
		}
	}

	return 0;
}
