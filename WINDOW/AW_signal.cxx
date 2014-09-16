#include "aw_signal.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"
#include <arb_msg.h>

#include <functional> // required by osx
#include <algorithm>

/**
 * AW_signal emission will recurse if a handler (slot, cb) emits the
 * signal again. Recursion is aborted at this depth.
 */
const int MAX_EMIT_RECURSION = 100;

/**
 * Slot carrying the data for the individual callbacks.
 **/
struct Slot {
    /* We have to use a class hierarchy because the callbacks have a
     * non trivial destructor (making the use of union impossible) and
     * no default constructor (making the use of struct impossible).
     */

    bool    deleted; //! this slot has been marked for deletion
    Slot() : deleted(false) {}

    virtual void emit() = 0 ;
    virtual AW_window* get_window() const { return NULL; }
    virtual Slot* clone() const = 0;
    virtual ~Slot() {}
    virtual bool operator==(const Slot&) const = 0;
    virtual bool operator<(const Slot&) const = 0;
    virtual int slot_type() const = 0;
};

/**
 * Slot carrying a WindowCallback 
 */
struct WindowCallbackSlot : public Slot {
    WindowCallback cb;
    AW_window* aww;

    WindowCallbackSlot(const WindowCallback& wcb, AW_window* w)
        : cb(wcb), aww(w) {}
    WindowCallbackSlot(const WindowCallbackSlot& other)
        : cb(other.cb), aww(other.aww) {}
    DECLARE_ASSIGNMENT_OPERATOR(WindowCallbackSlot);
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

/** 
 * Slot carrying a RootCallback 
 */
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
        aw_assert(!GB_have_error());
        cb(AW_root::SINGLETON);
        aw_assert(!GB_have_error());
    }
};

struct AW_signal::Pimpl : virtual Noncopyable {
    std::list<Slot*> slots;
    bool enabled;            // false -> no signal propagation
    int in_emit;             // we're in emit! don't delete from slots!
    bool deferred_destroy;   // destroy self after emit
    AW_window* last_window;
    Pimpl() : enabled(true), in_emit(0), deferred_destroy(0) {}

    void clean_slots();
    void clear();
    void destroy();
    void emit();
};

/** 
 * Remove Slots that where marked for deletion safely
 */
void AW_signal::Pimpl::clean_slots() {
    //  Only modify Slot-List if we're not in emit():
    if (in_emit > 0) return;
    
    std::list<Slot*>::iterator it = slots.begin();
    while (it != slots.end()) {
        if ((*it)->deleted) {
            delete *it;
            it = slots.erase(it);
        } 
        else {
            ++it;
        }
    }
}

/** Disconnects all callbacks from signal */
void AW_signal::clear() {
    if (prvt) prvt->clear();
}

void AW_signal::Pimpl::clear() {
    // mark slots for deletion
    for (std::list<Slot*>::iterator it = slots.begin();
         it != slots.end(); ++it) {
        (*it)->deleted = true;
    }
    
    // remove slots
    clean_slots();
}


/**
 * Destroy signal if not in emit, otherwise set flag to trigger
 * destruction once emit has exited.
 */
void AW_signal::Pimpl::destroy() {
    if (in_emit) {
        deferred_destroy = true;
    }
    else {
        clear();
        delete this;
    }
}
     
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

/** 
 * Copy (assignment) Operator 
 *
 * If we are within emit(), this is equivalent to abort emission
 * of the current signal and emit @param o!
 */
AW_signal& AW_signal::operator=(const AW_signal& o) {
    // copy the slots containing downstream callbacks
    clear();
    
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
    // tell Pimpl to self destruct, derred if necessary
    prvt->destroy();
    prvt = NULL;
}

/**
 * helper functor for accessing pointer-containers 
 * dereferences both arguments of the binary functor it wraps
 */
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

/** Equality Comparator 
 * Warning: ignores deleted state. Should it consider that?
 */
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

    // mark slot(s) as deleted
    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        if (**it == r) {
            (*it)->deleted = true;
        }
    }
    
    // remove and free if possible
    prvt->clean_slots();
}

/** Disconnects the Signal from the supplied RCB */
void AW_signal::disconnect(const WindowCallback& wcb, AW_window* aww) {
    WindowCallbackSlot w(wcb, aww);

    // mark slot(s) as deleted
    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        if (**it == w) {
            (*it)->deleted = true;
        }
    }
    
    // remove and free if possible
    prvt->clean_slots();
}



/** Emits the signal (i.e. runs all connected callbacks) */
void AW_signal::emit() {
    if (prvt) prvt->emit();
}

// internal implementation
void AW_signal::Pimpl::emit() {
    if (!enabled) return;

    in_emit++;

    // guard against emission loops
    aw_return_if_fail(in_emit < MAX_EMIT_RECURSION);

    for (std::list<Slot*>::iterator it = slots.begin(); it != slots.end(); ++it) {
        // skip slots marked as deleted
        if ((*it)->deleted) continue;

        (*it)->emit();
        
        // remember last touched window
        if ((*it)->get_window()) {
            last_window = (*it)->get_window();
        }
    }

    in_emit--;

    // cleanup slots removed during signal emission
    clean_slots();

    // trigger self destruct if our AW_signal was deleted in the mean time
    if (deferred_destroy) destroy();
}

AW_window* AW_signal::get_last_window() const {
    return prvt->last_window;
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

int rcb_self_remove_count = 0;
int wcb_self_remove_count = 0;

static void rcb_self_remove(AW_root*, AW_signal* sig) {
    rcb_self_remove_count ++;
    sig->disconnect(makeRootCallback(rcb_self_remove, sig));
    sig->emit();
}

static void wcb_self_remove(AW_window* w, AW_signal* sig) {
    wcb_self_remove_count ++;
    sig->disconnect(makeWindowCallback(wcb_self_remove, sig), w);
    sig->emit();
}

void TEST_AW_signal_self_remove() {
    AW_signal sig;
    
    sig.connect(makeRootCallback(rcb_self_remove, &sig));
    sig.connect(makeWindowCallback(wcb_self_remove, &sig), (AW_window*)123);
    sig.emit();
    
    TEST_EXPECT_EQUAL(sig.size(), 0);
    TEST_EXPECT_EQUAL(rcb_self_remove_count, 1);
    TEST_EXPECT_EQUAL(wcb_self_remove_count, 1);
}

int rcb_self_destroy_count = 0;

static void rcb_self_destroy(AW_root*, AW_signal *sig) {
    rcb_self_destroy_count ++;
    delete sig;
}

void TEST_AW_signal_self_destroy() {
    AW_signal *sig = new AW_signal();
    sig->connect(makeRootCallback(rcb_self_destroy, sig));
    sig->emit();
    TEST_EXPECT_EQUAL(rcb_self_destroy_count, 1);
}
TEST_PUBLISH(TEST_AW_signal_self_destroy);

#endif
