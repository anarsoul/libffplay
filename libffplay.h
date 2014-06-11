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

struct libffplay_ctx;
typedef struct libffplay_ctx libffplay_ctx_t;

typedef lockmgr_fn_t struct libffplay_audiomgr {
	void *userdata;
	int (*open_device) (void *userdata, int freq, int channels,
			    int bits_per_sample, void (*audio_cb) (void *buf,
								   size_t size,
								   int pos));
	int (*close_device) (void *userdata);
};
typedef struct libffplay_audiomgr libffplay_audiomgr_t;

struct libffplay_videomgr {
	void *userdata;
};
typedef struct libffplay_videomgr libffplay_videomgr_t;

enum {
	LFP_SEEK_SET = 0,
	LFP_SEEK_CUR = 1,
	LFP_SEEK_END = 2,
};

libffplay_ctx_t *libffplay_init(void);
void libffplay_set_audiomgr(libffplay_ctx_t * ctx,
			    libffplay_audiomgr_t * audiomgr);
void libffplay_set_videomgr(libffplay_ctx_t * ctx,
			    libffplay_videomgr_t * videomgr);
/* returns duration if success, negative error code otherwise */
int libffplay_play(libffplay_ctx_t * ctx, const char *filename);
void libffplay_pause(libffplay_ctx_t * ctx);
void libffplay_resume(libffplay_ctx_t * ctx);
void libffplay_seek(libffplay_ctx_t * ctx, int pos, int whence);
void libffplay_stop(libffplay_ctx_t * ctx);
void libffplay_exit(libffplay_ctx_t * ctx);

#endif
