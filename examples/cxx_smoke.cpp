#include <new>
#include <unistd.h>

struct Widget {
    int value;
};

extern "C" int norx_cxx_smoke()
{
    return 7;
}

extern "C" int main()
{
    static const char marker[] = "[   OK   ] userspace: cxx-write\n";
    alignas(Widget) unsigned char storage[sizeof(Widget)] = {};
    Widget *widget = new (storage) Widget{norx_cxx_smoke()};
    if (widget->value != 7) {
        return 1;
    }
    return write(1, marker, sizeof(marker) - 1) == (ssize_t)(sizeof(marker) - 1) ? 0 : 2;
}
