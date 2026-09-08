/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_RINGBASE_H
#define _YK3_RINGBASE_H

#include <linux/types.h>
#include <linux/kernel.h>

struct yk3_ringbase {
	u16 head;
	u16 tail;
	u16 size;
	u16 mask;
};

static inline void yk3_ringb_init(struct yk3_ringbase *ring, u16 size)
{
	ring->head = 0;
	ring->tail = 0;
	ring->size = size;
	ring->mask = size - 1;
}

static inline u16 yk3_ringb_head(struct yk3_ringbase *ring)
{
	return READ_ONCE(ring->head) & ring->mask;
}

static inline bool yk3_ringb_update_head(struct yk3_ringbase *ring, u16 head)
{
	if (READ_ONCE(ring->head) == head)
		return false;
	WRITE_ONCE(ring->head, head);
	return true;
}

static inline u16 yk3_ringb_head_orig(struct yk3_ringbase *ring)
{
	return READ_ONCE(ring->head);
}

static inline u16 yk3_ringb_tail(struct yk3_ringbase *ring)
{
	return READ_ONCE(ring->tail) & ring->mask;
}

static inline u16 yk3_ringb_tail_orig(struct yk3_ringbase *ring)
{
	return READ_ONCE(ring->tail);
}

static inline u16 yk3_ringb_size(struct yk3_ringbase *ring)
{
	return ring->size;
}

static inline void yk3_ringb_push(struct yk3_ringbase *ring)
{
	//ring->head++;	/* WRITE_ONCE ?? */
	WRITE_ONCE(ring->head, yk3_ringb_head_orig(ring) + 1);
}

static inline void yk3_ringb_push_multi(struct yk3_ringbase *ring, u16 count)
{
	//ring->head += count;
	WRITE_ONCE(ring->head, yk3_ringb_head_orig(ring) + count);
}

static inline void yk3_ringb_pop(struct yk3_ringbase *ring)
{
	//ring->tail++;
	WRITE_ONCE(ring->tail, yk3_ringb_tail_orig(ring) + 1);
}

static inline void yk3_ringb_pop_multi(struct yk3_ringbase *ring, u16 count)
{
	//ring->tail += count;
	WRITE_ONCE(ring->tail, yk3_ringb_tail_orig(ring) + count);
}

static inline bool yk3_ringb_empty(struct yk3_ringbase *ring)
{
	return yk3_ringb_head_orig(ring) == yk3_ringb_tail_orig(ring);
}

static inline u16 yk3_ringb_used(struct yk3_ringbase *ring)
{
	return READ_ONCE(ring->head) - READ_ONCE(ring->tail);
}

static inline bool yk3_ringb_full(struct yk3_ringbase *ring)
{
	return yk3_ringb_used(ring) == ring->size;
}

static inline u16 yk3_ringb_left(struct yk3_ringbase *ring)
{
	return (ring->size - yk3_ringb_used(ring));
}

static inline u16 yk3_ringb_bottom_left(struct yk3_ringbase *ring)
{
	return min_t(u16, ring->size - yk3_ringb_head(ring), yk3_ringb_left(ring));
}

static inline bool yk3_ringb_in_bottom(struct yk3_ringbase *ring)
{
	return yk3_ringb_head(ring) == (ring->size - yk3_ringb_bottom_left(ring));
}

static inline u16 yk3_ringb_top_left(struct yk3_ringbase *ring)
{
	return (u16)(yk3_ringb_left(ring) - yk3_ringb_bottom_left(ring));
}

#endif /* _YK3_RINGBASE_H */
