// urColo - macros to suppress warnings around third-party headers
#pragma once

// Helper macros to temporarily disable warnings around third-party headers.
// portable-file-dialogs triggers a few warnings on each compiler.
#if defined(__clang__)
// -Wundef: portable-file-dialogs uses undefined platform macros.
// -Wshadow: some variables shadow others in the header.
#define UC_SUPPRESS_WARNINGS_BEGIN \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wundef\"") \
    _Pragma("clang diagnostic ignored \"-Wshadow\"")
#define UC_SUPPRESS_WARNINGS_END _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
// -Wundef: portable-file-dialogs uses undefined platform macros.
// -Wshadow: some variables shadow others in the header.
#define UC_SUPPRESS_WARNINGS_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wundef\"") \
    _Pragma("GCC diagnostic ignored \"-Wshadow\"")
#define UC_SUPPRESS_WARNINGS_END _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
// 4668: 'identifier' is not defined as a preprocessor macro.
// 4456/4457/4458: declaration of 'x' hides a previous local.
#define UC_SUPPRESS_WARNINGS_BEGIN \
    __pragma(warning(push)) \
    __pragma(warning(disable : 4668 4456 4457 4458))
#define UC_SUPPRESS_WARNINGS_END __pragma(warning(pop))
#else
#define UC_SUPPRESS_WARNINGS_BEGIN
#define UC_SUPPRESS_WARNINGS_END
#endif
