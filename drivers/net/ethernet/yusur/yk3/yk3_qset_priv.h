/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_QSET_PRIV_H
#define _YK3_QSET_PRIV_H

#include "yk3_base.h"

enum yk3_qset_msgid {
	MSG_QSETID_ALLOC,
	MSG_QSETID_FREE,
	MSG_QSETID_GET,
	MSG_QSETID_GET_PEER,
	MSG_QSET_START,
	MSG_QSET_STOP,
	MSG_QSET_MAX,
};

struct yk3_qset_msg {
	enum yk3_qset_msgid msgid;
	union {
		struct {
			union {
				struct {
					u32 pf_id:8;
					u32 vf_id:16;
					enum yk3_ndev_type type;
				} req;
				struct {
					int qsetid;
				} rsp;
			};
		} qsetid_alloc;
		struct {
			union {
				struct {
					u16 qsetid;
				} req;
			};
		} qsetid_free;
		struct {
			union {
				struct {
					u32 pf_id:8;
					u32 vf_id:16;
					enum yk3_ndev_type type;
				} req;
				struct {
					int qsetid;
				} rsp;
			};
		} qsetid_get;
		struct {
			union {
				struct {
					u32 pf_id:8;
					u32 vf_id:16;
					enum yk3_ndev_type type;
					u16 qsetid;
				} req;
				struct {
					int qsetid;
				} rsp;
			};
		} qsetid_get_peer;
		struct {
			union {
				struct {
					u16 qsetid;
					struct yk3_queuebase rx_qbase;
					struct yk3_queuebase tx_qbase;
				} req;
			};
		} qset_start;
		struct {
			union {
				struct {
					u16 qsetid;
					struct yk3_queuebase tx_qbase;
				} req;
			};
		} qset_stop;
	};
};

void yk3_qset_set_qset2q(struct yk3_pdev_priv *pdev_priv, u16 qsetid,
			 struct yk3_queuebase qbase);
void yk3_qset_set_q2qset(struct yk3_pdev_priv *pdev_priv, u16 qsetid,
			 struct yk3_queuebase qbase);

#endif /* _YK3_QSET_PRIV_H */
