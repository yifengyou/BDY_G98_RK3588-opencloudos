/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_DMAMAP_H
#define _YK3_DMAMAP_H

#include "yk3_base.h"

struct yk3_dmamap_table;

struct yk3_dmamap_table *yk3_dmamap_table_create(struct device *dev);
void yk3_dmamap_table_destroy(struct yk3_dmamap_table *tbl);
int yk3_dmamap_map(struct yk3_dmamap_table *tbl, u64 iova, u64 size,
		   u64 pa, void *opaque);
void yk3_dmamap_unmap(struct yk3_dmamap_table *tbl, u64 iova, u64 size);
bool yk3_dmamap_exist(struct yk3_dmamap_table *tbl, u64 start, u64 end);
void yk3_dmamap_update_refcnt(struct yk3_dmamap_table *tbl, u64 start, u64 end);

#endif /* _YK3_DMAMAP_H */
