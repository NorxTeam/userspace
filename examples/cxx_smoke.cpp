#include <new>
#include <stdlib.h>
#include <unistd.h>

struct Widget {
    int value;
};

struct ExitProbe {
    ~ExitProbe()
    {
        static const char marker[] = "[   OK   ] userspace: cxx-destructor\n";
        (void)write(1, marker, sizeof(marker) - 1);
    }
};

static ExitProbe &cxx_exit_probe()
{
    static ExitProbe probe;
    return probe;
}

static void c_atexit_probe()
{
    static const char marker[] = "[   OK   ] userspace: c-atexit\n";
    (void)write(1, marker, sizeof(marker) - 1);
}

static int cxa_probe_hits;
static int cxa_probe_token;

static void cxa_probe(void *argument)
{
    if (argument == &cxa_probe_token) {
        ++cxa_probe_hits;
    }
}

extern "C" int __cxa_atexit(void (*function)(void *), void *argument, void *dso);
extern "C" void __cxa_finalize(void *dso);

extern "C" int norx_cxx_smoke()
{
    return 7;
}

static int &cxx_guarded_value()
{
    static int value = norx_cxx_smoke();
    return value;
}

extern "C" int main()
{
    static const char marker[] = "[   OK   ] userspace: cxx-write\n";
    static int other_dso;
    (void)cxx_exit_probe();
    alignas(Widget) unsigned char storage[sizeof(Widget)] = {};
    Widget *widget = new (storage) Widget{norx_cxx_smoke()};
    if (widget->value != 7 || cxx_guarded_value() != 7 || cxx_guarded_value() != 7) {
        return 1;
    }
    if (atexit(c_atexit_probe) != 0
        || __cxa_atexit(cxa_probe, &cxa_probe_token, &cxa_probe_token) != 0) {
        return 2;
    }
    __cxa_finalize(&other_dso);
    if (cxa_probe_hits != 0) {
        return 3;
    }
    __cxa_finalize(&cxa_probe_token);
    if (cxa_probe_hits != 1) {
        return 4;
    }
    __cxa_finalize(&cxa_probe_token);
    if (cxa_probe_hits != 1) {
        return 5;
    }
    return write(1, marker, sizeof(marker) - 1) == (ssize_t)(sizeof(marker) - 1) ? 0 : 6;
}
