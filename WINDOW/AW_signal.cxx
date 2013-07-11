#include "aw_signal.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"

/**
 * Executes the contained RootCallback
 */
void AW_signal::RootCallbackSlot::emit() {
    cb(AW_root::SINGLETON);
}

/**
 * Executes the contained WindowCallback
 */
void AW_signal::WindowCallbackSlot::emit() {
    cb(aww);
}

/**
 * Connects the Signal to the supplied WindowCallback and Window
 */
void AW_signal::connect(const WindowCallback& wcb, AW_window* aww) {
    aw_return_if_fail(aww != NULL);
    slots.push_front(new WindowCallbackSlot(wcb, aww));
}

/** 
 * Connects the Signal to the supplied RootCallback
 */
void AW_signal::connect(const RootCallback& rcb) {
    slots.push_front(new RootCallbackSlot(rcb));
}

/**
 * Emits the signal (i.e. runs all connected callbacks)
 */
void AW_signal::emit() {
    for (std::list<Slot*>::iterator it = slots.begin();
         it != slots.end(); ++it) {
        (*it)->emit();
    }
}

/**
 * Disconnects all callbacks from signal
 */
void AW_signal::clear() {
    for (std::list<Slot*>::iterator it = slots.begin();
         it != slots.end(); ++it) {
        delete (*it);
    }
    slots.clear();
}

/**
 * Destructor
 */
AW_signal::~AW_signal() {
    clear();
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
