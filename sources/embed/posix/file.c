#include <brutal/alloc.h>
#include <brutal/debug.h>
#include <brutal/text/utf8.h>
#include <dirent.h>
#include <embed/file.h>
#include <embed/posix/err.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

MaybeError embed_file_open(IoFile *self, Str path)
{
    char *cstr = (char *)utf8_str_to_cstr(path, alloc_global());

    self->embed.fd = open(cstr, O_RDONLY);

    alloc_free(alloc_global(), cstr);

    if (self->embed.fd == -1)
    {
        Error error = error_from_errno();
        return ERR(MaybeError, error);
    }

    return SUCCESS;
}

MaybeError embed_file_create(IoFile *self, Str path)
{
    char *cstr = (char *)utf8_str_to_cstr(path, alloc_global());

    self->embed.fd = open(cstr, O_CREAT | O_WRONLY | O_TRUNC, 0666);

    alloc_free(alloc_global(), cstr);

    if (self->embed.fd == -1)
    {
        Error error = error_from_errno();
        return ERR(MaybeError, error);
    }

    return SUCCESS;
}

MaybeError embed_file_close(IoFile *self)
{
    int result = close(self->embed.fd);

    if (result == -1)
    {
        Error error = error_from_errno();
        return ERR(MaybeError, error);
    }

    return SUCCESS;
}

int io_seek_to_whence(IoSeek seek)
{
    switch (seek.whence)
    {
    case IO_WHENCE_START:
        return SEEK_SET;
    case IO_WHENCE_CURRENT:
        return SEEK_CUR;
    case IO_WHENCE_END:
        return SEEK_END;
    default:
        panic$("Unknow whence {}", seek.whence);
    }
}

IoResult embed_file_seek(IoFile *self, IoSeek seek)
{
    ssize_t result = lseek(self->embed.fd, seek.position, io_seek_to_whence(seek));

    if (result == -1)
    {
        Error error = error_from_errno();
        return ERR(IoResult, error);
    }

    return OK(IoResult, result);
}

IoResult embed_file_read(IoFile *self, uint8_t *data, size_t size)
{
    ssize_t result = read(self->embed.fd, data, size);

    if (result == -1)
    {
        Error error = error_from_errno();
        return ERR(IoResult, error);
    }

    return OK(IoResult, result);
}

IoResult embed_file_write(IoFile *self, uint8_t const *data, size_t size)
{
    ssize_t result = write(self->embed.fd, data, size);

    if (result == -1)
    {
        Error error = error_from_errno();
        return ERR(IoResult, error);
    }

    return OK(IoResult, result);
}

Iter embed_dir_iter(EmbedDir self, IterFn fn, void *ctx)
{
    struct dirent *dir_ent = nullptr;
    while ((dir_ent = readdir((DIR *)self)))
    {
        EmbedDirInfo info = {
            .name = str$(dir_ent->d_name),
        };

        // we use a switch because we may need to support more than 3 dir type
        switch (dir_ent->d_type)
        {
        case DT_REG:
            info.type = EMBED_DIR_FILE;
            break;

        case DT_DIR:
            info.type = EMBED_DIR_DIRECTORY;
            break;

        default:
            info.type = EMBED_DIR_UNKNOWN;
            break;
        }

        if (fn(&info, ctx) == ITER_STOP)
        {
            return ITER_STOP;
        }
    }

    rewinddir((DIR *)self);

    return ITER_CONTINUE;
}

MaybeError embed_dir_open(EmbedDir *self, Str path, Alloc *alloc)
{

    char *path_cstr = (char *)str_to_cstr_utf8(path, alloc);
    *self = opendir((char *)path_cstr);
    alloc_free(alloc, path_cstr);

    if (*self == nullptr)
    {
        Error error = error_from_errno();
        return ERR(MaybeError, error);
    }

    return SUCCESS;
}

MaybeError embed_dir_close(EmbedDir *self)
{
    if (closedir(*self) == -1)
    {
        panic$("close dir error");
    }

    return SUCCESS;
}
