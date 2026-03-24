/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2025. All rights reserved.
 *
 * Description: Functions definition of ipourma_addr_res
 */

#ifndef _IPOURMA_ADDR_RES_H
#define _IPOURMA_ADDR_RES_H

#include "ipourma_types.h"

int ipourma_resolve_ipaddr(struct net_device *dev,
	u16 proto, union ubcore_eid *eid, struct in6_addr *addr);

#endif
