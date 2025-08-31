#ifndef __SSPAPVDEC__
#define __SSPAPVDEC__
#include <cstdint>

/* 2: EVENT HEADER */
typedef struct
{
    uint32_t trigger_number:27;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} sspApv_event_header;

typedef union
{
    uint32_t raw;
    sspApv_event_header bf;
} sspApv_event_header_t;

/* 3: TRIGGER TIME */
typedef struct
{
    uint32_t trigger_time_l:24;
    uint32_t undef:3;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} sspApv_trigger_time_1;

typedef union
{
    uint32_t raw;
    sspApv_trigger_time_1 bf;
} sspApv_trigger_time_1_t;

typedef struct
{
    uint32_t trigger_time_h:24;
    uint32_t undef:3;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} sspApv_trigger_time_2;

typedef union
{
    uint32_t raw;
    sspApv_trigger_time_2 bf;
} sspApv_trigger_time_2_t;

/* 5: MPD Frame */
/*
typedef struct
{
    uint32_t mpd_id:5;
    uint32_t undef:11;
    uint32_t fiber:5;
    uint32_t flags:6;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} sspApv_mpd_frame_1;
*/
/* latest firmware? hall Gen-rp setup 08/18/2022 */
typedef struct
{
    uint32_t mpd_id:5;
    uint32_t undef:11;
    uint32_t fiber:6;
    uint32_t flags:5;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} sspApv_mpd_frame_1;


typedef union
{
    uint32_t raw;
    sspApv_mpd_frame_1 bf;
} sspApv_mpd_frame_1_t;

/* 5: APV Data */
typedef struct
{
    uint32_t apv_sample0:13;
    uint32_t apv_sample1:13;
    uint32_t apv_channel_num_40:5;
    uint32_t data_type_defining:1;
} sspApv_apv_data_1;

typedef union
{
    uint32_t raw;
    sspApv_apv_data_1 bf;
} sspApv_apv_data_1_t;

typedef struct
{
    uint32_t apv_sample2:13;
    uint32_t apv_sample3:13;
    uint32_t apv_channel_num_65:5;
    uint32_t data_type_defining:1;
} sspApv_apv_data_2;

typedef union
{
    uint32_t raw;
    sspApv_apv_data_2 bf;
} sspApv_apv_data_2_t;

typedef struct
{
    uint32_t apv_sample4:13;
    uint32_t apv_sample5:13;
    uint32_t apv_id:5;
    uint32_t data_type_defining:1;
} sspApv_apv_data_3;

typedef union
{
    uint32_t raw;
    sspApv_apv_data_3 bf;
} sspApv_apv_data_3_t;

// block header
typedef struct
{
    uint32_t number_of_events_in_block:8;
    uint32_t event_block_number:10;
    uint32_t module_ID:4;
    uint32_t slot_number:5;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} block_header;

typedef union
{
    uint32_t raw;
    block_header bf;
} block_header_t;

/* 1: BLOCK TRAILER */
typedef struct
{
    uint32_t words_in_block:22;
    uint32_t slot_number:5;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} block_trailer;

typedef union
{
    uint32_t raw;
    block_trailer bf;
} block_trailer_t;

// generic word definition
typedef struct
{
    uint32_t undef:27;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} generic_data_word;

typedef union
{
    uint32_t raw;
    generic_data_word bf;
} generic_data_word_t;

// data not valid
typedef struct
{
    uint32_t undef:22;
    uint32_t slot_number:5;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} data_not_valid;

typedef union
{
    uint32_t raw;
    data_not_valid bf;
} data_not_valid_t;

// filler
typedef struct
{
    uint32_t undef:22;
    uint32_t slot_number:5;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} filler_word;

typedef union
{
    uint32_t raw;
    filler_word bf;
} filler_word_t;

// mpd timestamp header
typedef struct
{
    uint32_t timestamp_fine:8;
    uint32_t timestamp_coarse0:16;
    uint32_t udef:3;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} mpd_timestamp_header_word_1;

typedef union
{
    uint32_t raw;
    mpd_timestamp_header_word_1 bf;
} mpd_timestamp_header_word_1_t;

typedef struct
{
    uint32_t timestamp_coarse1:24;
    uint32_t udef:7;
    uint32_t data_type_defining:1;
} mpd_timestamp_header_word_2;

typedef union
{
    uint32_t raw;
    mpd_timestamp_header_word_2 bf;
} mpd_timestamp_header_word_2_t;

typedef struct
{
    uint32_t event_count:20;
    uint32_t udef:11;
    uint32_t data_type_defining:1;
} mpd_timestamp_header_word_3;

typedef union
{
    uint32_t raw;
    mpd_timestamp_header_word_3 bf;
} mpd_timestamp_header_word_3_t;

// mpd debug header
typedef struct
{
    uint32_t CM_T0:13;
    uint32_t CM_T1:13;
    uint32_t undef:1;
    uint32_t data_type_tag:4;
    uint32_t data_type_defining:1;
} mpd_debug_header_word_1;

typedef union
{
    uint32_t raw;
    mpd_debug_header_word_1 bf;
} mpd_debug_header_word_1_t;

typedef struct
{
    uint32_t CM_T2:13;
    uint32_t CM_T3:13;
    uint32_t udef:5;
    uint32_t data_type_defining:1;
} mpd_debug_header_word_2;

typedef union
{
    uint32_t raw;
    mpd_debug_header_word_2 bf;
} mpd_debug_header_word_2_t;

typedef struct
{
    uint32_t CM_T4:13;
    uint32_t CM_T5:13;
    uint32_t udef:5;
    uint32_t data_type_defining:1;
} mpd_debug_header_word_3;

typedef union
{
    uint32_t raw;
    mpd_debug_header_word_3 bf;
} mpd_debug_header_word_3_t;

#endif /* __SSPAPVDEC__ */
