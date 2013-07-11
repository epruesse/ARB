
#include "cb.h"
#include <list>

class AW_window;

/**
 * Class handlind signal emission.
 * WindowCallback and RootCallback objects are stored internally
 * and executed on "emit()". Order of execution is 
 * first registered, last executed. 
 */
class AW_signal {
    /*
     * We have to use a class hierarchy because the callbacks have a
     * non trivial destructor (making the use of union impossible) and
     * no default constructor (making the use of struct impossible).
     */
    class Slot {
    public:
        Slot() {}
        virtual void emit() = 0 ;
        virtual ~Slot() {}
    };
    class WindowCallbackSlot : public Slot {
        WindowCallback cb;
        AW_window* aww;
    public:
        WindowCallbackSlot(const WindowCallback& wcb, AW_window* w)
            : cb(wcb), aww(w) {}
        ~WindowCallbackSlot() {}
        void emit();
    };
    class RootCallbackSlot : public Slot {
        RootCallback cb;
    public:
        RootCallbackSlot(const RootCallback& rcb)
        : cb(rcb) {}
        ~RootCallbackSlot() {}
        void emit();
    };

    std::list<Slot*> slots;
public:
    ~AW_signal();

    void connect(const WindowCallback& wcb, AW_window* aww);
    void connect(const RootCallback& rcb);
    void emit();
    void clear();
};
