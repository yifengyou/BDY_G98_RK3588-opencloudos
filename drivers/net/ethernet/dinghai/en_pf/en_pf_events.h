#ifndef __EN_PF_EVENTS_H__
#define __EN_PF_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

int32_t dh_pf_events_init(struct dh_core_dev *dev);
void dh_pf_events_uninit(struct dh_core_dev *dev);
void dh_pf_sriov_cap_cfg_uninit(struct dh_core_dev *dev);
void zxdh_pf_nh_attach(struct dh_core_dev *dev, struct dh_nb *nb, bool attach);

#ifdef __cplusplus
}
#endif

#endif
