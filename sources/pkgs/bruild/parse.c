#include "parse.h"
#include <brutal/debug.h>
#include <brutal/io.h>

static void parse_bruild_deps(BruildPackage *package)
{
    Json json = package->json_src;
    Json pack_deps = json_get(json, str$("dependencies"));

    assert_truth(json_is(pack_deps, JSON_ARRAY));

    vec_foreach_v(dep, &pack_deps.array)
    {
        assert_truth(json_is(dep, JSON_STRING));

        vec_push(&package->dependencies, dep.string);
    }
}

void parse_bruild_json(BruildPackage *package, Alloc *alloc)
{
    Json json = package->json_src;

    Json pack_name = json_get(json, str$("target"));
    assert_truth(json_is(pack_name, JSON_STRING));
    package->name = str_dup(pack_name.string, alloc);

    parse_bruild_deps(package);
}
