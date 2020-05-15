#include "wren.hpp"
#include "zegdb.hpp"


const char *foreignClassesString();
WrenForeignClassMethods bindForeignClass(WrenVM*, const char* module, const char* classname);
WrenForeignMethodFn bindForeignMethod( 
    WrenVM* vm, 
    const char* module, 
    const char* className, 
    bool isStatic, 
    const char* signature);