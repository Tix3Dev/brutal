#include <brutal/debug.h>
#include <ttf/ttf.h>

const char *tag2str_val[] = {
    [TTF_TAG_NONE] = "none",
    [TTF_TAG_HEAD_VAL] = "head",
    [TTF_TAG_CVT_VAL] = "cvt ",
    [TTF_TAG_PREP] = "prep",
    [TTF_TAG_KERN] = "kern",
    [TTF_TAG_HHEA] = "hhea",
    [TTF_TAG_POST] = "post",
    [TTF_TAG_OS2] = "OS/2",
    [TTF_TAG_NAME] = "name",
    [TTF_TAG_MAXP] = "maxp",
    [TTF_TAG_GLYF] = "glyf",
    [TTF_TAG_FPGM] = "fpgm",
    [TTF_TAG_CMAP] = "cmap",
};

Str get_ttf_tag_str(TtfTags tag)
{
    if (tag < TTF_TAG_COUNT)
    {
        return str$(tag2str_val[tag]);
    }
    else
    {
        return str$("unkown ttf tag");
    }
}

TtfTags get_ttf_tag_from_value(uint32_t val)
{
    for (size_t i = 0; i < TTF_TAG_COUNT; i++)
    {
        if (val == *((uint32_t *)tag2str_val[i]))
        {
            return (TtfTags)i;
        }
    }
    return TTF_TAG_NONE;
}

static int ttf_get_cmap_score(uint16_t platform, uint16_t encoding)
{
    if ((platform == TTF_PLATEFORM_UNICODE && encoding == TTF_ENC_UNICODE_2_0_BMP) ||
        (platform == TTF_PLATEFORM_MICROSOFT && encoding == TTF_ENC_MS_UNICODE_BMP))
    {
        return 2;
    }

    log$("! may be unhandled cmap: {} {}", platform, encoding);

    return 0;
}

static TtfCmapSubtable ttf_find_cmap(TtfFont *self)
{
    size_t count = load_be(self->cmap->num_tables);
    TtfCmapSubtable selected = {};
    int selected_score = 0;
    for (size_t i = 0; i < count; i++)
    {
        int score = ttf_get_cmap_score(load_be(self->cmap->table[i].plateform), load_be(self->cmap->table[i].encoding));
        if (selected_score < score)
        {
            selected_score = score;
            selected = self->cmap->table[i];
        }
    }

    if (selected_score == 0)
    {
        log$("not found valid cmap");
        return (TtfCmapSubtable){};
    }

    return selected;
}

static void ttf_init_cmap(TtfFont *self)
{
    self->selected_cmap = ttf_find_cmap(self);
    if (load_be(self->selected_cmap.offset) == 0)
    {
        return;
    }

    TtfSubtable sub_table = *(TtfSubtable *)((void *)self->cmap + load_be(self->selected_cmap.offset));

    log$("ttf subtable info: {}", load_be(sub_table.format));
}
static void ttf_init_tables(TtfFont *self)
{
    log$("ttf dump:");

    log$(" - num table: {} ({})", (uint16_t)load_be(self->header->num_table), self->header->num_table._be_data);
    for (int i = 0; i < load_be(self->header->num_table); i++)
    {
        TtfTags tag = get_ttf_tag_from_value((self->header->dir_table[i].tag._be_data));
        if (load_be(self->header->dir_table[i].length) == 0 || tag == 0)
        {
            continue;
        }
        log$(" - table[{}]: ({}) {}", i, tag, get_ttf_tag_str(tag));
        log$(" - table[{}].offset:{} ", i, load_be(self->header->dir_table[i].offset));
        log$(" - table[{}].length:{} ", i, load_be(self->header->dir_table[i].length));
        switch (tag)
        {
        case TTF_TAG_CMAP:
            self->cmap = (TtfCmap *)(self->data.data + load_be(self->header->dir_table[i].offset));
            break;
        default:
            break;
        }
    }
}
TtfFont ttf_init(IoReader reader, Alloc *alloc)
{
    TtfFont self = {};
    Buf source_buf;
    buf_init(&source_buf, 512, alloc);
    io_copy(reader, buf_writer(&source_buf));
    log$("{}", source_buf.used);
    for (int i = 0; i < 16; i++)
    {
        log$("{}", source_buf.data[i]);
    }

    self.data = source_buf;
    self.header = (TtfHeader *)source_buf.data;
    ttf_init_tables(&self);
    ttf_init_cmap(&self);
    return self;
}
