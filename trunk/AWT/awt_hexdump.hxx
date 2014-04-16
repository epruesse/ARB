// ================================================================= //
//                                                                   //
//   File      : awt_hexdump.hxx                                     //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2011   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AWT_HEXDUMP_HXX
#define AWT_HEXDUMP_HXX

#ifndef AWT_HXX
#include "awt.hxx"
#endif

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef ARB_STRBUF_H
#include <arb_strbuf.h>
#endif
#ifndef _GLIBCXX_CCTYPE
#include <cctype>
#endif

// -------------------
//      hex dumper

inline char nibble2hex(unsigned char c) { return "0123456789ABCDEF"[c&0x0f]; }
inline void dump_hexbyte(GBS_strstruct& buf, unsigned char c) { buf.put(nibble2hex(c>>4)); buf.put(nibble2hex(c)); }

class MemDump {
    bool show_offset;   // prefix every line with position info ?
    bool hex;           // dump hex ?
    bool ascii;         // dump ascii ?

    size_t width;       // > 0   -> wrap lines every XXX positions
    size_t separate;    // > 0   -> separate by one additional space after every XXX bytes
    bool   space;       // true -> space hex bytes

    bool is_separate_position(size_t pos) const { return separate && pos && !(pos%separate); }

    void dump_sep(GBS_strstruct& buf) const { buf.cat(" | "); }
    void dump_offset(GBS_strstruct& buf, size_t off) const {
        if (show_offset) {
            dump_hexbyte(buf, off>>8);
            dump_hexbyte(buf, off&0xff);
            if (hex||ascii) dump_sep(buf);
        }
    }
    void dump_hex(GBS_strstruct& buf, const char *mem, size_t off, size_t count, bool padded) const {
        size_t i;
        for (i = 0; i<count; ++i) {
            if (is_separate_position(i)) buf.put(' ');
            dump_hexbyte(buf, mem[off+i]);
            if (space) buf.put(' ');
        }
        if (padded) {
            for (; i<width; ++i) {
                buf.nput(' ', 3+is_separate_position(i));
            }
        }
        buf.cut_tail(space);
    }
    void dump_ascii(GBS_strstruct& buf, const char *mem, size_t off, size_t count) const {
        for (size_t i = 0; i<count; ++i) {
            if (is_separate_position(i)) buf.put(' ');
            buf.put(isprint(mem[off+i]) ? mem[off+i] : '.');
        }
    }
    void dump_line(GBS_strstruct& buf, const char *mem, size_t off, size_t count) const {
        dump_offset(buf, off);
        if (hex) {
            dump_hex(buf, mem, off, count, ascii);
            if (ascii) dump_sep(buf);
        }
        if (ascii) dump_ascii(buf, mem, off, count);
        buf.put('\n');
    }
    void dump_wrapped(GBS_strstruct& buf, const char *mem, size_t size) const {
        awt_assert(wrapped());
        size_t off = 0;
        while (size) {
            size_t count = size<width ? size : width;
            dump_line(buf, mem, off, count);
            size -= count;
            off  += count;
        }
    }

public:

    MemDump(bool show_offset_, bool hex_, bool ascii_, size_t  width_ = 0, size_t  separate_ = 0, bool space_ = true)
        : show_offset(show_offset_),
          hex(hex_),
          ascii(ascii_),
          width(width_),
          separate(separate_), 
          space(space_)
    {}

    bool wrapped() const { return width; }
    
    size_t mem_needed_for_dump(size_t bytes) const {
        size_t sections = show_offset+hex+ascii;

        if (!sections) return 1;

        size_t perByte      = hex*3+ascii;
        size_t extraPerLine = (sections-1)*3+1;
        size_t lines        = width ? bytes/width+1 : 1;

        return bytes*perByte + lines*extraPerLine + 50;
    }

    void dump_to(GBS_strstruct& buf, const char *mem, size_t size) const {
        if (size) {
            if (wrapped()) dump_wrapped(buf, mem, size);
            else { // one-line dump
                MemDump mod(*this);
                mod.width = size;
                mod.dump_wrapped(buf, mem, size);
            }
        }
    }
};



#else
#error awt_hexdump.hxx included twice
#endif // AWT_HEXDUMP_HXX
