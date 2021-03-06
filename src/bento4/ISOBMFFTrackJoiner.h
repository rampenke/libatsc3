/*
 * ISOBMFFTrackJoiner.h
 *
 *  Created on: Feb 20, 2019
 *      Author: jjustman
 */

//linux compat
//#include <_types/_uint32_t.h>
//#include <_types/_uint64_t.h>
//#include <_types/_uint8_t.h>


#include <list>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
using namespace std;

//#include "../atsc3_utils.h"
#include "../atsc3_isobmff_trun_box.h"
#include "../atsc3_lls_sls_monitor_output_buffer.h"

#include "Ap4.h"
#include "Ap4Atom.h"

#ifndef BENTO4_ISOBMFFTRACKJOINER_H_
#define BENTO4_ISOBMFFTRACKJOINER_H_

#include "ISOBMFFTrackJoiner_firewall_gpl.h"

#if defined (__cplusplus)
extern "C" {
#endif

extern FILE* __ISOBMFFTRACKJOINER_DEBUG_LOG_FILE;
extern bool  __ISOBMFFTRACKJOINER_DEBUG_LOG_AVAILABLE;

extern int _ISOBMFFTRACKJOINER_INFO_ENABLED;
extern int _ISOBMFFTRACKJOINER_DEBUG_ENABLED;
extern int _ISOBMFFTRACKJOINER_TRACE_ENABLED;

void ISOBMFF_track_joiner_monitor_output_buffer_parse_and_build_joined_alc_boxes(lls_sls_monitor_output_buffer_t* lls_sls_monitor_output_buffer, AP4_MemoryByteStream** output_stream);
void ISOBMFF_track_joiner_monitor_output_buffer_parse_and_build_joined_mmt_boxes(lls_sls_monitor_output_buffer_t* lls_sls_monitor_output_buffer, AP4_MemoryByteStream** output_stream);
void ISOBMFF_track_joiner_monitor_output_buffer_parse_and_build_joined_mmt_rebuilt_boxes(lls_sls_monitor_output_buffer_t* lls_sls_monitor_output_buffer, AP4_MemoryByteStream** output_stream_p);

uint32_t ISOBMFF_rebuild_moof_from_sample_data(lls_sls_monitor_buffer_isobmff_t* lls_sls_monitor_buffer_isobmff, AP4_MemoryByteStream** output_stream_p);
void parseAndBuildJoinedBoxes_from_lls_sls_monitor_output_buffer(lls_sls_monitor_output_buffer_t* lls_sls_monitor_output_buffer, block_t* audio_output_buffer, block_t* video_output_buffer, AP4_MemoryByteStream** output_stream_p);

void parseAndBuildJoinedBoxes(ISOBMFFTrackJoinerFileResouces_t*, AP4_ByteStream* output_stream);
void parseAndBuildJoinedBoxesFromMemory(uint8_t* file1_payload, uint32_t file1_size, uint8_t* file2_payload, uint32_t file2_size, AP4_ByteStream* output_stream);


void dumpFullMetadata(list<AP4_Atom*> atomList);
void printBoxType(AP4_Atom* atom);

#define __ISOBMFF_JOINER_PRINTLN(level, ...) if(__ISOBMFFTRACKJOINER_DEBUG_LOG_AVAILABLE && !__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE) { \
		__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE = fopen("isobmff.debug", "w"); \
			if(!__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE) { \
				__ISOBMFFTRACKJOINER_DEBUG_LOG_AVAILABLE = false; \
				__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE = stderr; \
			} \
	} \
	fprintf(__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE, "%s:%d:%s :",__FILE__,__LINE__, level); \
	fprintf(__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE, __VA_ARGS__);\
	fprintf(__ISOBMFFTRACKJOINER_DEBUG_LOG_FILE, "\r%s","\n")

#define __ISOBMFF_JOINER_INFO(...)  if(_ISOBMFFTRACKJOINER_INFO_ENABLED) {  __ISOBMFF_JOINER_PRINTLN("INFO :", __VA_ARGS__); }
#define __ISOBMFF_JOINER_DEBUG(...) if(_ISOBMFFTRACKJOINER_DEBUG_ENABLED) { __ISOBMFF_JOINER_PRINTLN("DEBUG:", __VA_ARGS__); }

#if defined (__cplusplus)
}
#endif


//list<AP4_Atom*> ISOBMFFTrackParse(uint8_t* full_mpu_payload, uint32_t full_mpu_payload_size);
//internal
list<AP4_Atom*> ISOBMFFTrackParse(block_t* isobmff_track_block);

//ISOBMFFTrackJoinerFileResouces_t* loadFileResources(const char*, const char*);



#endif /* BENTO4_ISOBMFFTRACKJOINER_H_ */
