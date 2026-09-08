/* SPDX-License-Identifier: GPL-2.0 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM yk3

#if !defined(_YK3_LAN_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _YK3_LAN_TP_H

#include <linux/tracepoint.h>

/* tracepoint for lan */
TRACE_EVENT(yk3_lan_mbox_set_mac,
	    TP_PROTO(const u8 *mac, const u8 *old_mac, u16 qset, u16 op),
	    TP_ARGS(mac, old_mac, qset, op),
	    TP_STRUCT__entry(__array(u8, mac, 6)
			     __array(u8, old_mac, 6)
			     __field(u16, qset)
			     __field(u16, op)
	    ),
	    TP_fast_assign(memcpy(__entry->mac, mac, 6);
			   memcpy(__entry->old_mac, old_mac, 6);
			   __entry->qset = qset;
			   __entry->op = op;
	    ),
	    TP_printk("mac: %pM, old_mac: %pM, qset %d, op %d",
		      __entry->mac, __entry->old_mac,
		      __entry->qset, __entry->op)
);

TRACE_EVENT(yk3_lan_mbox_vf_mac,
	    TP_PROTO(const u8 *mac, u16 qset),
	    TP_ARGS(mac, qset),
	    TP_STRUCT__entry(__array(u8, mac, 6)
			    __field(u16, qset)
	    ),
	    TP_fast_assign(memcpy(__entry->mac, mac, 6);
			   entry->qset = qset;
	    ),
	    TP_printk("pf recv vf set mac: %pM, qset %d",
		      __entry->mac, __entry->qset)
);

TRACE_EVENT(yk3_lan_mbox_set_vlan,
	    TP_PROTO(u16 vlan_id, u16 proto, u16 qset, u16 op),
	    TP_ARGS(vlan_id, proto, qset, op),
	    TP_STRUCT__entry(__field(u16, vlan_id)
			     __field(u16, proto)
			     __field(u16, qset)
			     __field(u16, op)
	    ),
	    TP_fast_assign(__entry->vlan_id = vlan_id;
			   __entry->proto = proto;
			   __entry->qset = qset;
			   __entry->op = op;
	    ),
	    TP_printk("vlan_id %d, proto 0x%04x, qset %d, op %d",
		      __entry->vlan_id, __entry->proto, __entry->qset, __entry->op)
);

#endif /* _YK3_LAN_TP_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE yk3_lan_trace
#include <trace/define_trace.h>
