#include "aw_signal.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"


/**
 * Slot carrying the data for the individual callbacks.
 *
 * We have to use a class hierarchy because the callbacks have a
 * non trivial destructor (making the use of union impossible) and
 * no default constructor (making the use of struct impossible).
 */
struct Slot {
    Slot() {}
    virtual void emit() = 0 ;
    virtual Slot* clone() const = 0;
    virtual ~Slot() {}
};

/** Slot carrying a WindowCallback */
struct WindowCallbackSlot : public Slot {
    WindowCallback cb;
    AW_window* aww;

    WindowCallbackSlot(const WindowCallback& wcb, AW_window* w)
        : cb(wcb), aww(w) {}
    ~WindowCallbackSlot() {}
    void emit() {
        cb(aww);
    }

    Slot* clone() const { 
        return new WindowCallbackSlot(*this); 
    }
};

/** Slot carrying a RootCallback */
struct RootCallbackSlot : public Slot {
    RootCallback cb;

    RootCallbackSlot(const RootCallback& rcb) : cb(rcb) {}
    ~RootCallbackSlot() {}
    void emit() {
        cb(AW_root::SINGLETON);
    }
    
    Slot* clone() const { 
        return new RootCallbackSlot(*this); 
    }
};

struct AW_signal::Pimpl {
    std::list<Slot*> slots;
    bool enabled;            // false -> no signal propagation
    Pimpl() : enabled(true) {}
};

     
/** Default Constructor */
AW_signal::AW_signal() 
    : prvt(new Pimpl)
{
}

/** Copy Constructor */
AW_signal::AW_signal(const AW_signal& o) 
    : prvt(new Pimpl)
{
    *this = o;
}

/** Copy (assignment) Operator */
AW_signal& AW_signal::operator=(const AW_signal& o) {
    // copy the slots containing downstream callbacks
    prvt->slots.clear();
    for (std::list<Slot*>::iterator it = o.prvt->slots.begin();
         it != o.prvt->slots.end(); ++it) {
        prvt->slots.push_front((*it)->clone());
    }

    return *this;
}

/** Destructor */
AW_signal::~AW_signal() {
    // remove downstream signals
    clear();

    // remove list (upstream is handled by destructors)
    delete prvt;
}

/** Returns the number of connected callbacks */
size_t AW_signal::size() const {
    return prvt->slots.size();
}

/** Connects the Signal to the supplied WindowCallback and Window */
void AW_signal::connect(const WindowCallback& wcb, AW_window* aww) {
    aw_return_if_fail(aww != NULL);
    prvt->slots.push_front(new WindowCallbackSlot(wcb, aww));
}

/** Connects the Signal to the supplied RootCallback */
void AW_signal::connect(const RootCallback& rcb) {
    prvt->slots.push_front(new RootCallbackSlot(rcb));
}

/** Emits the signal (i.e. runs all connected callbacks) */
void AW_signal::emit() {
    if (!prvt->enabled) return;
    if (!pre_emit()) return;

    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        (*it)->emit();
    }

    post_emit();
}

/** Disconnects all callbacks from signal */
void AW_signal::clear() {
    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        delete (*it);
    }
    prvt->slots.clear();
}


#ifdef UNIT_TESTS
#include <test_unit.h>

AW_signal sig;

int wcb1_count = 0;
int rcb1_count = 0;

static void wcb1(AW_window* w, const char *test) {
    printf("in wcb1\n");
    TEST_EXPECT(w == (AW_window*)123);
    TEST_EXPECT(test == (const char*)456);
    wcb1_count ++;
}

static void rcb1(AW_root*, const char *) {
    rcb1_count ++;
}

void TEST_AW_signal() {
    // Empty signal should not call anything
    sig.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 0);
    TEST_EXPECT_EQUAL(rcb1_count, 0);

    // Test signal with one WindowCallback
    sig.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    sig.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 1);
    TEST_EXPECT_EQUAL(rcb1_count, 0);

    // Add a root callback and emit again
    sig.connect(makeRootCallback(rcb1, (const char*)135));
    sig.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 2);
    TEST_EXPECT_EQUAL(rcb1_count, 1);

    // Disconnect all and emit again
    sig.clear();
    sig.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 2);
    TEST_EXPECT_EQUAL(rcb1_count, 1);
}

#endif
