#include <fontconfig/fontconfig.h>

#include <windef.h>

FcConfig* __cdecl MaaFcInitLoadConfigAndFonts()
{
    return FcInitLoadConfigAndFonts();
}

FcPattern* __cdecl MaaFcNameParse(FcChar8* pattern)
{
    return FcNameParse(pattern);
}

FcBool __cdecl MaaFcConfigSubstitute(FcConfig* config, FcPattern* p, FcMatchKind kind)
{
    return FcConfigSubstitute(config, p, kind);
}

void __cdecl MaaFcDefaultSubstitute(FcPattern* p)
{
    FcDefaultSubstitute(p);
}

FcResult __cdecl MaaFcPatternGetString(FcPattern* p, const char* object, int n, FcChar8** s)
{
    return FcPatternGetString(p, object, n, s);
}

FcResult __cdecl MaaFcPatternGetCharSet(FcPattern* p, const char* object, int n, FcCharSet** c)
{
    return FcPatternGetCharSet(p, object, n, c);
}

FcFontSet* __cdecl MaaFcFontSort(FcConfig* config, FcPattern* p, FcBool trim, FcCharSet** csp, FcResult* result)
{
    return FcFontSort(config, p, trim, csp, result);
}

FcCharSet* __cdecl MaaFcCharSetCreate()
{
    return FcCharSetCreate();
}

FcCharSet* __cdecl MaaFcCharSetSubtract(FcCharSet* a, FcCharSet* b)
{
    return FcCharSetSubtract(a, b);
}

FcBool __cdecl MaaFcCharSetMerge(FcCharSet* a, FcCharSet* b, FcBool* c)
{
    return FcCharSetMerge(a, b, c);
}

FcChar32 __cdecl MaaFcCharSetFirstPage(FcCharSet* a, FcChar32* pagemap, FcChar32* len)
{
    return FcCharSetFirstPage(a, pagemap, len);
}

FcChar32 __cdecl MaaFcCharSetNextPage(FcCharSet* a, FcChar32* next, FcChar32* len)
{
    return FcCharSetNextPage(a, next, len);
}

void __cdecl MaaFcCharSetDestroy(FcCharSet* a)
{
    FcCharSetDestroy(a);
}

void __cdecl MaaFcPatternDestroy(FcPattern* p)
{
    FcPatternDestroy(p);
}

void __cdecl MaaFcConfigDestroy(FcConfig* config)
{
    FcConfigDestroy(config);
}

void __cdecl MaaFcFontSetDestroy(FcFontSet* fs)
{
    FcFontSetDestroy(fs);
}

int __cdecl MaaFcCharsetMapSize() {
    return FC_CHARSET_MAP_SIZE;
}
