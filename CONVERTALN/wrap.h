#ifndef WRAP_H
#define WRAP_H

class WrapMode : virtual Noncopyable {
    char *separators;

    bool allowed_to_wrap() const { return separators; }
    const char *get_seps() const { ca_assert(allowed_to_wrap()); return separators; }
    int wrap_pos(const char *str, int wrapCol) const;

    const char *print_return_wrapped(Writer& write, const char * const content, const int len, const int rest_width) const;

public:
    WrapMode(const char *separators_) : separators(nulldup(separators_)) {}
    WrapMode(bool allowWrap) : separators(allowWrap ? strdup(WORD_SEP) : NULL) {} // true->wrap words, false->wrapping forbidden
    ~WrapMode() { free(separators); }

    void print(Writer& write, const char *first_prefix, const char *other_prefix, const char *content, int max_width) const;
};

#else
#error wrap.h included twice
#endif // WRAP_H
