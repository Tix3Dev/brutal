#pragma once
#include <json/parser.h>
#include "bruild.h"
typedef struct
{
    Json json_src;
    Str name;
    Str src_path;
    Str bruild_file;
    VecPath files;
    Vec(Str) dependencies;
} BruildPackage;

Str package_path_from_name(Str name, Alloc *alloc);

void package_init(BruildPackage *package, Str name, Str path, Alloc *alloc);
