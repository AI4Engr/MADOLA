#pragma once
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

namespace madola {
namespace utils {

inline std::string getHomeDirectory() {
#ifdef _WIN32
    // Windows: Use USERPROFILE or HOMEDRIVE+HOMEPATH
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        return std::string(userProfile);
    }

    const char* homeDrive = std::getenv("HOMEDRIVE");
    const char* homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath) {
        return std::string(homeDrive) + std::string(homePath);
    }

    return "C:\\";
#else
    // Unix/Linux/macOS: Use HOME environment variable or getpwuid
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home);
    }

    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return std::string(pw->pw_dir);
    }

    return "/tmp";
#endif
}

inline std::string getMadolaDirectory() {
    std::string home = getHomeDirectory();
    std::string madolaDir = home;

#ifdef _WIN32
    madolaDir += "\\.madola";
#else
    madolaDir += "/.madola";
#endif

    // Create directory if it doesn't exist
    std::filesystem::create_directories(madolaDir);

    return madolaDir;
}

inline std::string getGenCppDirectory() {
    std::string madolaDir = getMadolaDirectory();
    std::string genCppDir = madolaDir;

#ifdef _WIN32
    genCppDir += "\\gen_cpp";
#else
    genCppDir += "/gen_cpp";
#endif

    std::filesystem::create_directories(genCppDir);
    return genCppDir;
}

inline std::string getTroveDirectory(const std::string& moduleName = "") {
    std::string madolaDir = getMadolaDirectory();
    std::string troveDir = madolaDir;

#ifdef _WIN32
    troveDir += "\\trove";
    if (!moduleName.empty()) {
        troveDir += "\\" + moduleName;
    }
#else
    troveDir += "/trove";
    if (!moduleName.empty()) {
        troveDir += "/" + moduleName;
    }
#endif

    std::filesystem::create_directories(troveDir);
    return troveDir;
}

} // namespace utils
} // namespace madola
