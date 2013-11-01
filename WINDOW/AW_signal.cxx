#include "aw_signal.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"

#include <functional> // required by osx
#include <algorithm>

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
    virtual AW_window* get_window() const { return NULL; }
    virtual Slot* clone() const = 0;
    virtual ~Slot() {}
    virtual bool operator==(const Slot&) const = 0;
    virtual bool operator<(const Slot&) const = 0;
    virtual int slot_type() const = 0;
};

/** Slot carrying a WindowCallback */
struct WindowCallbackSlot : public Slot {
    WindowCallback cb;
    AW_window* aww;

    WindowCallbackSlot(const WindowCallback& wcb, AW_window* w)
        : cb(wcb), aww(w) {}
    ~WindowCallbackSlot() {}

    int slot_type() const OVERRIDE {
        return 1;
    }

    bool operator<(const Slot& o) const OVERRIDE {
        if (slot_type() != o.slot_type()) {
            return slot_type() < o.slot_type();
        }
        else {
            return aww < ((WindowCallbackSlot&)o).aww 
                && cb  < ((WindowCallbackSlot&)o).cb;
        }
    }

    bool operator==(const Slot& o) const OVERRIDE {
        return slot_type() == o.slot_type() 
            && aww == ((WindowCallbackSlot&)o).aww
            && cb  == ((WindowCallbackSlot&)o).cb;
    }

    Slot* clone() const OVERRIDE { 
        return new WindowCallbackSlot(*this); 
    }

    void emit() OVERRIDE {
        cb(aww);
    }

    AW_window* get_window() const OVERRIDE {
        return aww;
    }
};

/** Slot carrying a RootCallback */
struct RootCallbackSlot : public Slot {
    RootCallback cb;

    RootCallbackSlot(const RootCallback& rcb) : cb(rcb) {}
    ~RootCallbackSlot() {}

    int slot_type() const OVERRIDE {
        return 2;
    }

    bool operator<(const Slot& o) const OVERRIDE {
        if (slot_type() != o.slot_type()) {
            return slot_type() < o.slot_type();
        }
        else {
            return cb  < ((RootCallbackSlot&)o).cb;
        }
    }

    bool operator==(const Slot& o) const OVERRIDE {
        return slot_type() == o.slot_type() 
            && cb == ((RootCallbackSlot&)o).cb;
    }
    
    Slot* clone() const OVERRIDE { 
        return new RootCallbackSlot(*this); 
    }

    void emit() OVERRIDE {
        cb(AW_root::SINGLETON);
    }
};

struct AW_signal::Pimpl {
    std::list<Slot*> slots;
    bool enabled;            // false -> no signal propagation
    AW_window* last_window;
    Pimpl() : enabled(true) {}
    ~Pimpl() {
        for (std::list<Slot*>::iterator it = slots.begin();
             it != slots.end(); ++it) {
            delete (*it);
        }
    }
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

    return operator+=(o);
}

/** Add-to Operator */
AW_signal& AW_signal::operator+=(const AW_signal& o) {
    for (std::list<Slot*>::iterator it = o.prvt->slots.begin();
         it != o.prvt->slots.end(); ++it) {
        prvt->slots.push_back((*it)->clone());
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

template<class T> 
struct dereference : public T {
    typedef typename T::first_argument_type* first_argument_type;
    typedef typename T::second_argument_type* second_argument_type;
    typedef typename T::result_type result_type;

    result_type operator()(const first_argument_type& f, 
                           const second_argument_type& s) const {
        return T::operator()(*f, *s);
    }
};

/** Equality Comparator */
bool AW_signal::operator==(const AW_signal& o) const {
    if (size() != o.size()) return false;
    return std::equal(prvt->slots.begin(), prvt->slots.end(), o.prvt->slots.begin(),
                      dereference<std::equal_to<Slot> >());
}


/** Compare Signals ignoring signal order */
bool AW_signal::unordered_equal(const AW_signal& o) const {
    if (size() != o.size()) return false;

    // order signals
    std::list<Slot*> l(prvt->slots), r(o.prvt->slots);
    l.sort(dereference<std::less<Slot> >());
    r.sort(dereference<std::less<Slot> >());

    return std::equal(l.begin(), l.end(), r.begin(),
                      dereference<std::equal_to<Slot> >());
}

/** Returns the number of connected callbacks */
size_t AW_signal::size() const {
    return prvt->slots.size();
}

/** Connects the Signal to the supplied WindowCallback and Window */
void AW_signal::connect(const WindowCallback& wcb, AW_window* aww) {
    aw_return_if_fail(aww != NULL);
#ifdef DEBUG
    WindowCallbackSlot wcbs(wcb, aww);
    if (std::find_if(prvt->slots.begin(), prvt->slots.end(), 
                     bind1st(dereference<std::equal_to<Slot> >(), &wcbs)) 
        != prvt->slots.end()) {
        aw_warning("duplicate signal!");
    }
#endif
    prvt->slots.push_back(new WindowCallbackSlot(wcb, aww));
}

/** Connects the Signal to the supplied RootCallback */
void AW_signal::connect(const RootCallback& rcb) {
#ifdef DEBUG
    RootCallbackSlot rcbs(rcb);
    if (std::find_if(prvt->slots.begin(), prvt->slots.end(), 
                     bind1st(dereference<std::equal_to<Slot> >(), &rcbs)) 
        != prvt->slots.end()) {
        aw_warning("duplicate signal!");
    }
#endif
    prvt->slots.push_back(new RootCallbackSlot(rcb));
}

/** Disconnects the Signal from the supplied RCB */
void AW_signal::disconnect(const RootCallback& rcb) {
    RootCallbackSlot r(rcb);
    prvt->slots.remove_if(bind1st(dereference<std::equal_to<Slot> >(), &r));
}

/** Disconnects the Signal from the supplied RCB */
void AW_signal::disconnect(const WindowCallback& wcb, AW_window* aww) {
    WindowCallbackSlot w(wcb, aww);
    prvt->slots.remove_if(bind1st(dereference<std::equal_to<Slot> >(), &w));
}



/** Emits the signal (i.e. runs all connected callbacks) */
void AW_signal::emit() {
    if (!prvt->enabled) return;

    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        (*it)->emit();
        if ((*it)->get_window()) 
            prvt->last_window = (*it)->get_window();
    }
}

AW_window* AW_signal::get_last_window() const {
    return prvt->last_window;
}

/** Disconnects all callbacks from signal */
void AW_signal::clear() {
    delete prvt;
    prvt = new Pimpl;
}


#ifdef UNIT_TESTS
#include <test_unit.h>

int wcb1_count = 0;
int wcb2_count = 0;
int rcb1_count = 0;
int rcb2_count = 0;

static void wcb1(AW_window* w, const char *test) {
    TEST_EXPECT(w == (AW_window*)123);
    TEST_EXPECT(test == (const char*)456);
    wcb1_count ++;
}

static void wcb2(AW_window* w, const char *test) {
    TEST_EXPECT(w == (AW_window*)123);
    TEST_EXPECT(test == (const char*)456);
    wcb2_count ++;
}

static void rcb1(AW_root*, const char *) {
    rcb1_count ++;
}

static void rcb2(AW_root*, const char *) {
    rcb2_count ++;
}

void TEST_AW_signal_emit() {
    wcb1_count = 0;
    rcb1_count = 0;
    AW_signal sig;

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
    
    // Add some cbs, remove and emit
    sig.connect(makeRootCallback(rcb1, (const char*)135));
    sig.connect(makeRootCallback(rcb2, (const char*)135));
    sig.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    sig.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)456);
    sig.disconnect(makeRootCallback(rcb2, (const char*)135));
    sig.disconnect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)456);
    sig.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 3);
    TEST_EXPECT_EQUAL(rcb1_count, 2);
}

