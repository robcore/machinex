#ifndef _UAPI_MSM_MDP_H_
#define _UAPI_MSM_MDP_H_

#include <linux/types.h>
#include <linux/fb.h>

struct mdp_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct mdp_img {
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t offset;
	int memory_id;		/* the file descriptor */
	uint32_t priv;
};
#endif /*_UAPI_MSM_MDP_H_*/
