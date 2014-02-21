
#pragma once

#ifndef CB_H
#include "cb.h"
#endif
#include <list>
#include <vector>

class AW_window;
typedef struct _GtkWidget GtkWidget;

/**
 * Class handlind signal emission.
 * WindowCallback and RootCallback objects are stored internally
 * and executed on "emit()". Order of execution is 
 * first registered, last executed. 
 */
class AW_signal {
private:
    struct Pimpl;
    Pimpl *prvt;

    void clean_slots();
public:
    AW_signal();
    virtual ~AW_signal();
    AW_signal(const AW_signal&);
    AW_signal& operator=(const AW_signal&);
    AW_signal& operator+=(const AW_signal&);

    bool operator==(const AW_signal&) const;
    bool unordered_equal(const AW_signal&) const;
    
    size_t size() const;

    void connect(const WindowCallback& wcb, AW_window* aww);
    void connect(const RootCallback& rcb);
    void disconnect(const WindowCallback& wcb, AW_window* aww);
    void disconnect(const RootCallback& rcb);

    void bind(GtkWidget*, const char* signal);
    void unbind(GtkWidget*, const char* signal);
   
    void set_enabled(bool);

    void emit();
    void clear();

    AW_window* get_last_window() const;
};





