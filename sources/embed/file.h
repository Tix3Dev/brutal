#pragma once

#include <brutal/io/file.h>

MaybeError embed_file_open(IoFile *self, Str path);

MaybeError embed_file_create(IoFile *self, Str path);

MaybeError embed_file_open(IoFile *self, Str path);

MaybeError embed_file_close(IoFile *self);

IoResult embed_file_seek(IoFile *self, IoSeek pos);

IoResult embed_file_read(IoFile *self, uint8_t *data, size_t size);

IoResult embed_file_write(IoFile *self, uint8_t const *data, size_t size);

typedef enum
{
    EMBED_DIR_UNKNOWN,
    EMBED_DIR_FILE,
    EMBED_DIR_DIRECTORY,
} EmbedDirType;

typedef struct
{
    Str name;
    EmbedDirType type;
} EmbedDirInfo;

Iter embed_dir_iter(EmbedDir self, IterFn fn, void *ctx);

MaybeError embed_dir_open(EmbedDir *self, Str path, Alloc *alloc);

MaybeError embed_dir_close(EmbedDir *self);
