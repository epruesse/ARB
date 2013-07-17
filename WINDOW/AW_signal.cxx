#include "aw_signal.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"

#include "gtk/gtk.h"


/*
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

struct AW_signal_g_signal_binding {
    GObject    *object;
    const char *sig_name;
    gulong      handler_id;
    AW_signal  *signal;

    AW_signal_g_signal_binding(GtkWidget*, const char*);
    AW_signal_g_signal_binding(const AW_signal_g_signal_binding&);
    ~AW_signal_g_signal_binding();

    AW_signal_g_signal_binding& operator=(const AW_signal_g_signal_binding&);
    bool operator==(const AW_signal_g_signal_binding&) const;

    void connect(AW_signal*);
    void disconnect();
    
};

struct AW_signal::Pimpl {
    std::list<Slot*> slots;
    std::list<AW_signal_g_signal_binding> gsignals;
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
    
    // copy the upstream g_signals we are connected to
    prvt->gsignals = o.prvt->gsignals;

    // connect to g_signals
    for (std::list<AW_signal_g_signal_binding>::iterator it = prvt->gsignals.begin();
         it != prvt->gsignals.end(); ++it) {
        it->connect(this);
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
    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        (*it)->emit();
    }
}

/** Disconnects all callbacks from signal */
void AW_signal::clear() {
    for (std::list<Slot*>::iterator it = prvt->slots.begin();
         it != prvt->slots.end(); ++it) {
        delete (*it);
    }
    prvt->slots.clear();
}


/////////////////////// binding to GTK / GLIB Signals ////////////////////////

/**
 * Connects a signal of a GTK widget to this signal.
 * (i.e. triggering of that signal triggers this signal
 */
void AW_signal::bind(GtkWidget* widget, const char* sig) {
    aw_return_if_fail(widget != NULL);
    aw_return_if_fail(sig != NULL);
    
    prvt->gsignals.push_back(AW_signal_g_signal_binding(widget, sig));
    prvt->gsignals.back().connect(this);
}

/**
 * Disconnect all signals coming from GtkWidget w
 */
void AW_signal::unbind(GtkWidget* widget, const char *sig) {
    aw_return_if_fail(widget != NULL);
    aw_return_if_fail(sig != NULL);

    prvt->gsignals.remove(AW_signal_g_signal_binding(widget, sig));
}

AW_signal_g_signal_binding::AW_signal_g_signal_binding(GtkWidget *w, const char* s) 
    : object(G_OBJECT(w)),
      sig_name(strdup(s)),
      handler_id(0),
      signal(NULL)
{
}

AW_signal_g_signal_binding::AW_signal_g_signal_binding(const AW_signal_g_signal_binding& o) 
    : object(o.object),
      sig_name(strdup(o.sig_name)),
      handler_id(0),
      signal(NULL)
{
}

AW_signal_g_signal_binding& 
AW_signal_g_signal_binding::operator=(const AW_signal_g_signal_binding& o) {
    disconnect();
    object = o.object;
    free((void*)sig_name);
    sig_name = strdup(o.sig_name);
    if (o.signal && o.handler_id) {
        connect(o.signal);
    }
    return *this;
}    

static bool _aw_signal_received_from_widget(GtkWidget*, gpointer data) {
    AW_signal* sig = (AW_signal*) data;
    sig->emit();
    return true; // hope this is right
}

static void _aw_signal_unref_widget(void* data, GObject* obj) {
    AW_signal_g_signal_binding* binding = (AW_signal_g_signal_binding*) data;
    binding->handler_id = 0;
    binding->signal->unbind(GTK_WIDGET(obj), binding->sig_name);
}


AW_signal_g_signal_binding::~AW_signal_g_signal_binding() {
    disconnect();
    free((void*)sig_name);
}

bool AW_signal_g_signal_binding::operator==(const AW_signal_g_signal_binding& o) const {
    return object == o.object && strcmp(sig_name, o.sig_name);
}

void AW_signal_g_signal_binding::disconnect() {
    if (!signal || !handler_id) return;
    g_signal_handler_disconnect(object, handler_id);
    g_object_weak_unref(object, _aw_signal_unref_widget, this);
    signal = NULL;
    handler_id = 0L;
}

void AW_signal_g_signal_binding::connect(AW_signal* s) {
    if (signal) disconnect();
    signal = s;

    g_object_weak_ref(object, _aw_signal_unref_widget, this);

    handler_id = g_signal_connect(object, sig_name, 
                                  G_CALLBACK(_aw_signal_received_from_widget), s);
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