void TEST_AW_signal_equal() {
    wcb1_count = 0;
    rcb1_count = 0;
    AW_signal sig1, sig2;

    // Empty signals should be equal
    TEST_EXPECT_EQUAL(sig1 == sig2, true);

    // Add WCB to one, should be unequal
    sig1.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    TEST_EXPECT_EQUAL(sig1 == sig2, false);

    // Add same WCB to other, should be equal
    sig2.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    TEST_EXPECT_EQUAL(sig1 == sig2, true);

    // different window, different signal
    sig2.clear();
    sig2.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)234);
    TEST_EXPECT_EQUAL(sig1 == sig2, false);
    
    // different data, different signal
    sig2.clear();
    sig2.connect(makeWindowCallback(wcb1, (const char*)654), (AW_window*)123);
    TEST_EXPECT_EQUAL(sig1 == sig2, false);

    // different cb, different signal
    sig2.clear();
    sig2.connect(makeWindowCallback(wcb2, (const char*)456), (AW_window*)123);
    TEST_EXPECT_EQUAL(sig1 == sig2, false);

    // Try same with RCB
    sig1.clear();
    sig2.clear();
    sig1.connect(makeRootCallback(rcb1, (const char*)456));
    TEST_EXPECT_EQUAL(sig1 == sig2, false);
    sig2.connect(makeRootCallback(rcb1, (const char*)456));
    TEST_EXPECT_EQUAL(sig1 == sig2, true);
    sig2.clear();
    sig2.connect(makeRootCallback(rcb1, (const char*)654));
    TEST_EXPECT_EQUAL(sig1 == sig2, false);
    sig2.clear();
    sig2.connect(makeRootCallback(rcb2, (const char*)456));
    TEST_EXPECT_EQUAL(sig1 == sig2, false);

    // RCB != WCB
    sig1.clear();
    sig1.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    TEST_EXPECT_EQUAL(sig1 == sig2, false);

    // Order dependence
    sig1.connect(makeRootCallback(rcb2, (const char*)456));
    sig2.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    TEST_EXPECT_EQUAL(sig1 == sig2, false);
    TEST_EXPECT_EQUAL(sig1.unordered_equal(sig2), true);
}

void TEST_AW_signal_assign() {
    wcb1_count = 0;
    rcb1_count = 0;
    AW_signal sig1, sig2;

    // test simple assign
    sig1.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    sig2 = sig1;
    sig2.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 1);
    TEST_EXPECT_EQUAL(rcb1_count, 0);

    // test assign removes old cb 
    sig1.clear();
    sig1.connect(makeRootCallback(rcb1, (const char*)135));
    sig2 = sig1;
    sig2.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 1);
    TEST_EXPECT_EQUAL(rcb1_count, 1);

    // test add-to assign
    sig1.clear();
    sig1.connect(makeWindowCallback(wcb1, (const char*)456), (AW_window*)123);
    sig2 += sig1;
    sig2.emit();
    TEST_EXPECT_EQUAL(wcb1_count, 2);
    TEST_EXPECT_EQUAL(rcb1_count, 2);

    // test duplicate assign?

    

}
#endif
