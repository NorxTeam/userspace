#include <stddef.h>
#include <string.h>

void *memcpy(void *destination, const void *source, size_t length)
{
    unsigned char *out = destination;
    const unsigned char *in = source;
    for (size_t index = 0; index < length; ++index) {
        out[index] = in[index];
    }
    return destination;
}

void *memmove(void *destination, const void *source, size_t length)
{
    unsigned char *out = destination;
    const unsigned char *in = source;
    if (out < in) {
        for (size_t index = 0; index < length; ++index) {
            out[index] = in[index];
        }
    } else if (out > in) {
        for (size_t index = length; index != 0; --index) {
            out[index - 1] = in[index - 1];
        }
    }
    return destination;
}

void *memset(void *destination, int value, size_t length)
{
    unsigned char *out = destination;
    for (size_t index = 0; index < length; ++index) {
        out[index] = (unsigned char)value;
    }
    return destination;
}

int memcmp(const void *left, const void *right, size_t length)
{
    const unsigned char *a = left;
    const unsigned char *b = right;
    for (size_t index = 0; index < length; ++index) {
        if (a[index] != b[index]) {
            return a[index] < b[index] ? -1 : 1;
        }
    }
    return 0;
}

size_t strlen(const char *string)
{
    size_t length = 0;
    while (string[length] != '\0') {
        ++length;
    }
    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t length)
{
    for (size_t index = 0; index < length; ++index) {
        if (left[index] != right[index] || left[index] == '\0') {
            return (unsigned char)left[index] - (unsigned char)right[index];
        }
    }
    return 0;
}

char *strchr(const char *string, int character)
{
    while (*string != '\0') {
        if ((unsigned char)*string == (unsigned char)character) {
            return (char *)string;
        }
        ++string;
    }
    return character == 0 ? (char *)string : (char *)0;
}
