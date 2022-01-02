#include <brutal/alloc.h>
#include <brutal/debug.h>
#include <brutal/ds.h>
#include <brutal/io.h>
#include <embed/file.h>
#include <json/parser.h>
#include "bruild.h"
#include "package.h"
#include "parse.h"

Vec(BruildPackage) packages_queue;

typedef struct
{
    Alloc *alloc;
    Str cur;
    VecPath *cfiles;
} ParseBruildCtx;

bool is_c_file(Str path)
{
    int p = str_last_chr(path, '.');

    if (p == -1 || p == 0)
    {
        return false;
    }

    Str ext = str_sub(path, p, path.len);

    return str_eq(ext, str$(".c"));
}

Iter parse_bruild_cfiles(EmbedDirInfo *info, ParseBruildCtx *ctx)
{
    /* skip every '.directory/' '.' and '..' */
    if (str_first_chr(info->name, '.') == 0)
    {
        return ITER_CONTINUE;
    }

    if (info->type == EMBED_DIR_DIRECTORY)
    {

        Str full_dir_path = str_fmt(ctx->alloc, "{}{}/", ctx->cur, info->name);

        EmbedDir dir = nullptr;
        if (embed_dir_open(&dir, full_dir_path, ctx->alloc).succ == false)
        {
            log$("can't use: {}", full_dir_path);
            return ITER_CONTINUE;
        }

        ParseBruildCtx new_ctx = *ctx;
        new_ctx.cur = full_dir_path;
        embed_dir_iter(dir, (IterFn *)parse_bruild_cfiles, &new_ctx);
        embed_dir_close(&dir);
    }
    else
    {
        if (is_c_file(info->name))
        {
            log$("in dir founded: {}/{}", ctx->cur, info->name);
            vec_push(ctx->cfiles, str_fmt(ctx->alloc, ctx->cur, info->name));
        }
    }
    return ITER_CONTINUE;
}

void package_get_cfiles(BruildPackage *package, Str package_path, Alloc *alloc)
{

    VecPath cfiles;
    vec_init(&cfiles, alloc);
    ParseBruildCtx ctx = {
        .alloc = alloc,
        .cfiles = &cfiles,
        .cur = str_dup(package_path, alloc),
    };
    EmbedDir dir;
    embed_dir_open(&dir, package_path, alloc);
    embed_dir_iter(dir, (IterFn *)parse_bruild_cfiles, &ctx);
    embed_dir_close(&dir);

    package->files = cfiles;
}

bool is_package_already_in_queue(Str name)
{
    vec_foreach_v(pack, &packages_queue)
    {
        if (str_eq(pack.name, name))
        {
            return true;
        }
    }
    return false;
}

bool parse_bruild_package(Str name, BruildPackage *target, Alloc *alloc)
{
    if (is_package_already_in_queue(name))
    {
        return false;
    }

    Str package_path = package_path_from_name(name, alloc);
    package_init(target, name, package_path, alloc);

    parse_bruild_json(target, alloc);
    vec_foreach_v(dep, &target->dependencies)
    {
        BruildPackage dep_pack;
        if (parse_bruild_package(dep, &dep_pack, alloc))
        {
            vec_push(&packages_queue, dep_pack);
        }
    }

    package_get_cfiles(target, package_path, alloc);

    return true;
}

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        log$("usage: bruild [target]");
        return 1;
    }
    HeapAlloc heap;
    heap_alloc_init(&heap, NODE_DEFAULT);
    vec_init(&packages_queue, base$(&heap));
    BruildPackage root_package;
    parse_bruild_package(str$(argv[1]), &root_package, base$(&heap));

    return 0;
}
