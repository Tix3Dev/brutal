#pragma once
#include <brutal/ds/vec.h>

typedef Vec(Str) VecPath;

typedef struct
{
    Str path;
    Str opath;
    int last_modified_time;
    VecPath hfiles;
} BruildCFiles;
