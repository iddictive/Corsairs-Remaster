/* Native bundle entry point. Keep scripts in Resources and sign the full app. */
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char executable[PATH_MAX], resolved[PATH_MAX], python[PATH_MAX], script[PATH_MAX];
    uint32_t size = sizeof(executable);
    if (_NSGetExecutablePath(executable, &size) || !realpath(executable, resolved)) {
        perror("Cannot resolve Corsairs application"); return 1;
    }
    char *slash = strrchr(resolved, '/');
    if (!slash) return 1;
    *slash = '\0';
    slash = strrchr(resolved, '/');
    if (!slash) return 1;
    *slash = '\0'; /* Contents */
    if (snprintf(python, sizeof(python), "%s/Frameworks/Python.framework/Versions/3.14/bin/python3.14", resolved) >= sizeof(python) ||
        snprintf(script, sizeof(script), "%s/Resources/public_launcher.py", resolved) >= sizeof(script)) return 1;
    char **arguments = calloc((size_t)argc + 4, sizeof(*arguments));
    if (!arguments) return 1;
    arguments[0] = python; arguments[1] = "-I"; arguments[2] = "-B"; arguments[3] = script;
    for (int i = 1; i < argc; ++i) arguments[i + 3] = argv[i];
    setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1);
    execv(python, arguments);
    perror("Cannot start bundled Corsairs launcher");
    free(arguments);
    return 1;
}
