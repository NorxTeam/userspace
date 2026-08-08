#include <new>

struct Widget {
    int value;
};

extern "C" int norx_cxx_smoke()
{
    return 7;
}

extern "C" int main()
{
    alignas(Widget) unsigned char storage[sizeof(Widget)] = {};
    Widget *widget = new (storage) Widget{norx_cxx_smoke()};
    return widget->value == 7 ? 0 : 1;
}
