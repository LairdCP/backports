// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of cc33xx
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/firmware.h>
#include "acx.h"
#include "cc33xx_80211.h"
#include "cmd.h"
#include "conf.h"
#include "event.h"
#include "tx.h"

#define PG2_CHIP_VERSION    2


static int cc33xx_init_phy_vif_config(struct cc33xx *wl,
				      struct cc33xx_vif *wlvif)
{
	int ret;

	ret = cc33xx_acx_slot(wl, wlvif, DEFAULT_SLOT_TIME);
	if (ret < 0)
		return ret;

	return 0;
}

static int cc33xx_init_sta_beacon_filter(struct cc33xx *wl,
					 struct cc33xx_vif *wlvif)
{
	int ret;

	ret = cc33xx_acx_beacon_filter_table(wl, wlvif);
	if (ret < 0)
		return ret;

	/* disable beacon filtering until we get the first beacon */
	ret = cc33xx_acx_beacon_filter_opt(wl, wlvif, false);
	if (ret < 0)
		return ret;

	return 0;
}

static int cc33xx_ap_init_templates(struct cc33xx *wl,
				    struct ieee80211_vif *vif)
{
	struct cc33xx_vif *wlvif = cc33xx_vif_to_data(vif);
	int ret;

	/*
	 * when operating as AP we want to receive external beacons for
	 * configuring ERP protection.
	 */
	ret = cc33xx_acx_beacon_filter_opt(wl, wlvif, false);
	if (ret < 0)
		return ret;

	return 0;
}

static void cc33xx_set_ba_policies(struct cc33xx *wl, struct cc33xx_vif *wlvif)
{
	/* Reset the BA RX indicators */
	wlvif->ba_allowed = true;
	wl->ba_rx_session_count = 0;

	/* BA is supported in STA/AP modes */
	wlvif->ba_support = (wlvif->bss_type == BSS_TYPE_AP_BSS ||
			     wlvif->bss_type == BSS_TYPE_STA_BSS);
}

/* Applies when MAC address is other than 0x0.
 * Routine for actual search in file 
 * data_ptr returned contains the pointer to entry. */
