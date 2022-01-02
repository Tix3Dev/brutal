#pragma once

#include <json/parser.h>
#include "bruild.h"
#include "package.h"

void parse_bruild_json(BruildPackage *package, Alloc *alloc);
