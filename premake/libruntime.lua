project "libruntime"
    kind "StaticLib"
    flags { "ExtraWarnings", "FatalWarnings", "NoExceptions" }
    files { "../source/runtime/*.cpp" }
    includedirs { "../include" }
