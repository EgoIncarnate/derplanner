solution "derplanner"
    platforms { "x32", "x64" }
    configurations { "release", "debug" }
    language "C++"

    flags
    {
        "NoPCH",
        "NoRTTI",
        "ExtraWarnings",
        "FatalWarnings",
    }

    configuration "debug"
        flags "Symbols"

    configuration "release"
        flags "OptimizeSpeed"

    configuration { "debug", "x32" }
        targetdir "bin/x32/debug"

    configuration { "release", "x32" }
        targetdir "bin/x32/release"

    configuration { "debug", "x64" }
        targetdir "bin/x64/debug"

    configuration { "release", "x64" }
        targetdir "bin/x64/release"

    dofile "premake/unittestpp.lua"
    dofile "premake/tests.lua"

    objdir ".build/obj"
