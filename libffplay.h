/*
 * Copyright (c) 2014 Vasily Khoruzhick <anarsoul@gmail.com>
 *
 * ffplay is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ffplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ffplay; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __LIBFFPLAY_H
#define __LIBFFPLAY_H 1

#include <sys/types.h>

struct libffplay_ctx;
typedef struct libffplay_ctx libffplay_ctx_t;

typedef struct libffplay_audiomgr libffplay_audiomgr_t;
struct libffplay_audiomgr {
	int (*open_device) (libffplay_audiomgr_t *self, int freq, int channels,
			    int bits_per_sample, void *opaque,
			    void (*audio_cb) (void *opaque,
					      void *buf,
					      size_t size));
	int (*close_device) (libffplay_audiomgr_t *self);
};

typedef struct libffplay_videomgr libffplay_videomgr_t;
struct libffplay_videomgr {
};

enum {
	LFP_EVENT_EOF,
	LFP_EVENT_TIMESTAMP,
};

typedef struct libffplay_eventmgr libffplay_eventmgr_t;
struct libffplay_eventmgr {
	void (*send_event)(libffplay_eventmgr_t *self, int event_type, void *data);
};

enum {
	LFP_SEEK_SET = 0,
	LFP_SEEK_CUR = 1,
};

libffplay_ctx_t *libffplay_init(void);
void libffplay_set_audiomgr(libffplay_ctx_t *ctx,
			    libffplay_audiomgr_t *audiomgr);
void libffplay_set_videomgr(libffplay_ctx_t *ctx,
			    libffplay_videomgr_t *videomgr);
void libffplay_set_eventmgr(libffplay_ctx_t *ctx,
			    libffplay_eventmgr_t *eventmgr);
/* returns duration if success, negative error code otherwise */
int libffplay_play(libffplay_ctx_t * ctx, const char *filename);
void libffplay_pause(libffplay_ctx_t * ctx);
void libffplay_resume(libffplay_ctx_t * ctx);
void libffplay_toggle_pause(libffplay_ctx_t *ctx);
void libffplay_seek(libffplay_ctx_t * ctx, double pos, int whence);
double libffplay_tell(libffplay_ctx_t *ctx);
double libffplay_stream_length(libffplay_ctx_t *ctx);
void libffplay_stop(libffplay_ctx_t * ctx);
void libffplay_exit(libffplay_ctx_t * ctx);

#endif
