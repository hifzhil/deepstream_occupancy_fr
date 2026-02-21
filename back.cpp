/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <gst/gst.h>
#include <glib.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include "gstnvdsmeta.h"
#include "nvds_analytics_meta.h"
#include "analytics.h"

/* custom_parse_nvdsanalytics_meta_data 
 * and extract nvanalytics metadata */
	extern "C" void
analytics_custom_parse_nvdsanalytics_meta_data (NvDsMetaList *l_user, AnalyticsUserMeta *data)
{
	static std::unordered_map<std::string, int> source_map;
	static time_t last_log_time = 0;
	std::stringstream out_string;
	NvDsUserMeta *user_meta = (NvDsUserMeta *) l_user->data;
	/* convert to metadata */
	NvDsAnalyticsFrameMeta *meta =
		(NvDsAnalyticsFrameMeta *) user_meta->user_meta_data;
	
	/* Check if 1 second has passed since last log */
	time_t current_time = time(NULL);
	if (current_time <= last_log_time) {
		return;  // Skip if less than 1 second has passed
	}
	last_log_time = current_time;
	
	/* Get formatted timestamp */
	char timestamp[64];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&current_time));
	
	/* Create a unique key for this source based on the line crossing names */
	std::string source_key;
	for (const auto &lc : meta->objLCCumCnt) {
		source_key += lc.first + "_";
	}
	
	/* Assign source ID if not already assigned */
	if (source_map.find(source_key) == source_map.end()) {
		source_map[source_key] = source_map.size();
	}
	int source_id = source_map[source_key];
	
	/* Fill the data for entry, exit, occupancy */
	data->lcc_cnt_entry = 0;
	data->lcc_cnt_exit = 0;
	data->lccum_cnt = 0;
	data->lcc_cnt_entry = meta->objLCCumCnt["Entry"];
	data->lcc_cnt_exit = meta->objLCCumCnt["Exit"];

	if (meta->objLCCumCnt["Entry"] > meta->objLCCumCnt["Exit"])
		data->lccum_cnt = meta->objLCCumCnt["Entry"] - meta->objLCCumCnt["Exit"];

	/* Print detailed information with source ID and timestamp */
	g_print("\n=== Analytics Update for Source %d [%s] ===\n", source_id, timestamp);
	g_print("Entry Count: %u\n", data->lcc_cnt_entry);
	g_print("Exit Count: %u\n", data->lcc_cnt_exit);
	g_print("Current Occupancy: %u\n", data->lccum_cnt);
	
	/* Print line crossing counts if available */
	if (!meta->objLCCumCnt.empty()) {
		g_print("Line Crossing Details for Source %d:\n", source_id);
		for (const auto &lc : meta->objLCCumCnt) {
			g_print("  %s: %lu\n", lc.first.c_str(), lc.second);
		}
	}

	/* Print ROI counts if available */
	if (!meta->objInROIcnt.empty()) {
		g_print("ROI Counts for Source %d:\n", source_id);
		for (const auto &roi : meta->objInROIcnt) {
			g_print("  %s: %u\n", roi.first.c_str(), roi.second);
		}
	}
	
	g_print("========================\n\n");
}


