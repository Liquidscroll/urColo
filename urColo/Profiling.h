#pragma once
#include <atomic>
namespace uc {
inline std::atomic<int> g_fromImVec4Count{0};
inline std::atomic<int> g_toImVec4Count{0};
}
#define PROFILE_FROM_IMVEC4() (++uc::g_fromImVec4Count)
#define PROFILE_TO_IMVEC4() (++uc::g_toImVec4Count)