static bool find_calibration_entry(u8 *id, u8 **data_ptr, u8 *stop_address)
{
	u8 default_mac_address[ETH_ALEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	struct calibration_header *calibration_header;
	struct calibration_header_fw *calibration_header_fw;
	u8 *default_calibration = NULL;
	int compare_result = 0;
	bool mac_match = false;
	bool valid_data = false;

	while (*data_ptr < stop_address)
	{
		/* Cast to a struct for convenient fields reading */
		calibration_header = (struct calibration_header *)(*data_ptr);
		calibration_header_fw = &(calibration_header->cal_header_fw);

		if (le16_to_cpu(calibration_header->static_pattern) != 0x7F7F) {
			cc33xx_debug(DEBUG_BOOT,
				     "Problem with sync pattern, read: %x",
				     le16_to_cpu(calibration_header->static_pattern));
			break;
		}

		/* Compare with actual mac address */
		compare_result = memcmp(calibration_header_fw->chip_id, id,
					ETH_ALEN);
		if (0 == compare_result) {
			mac_match = true;
			*data_ptr = (u8 *)calibration_header;
			break;
		}

		/* Compare with default mac address, if it's found save it for
		 * later if necessary (if no calibration for id is found) */
		compare_result = memcmp(calibration_header_fw->chip_id,
					default_mac_address,
					ETH_ALEN);
		if (0 == compare_result)
			default_calibration = (u8 *)calibration_header;

		/* advance ptr by specified payload length to next entry in file */
		*data_ptr = (u8 *)calibration_header
				+ sizeof(struct calibration_header)
				+ le16_to_cpu(calibration_header_fw->length);
	}

	if (false == mac_match) {
		if (NULL != default_calibration) {
			cc33xx_warning("No calibration for device address, "
				       "using default calibration" 
			 	       "(labeled mac FF:FF:FF:FF:FF:FF)");
			/* Take default calibration */
			*data_ptr = default_calibration;
			valid_data = true;
		} else {
			cc33xx_debug(DEBUG_BOOT,"Can't find device's static" 
				    " calibration data in calibration file and" 
				    " cannot find default calibration");
			valid_data = false;
		}
	} else {
		valid_data = true;
		cc33xx_debug(DEBUG_BOOT, "Calibration MAC address match!");
	}
	return valid_data;
}

int download_static_calibration_data(struct cc33xx *wl)
{
	int ret;
	const struct firmware *fw = NULL;
	const char *calibration_file = "ti-connectivity/static_calibration.bin";
	u8 *mac_address;
	bool valid_calibration_data = false;
	bool file_loaded;
	u8 *file_ptr = NULL;
	u8 *calibration_entry_ptr = NULL;
	struct calibration_file_header *file_header = NULL;
	u8 *stop_search_address;

	if(wl->pg_version >= PG2_CHIP_VERSION)
	{
		cc33xx_debug(DEBUG_BOOT,
			     "Chip is PG2, No static calibration needed");
		return 1;
	}
		
	ret = request_firmware(&fw, calibration_file, wl->dev);
	if (ret < 0) {
		cc33xx_warning("Could not get firmware %s: %d,"
			       " proceeding with no calibration",
			       calibration_file, ret);
		valid_calibration_data = false;
		file_loaded = false;
		ret = 0; /* Don't kill driver over this */
		goto out;
	} else {
		file_loaded = true;
	}
	file_ptr = (u8 *)fw->data;

    	file_header = (struct calibration_file_header *)file_ptr;
	cc33xx_debug(DEBUG_BOOT, "Parsing static calibration file version: %d, "
		     "payload struct ver: %d, entries count: %d, file size: %d",
		     file_header->file_version,
		     file_header->payload_struct_version,
		     le16_to_cpu(file_header->entries_count), (int)fw->size);
    
	/* Limit the search to the file's length and skip 4 file header bytes */
	mac_address = (u8 *)wl->efuse_mac_address;
	calibration_entry_ptr = file_ptr
			       	+ sizeof(struct calibration_file_header);
	stop_search_address = file_ptr + fw->size;
	valid_calibration_data = find_calibration_entry(mac_address,
			       				&calibration_entry_ptr,
			       				stop_search_address);

out:
	ret = cc33xx_acx_static_calibration_configure(wl, file_header,
						      calibration_entry_ptr,
						      valid_calibration_data);
	if (file_loaded)
		release_firmware(fw);

	return ret;
}

/* vif-specifc initialization */
static int cc33xx_init_sta_role(struct cc33xx *wl, struct cc33xx_vif *wlvif)
{
	int ret = cc33xx_acx_group_address_tbl(wl, wlvif, true, NULL, 0);
	if (ret < 0)
		return ret;

	/* Beacon filtering */
	ret = cc33xx_init_sta_beacon_filter(wl, wlvif);
	if (ret < 0)
		return ret;

	return 0;
}


/* vif-specific initialization */
static int cc33xx_init_ap_role(struct cc33xx *wl, struct cc33xx_vif *wlvif)
{
	int ret;

	/* initialize Tx power */
	ret = cc33xx_acx_tx_power(wl, wlvif, wlvif->power_level);
	if (ret < 0)
		return ret;

	if (wl->radar_debug_mode)
		wlcore_cmd_generic_cfg(wl, wlvif,
				       WLCORE_CFG_FEATURE_RADAR_DEBUG,
				       wl->radar_debug_mode, 0);

	return 0;
}

int cc33xx_init_vif_specific(struct cc33xx *wl, struct ieee80211_vif *vif)
{
	struct cc33xx_vif *wlvif = cc33xx_vif_to_data(vif);
	struct conf_tx_ac_category *conf_ac;
	struct conf_tx_tid *conf_tid;
	struct conf_tx_ac_category ac_conf[4];
	struct conf_tx_tid tid_conf[8];
	struct conf_tx_settings *tx_settings = &wl->conf.host_conf.tx;
	struct conf_tx_ac_category* p_wl_host_ac_conf = &tx_settings->ac_conf0;
	struct conf_tx_tid* p_wl_host_tid_conf = &tx_settings->tid_conf0;
	bool is_ap = (wlvif->bss_type == BSS_TYPE_AP_BSS);
	u8 ps_scheme = wl->conf.mac.ps_scheme;
	int ret, i;
	
	/* consider all existing roles before configuring psm. */

	if (wl->ap_count == 0 && is_ap) { /* first AP */
		ret = cc33xx_acx_sleep_auth(wl, CC33XX_PSM_ELP);
		if (ret < 0)
			return ret;

		/* unmask ap events */
		wl->event_mask |= wl->ap_event_mask;

	/* first STA, no APs */
	} else if (wl->sta_count == 0 && wl->ap_count == 0 && !is_ap) {
		u8 sta_auth = wl->conf.host_conf.conn.sta_sleep_auth;
		/* Configure for power according to debugfs */
		if (sta_auth != CC33XX_PSM_ILLEGAL)
			ret = cc33xx_acx_sleep_auth(wl, sta_auth);
		/* Configure for ELP power saving */
		else
			ret = cc33xx_acx_sleep_auth(wl, CC33XX_PSM_ELP);

		if (ret < 0)
			return ret;
	}

	/* Mode specific init */
	if (is_ap) {
		ret = cc33xx_init_ap_role(wl, wlvif);
		if (ret < 0)
			return ret;
	} else {
		ret = cc33xx_init_sta_role(wl, wlvif);
		if (ret < 0)
			return ret;
	}

	cc33xx_init_phy_vif_config(wl, wlvif);

	/* Default TID/AC configuration */
	BUG_ON(tx_settings->tid_conf_count != tx_settings->ac_conf_count);
	memcpy(ac_conf,p_wl_host_ac_conf,4*sizeof(struct conf_tx_ac_category));
	memcpy(tid_conf,p_wl_host_tid_conf,8*sizeof(struct conf_tx_tid));

	for (i = 0; i < tx_settings->tid_conf_count; i++) {
		conf_ac =  &ac_conf[i];
		conf_tid = &tid_conf[i];

		/* If no ps poll is used, send legacy ps scheme in cmd */
		if (ps_scheme == PS_SCHEME_NOPSPOLL)
			ps_scheme = PS_SCHEME_LEGACY;

		//TODO: RazB - need to configure MUEDCA
		ret = cc33xx_tx_param_cfg(wl, wlvif, conf_ac->ac,
					  conf_ac->cw_min, conf_ac->cw_max,
					  conf_ac->aifsn, conf_ac->tx_op_limit,
					  false, ps_scheme, conf_ac->is_mu_edca,
					  conf_ac->mu_edca_aifs,
					  conf_ac->mu_edca_ecw_min_max,
					  conf_ac->mu_edca_timer);
	
		if (ret < 0)
			return ret;
	}

	/* Mode specific init - post mem init */
	if (is_ap)
		ret = cc33xx_ap_init_templates(wl, vif);

	if (ret < 0)
		return ret;

	/* Configure initiator BA sessions policies */
	cc33xx_set_ba_policies(wl, wlvif);

	return 0;
}

int cc33xx_hw_init(struct cc33xx *wl)
{
	int ret = 0;
	ret = cc33xx_acx_init_mem_config(wl);
	
	cc33xx_debug(DEBUG_CC33xx, "Skipping cc33xx_hw_init");	
	cc33xx_debug(DEBUG_TX, "available tx blocks: %d", 16);
	wl->last_fw_rls_idx = 0;
	wl->partial_rx.status = CURR_RX_START;
	return 0;
}

int cc33xx_download_ini_params_and_wait(struct cc33xx *wl)
{
	struct cc33xx_cmd_ini_params_download *cmd;
	size_t command_size = ALIGN((sizeof(*cmd) + sizeof(wl->conf)),4);
	int ret;

	if (wl->conf.core.enable_FlowCtrl == 0)
		cc33xx_warning("flow control disable - BLE will not work");
	
	cc33xx_set_max_buffer_size(wl,INI_MAX_BUFFER_SIZE);

	cc33xx_debug(DEBUG_ACX,
		     "Downloading INI Params and Configurations to FW, "
		     "INI Bin File Payload Length: %d", (int)sizeof(wl->conf));
	cmd = kzalloc(command_size, GFP_KERNEL);
	if (!cmd) {
		cc33xx_error("INI Params Download: "
			     "process failed due to memory allocation failure");
		cc33xx_set_max_buffer_size(wl,CMD_MAX_BUFFER_SIZE);
		return -ENOMEM;
	}

	cmd->length = cpu_to_le32(sizeof(wl->conf));
	
	/* copy INI file params payload */
	memcpy((cmd->payload), &(wl->conf), sizeof(wl->conf));

	ret = cc33xx_cmd_send(wl, CMD_DOWNLOAD_INI_PARAMS,
			      cmd, command_size, 0);
	if (ret < 0) {
		cc33xx_warning("download INI params to FW command "
			       "sending failed: %d", ret);
	} else {
		cc33xx_debug(DEBUG_BOOT, "INI Params downloaded successfully");
	}

	cc33xx_set_max_buffer_size(wl, CMD_MAX_BUFFER_SIZE);
	kfree(cmd);
	return ret;
}
