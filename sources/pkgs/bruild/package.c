#include "package.h"
#include <brutal/alloc.h>
#include <brutal/debug.h>
#include <brutal/io.h>

Buf bruild_get_file_content(Str path, Alloc *alloc)
{
    IoFile file;

    UNWRAP_OR_PANIC(io_file_open(&file, path), "File not found!");
    IoReader source_file_reader = io_file_reader(&file);

    Buf source_buf;
    buf_init(&source_buf, 512, alloc);

    IoWriter source_buf_writer = buf_writer(&source_buf);
    io_copy(&source_file_reader, &source_buf_writer);

    io_file_close(&file);

    return source_buf;
}

Str package_path_from_name(Str name, Alloc *alloc)
{
    Str name_res = str_dup(name, alloc);
    for (size_t i = 0; i < name_res.len; i++)
    {
        if (name_res.buf[i] == '.')
        {
            ((char *)name_res.buf)[i] = '/';
        }
    }

    return str_fmt(alloc, "./sources/{}/", name_res);
}

void package_init(BruildPackage *package, Str name, Str path, Alloc *alloc)
{
    *package = (BruildPackage){};
    vec_init(&package->files, alloc);
    vec_init(&package->dependencies, alloc);

    package->name = name;
    package->src_path = path;
    package->bruild_file = str_fmt(alloc, "{}/bruild.json", path);
    
    Buf source_buf = bruild_get_file_content(package->bruild_file, alloc);
    Scan scan;
    scan_init(&scan, buf_str(&source_buf));
    package->json_src = json_parse(&scan, alloc);
}
