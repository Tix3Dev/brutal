#pragma once
#include <brutal/base.h>
#include <brutal/io.h>
#include <stddef.h>

typedef struct PACKED
{
    be_uint32_t tag;
    be_uint32_t checksum;
    be_uint32_t offset;
    be_uint32_t length;
} TtfDirTableEntry;

typedef struct PACKED
{
    be_uint32_t version;
    be_uint16_t num_table;
    be_uint16_t search_range;
    be_uint16_t entry_selector;
    be_uint16_t range_shift;
    TtfDirTableEntry dir_table[];
} TtfHeader;

typedef struct PACKED
{
    be_uint32_t version;
    be_uint32_t revision;
    be_uint32_t checksum_adj;
    be_uint32_t magic;
    be_uint16_t flags;
    be_uint16_t unit_per_em;
    be_uint64_t created;
    be_uint64_t modified;
    be_int16_t x_min;
    be_int16_t y_min;
    be_int16_t x_max;
    be_int16_t y_max;
    be_int16_t mac_style;
    be_int16_t lowest_rec_ppem;
    be_int16_t font_direction_hint;
    be_int16_t index_to_loc_format;
    be_int16_t glyph_data_format;
} TtfHead;

typedef struct PACKED
{
    be_uint16_t format;
    be_uint16_t length;
    be_uint16_t version;

} TtfSubtable;

typedef enum
{
    TTF_PLATEFORM_UNICODE = 0,
    TTF_PLATEFORM_MACINTOSH = 1,
    TTF_PLATEFORM_ISO = 2, // deprecated
    TTF_PLATEFORM_MICROSOFT = 3,
    TTF_PLATEFORM_CUSTOM = 4,
} TtfCmapPlatforms;

typedef enum
{
    TTF_ENC_UNICODE_1_0 = 0,      // deprecated
    TTF_ENC_UNICODE_1_1 = 1,      // deprecated
    TTF_ENC_UNICODE_ISO10646 = 2, // deprecated
    TTF_ENC_UNICODE_2_0_BMP = 3,
    TTF_ENC_UNICODE_2_0_FULL = 4,
    TTF_ENC_UNICODE_VARIANT = 5,
    TTF_ENC_UNICODE_FULL = 6,
} TtfCmapUnicodeEncodings;

typedef enum
{
    TTF_ENC_ISO_7BIT = 0,
    TTF_ENC_ISO_10646 = 1,
    TTF_ENC_ISO_8859_1 = 2,
} TtfCmapIsoEncodings;

typedef enum
{
    TTF_CMAP_FMT_BYTE_ENCODING = 0,
    TTF_CMAP_FMT_HIGH_BYTE_MAPPING_THROUGH = 2,
    TTF_CMAP_FMT_SEGMENT_MAPPING_TO_DELTA = 4,
    TTF_CMAP_FMT_TRIMMED_TABLE = 6,
    TTF_CMAP_FMT_MIXED_16_32_BIT = 8,
    TTF_CMAP_FMT_TRIMMED_ARRAY = 10,
    TTF_CMAP_FMT_SEGMENTED_COVERAGE = 12,
} TtfCmapSubtableFormats;

typedef enum
{
    TTF_ENC_MS_SYMBOL = 0,
    TTF_ENC_MS_UNICODE_BMP = 1,
    TTF_ENC_MS_SHIFTJIS = 2,
    TTF_ENC_MS_PRC = 3,
    TTF_ENC_MS_BIG5 = 4,
    TTF_ENC_MS_WANSUNG = 5,
    TTF_ENC_MS_JOHAB = 6,
    TTF_ENC_MS_UNICODE_FULL = 10,
} TtfCmapMicrosoftEncodings;

typedef struct PACKED
{
    be_int16_t plateform;
    be_int16_t encoding;
    be_int32_t offset;
} TtfCmapSubtable;

typedef struct PACKED
{
    be_int16_t version;
    be_int16_t num_tables;
    TtfCmapSubtable table[];
} TtfCmap;

typedef struct PACKED
{
    // TODO
} TtfSimpleGlyph;

typedef struct
{
    be_uint16_t contours_count;
    be_uint16_t x_min;
    be_uint16_t y_min;
    be_uint16_t x_max;
    be_uint16_t y_max;
    TtfSimpleGlyph simple_glyph[];
} TtfGlyph;

typedef enum
{
    TTF_TAG_NONE,
    TTF_TAG_HEAD_VAL,
    TTF_TAG_CVT_VAL,
    TTF_TAG_PREP,
    TTF_TAG_KERN,
    TTF_TAG_HHEA,
    TTF_TAG_POST,
    TTF_TAG_OS2,
    TTF_TAG_NAME,
    TTF_TAG_MAXP,
    TTF_TAG_GLYF,
    TTF_TAG_FPGM,
    TTF_TAG_CMAP,
    TTF_TAG_COUNT,
} TtfTags;

typedef struct
{
    TtfHeader *header;
    Buf data;
    TtfCmap *cmap;
    TtfCmapSubtable selected_cmap;
    TtfHead head;
    TtfSubtable kern;

} TtfFont;

TtfTags get_ttf_tag_from_value(uint32_t val);

Str get_ttf_tag_str(TtfTags tag);

TtfFont ttf_init(IoReader reader, Alloc *alloc);

void ttf_draw(int x, int y, Str text, TtfFont *font, Buf *buf);

