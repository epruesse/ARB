
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

protected:
    virtual bool pre_emit() { return true; }
    virtual void post_emit() {}

public:
    AW_signal();
    virtual ~AW_signal();
    AW_signal(const AW_signal&);
    AW_signal& operator=(const AW_signal&);

    size_t size() const;
    void connect(const WindowCallback& wcb, AW_window* aww);
    void connect(const RootCallback& rcb);
    void bind(GtkWidget*, const char* signal);
    void unbind(GtkWidget*, const char* signal);
   
    void emit();
    void clear();
};





