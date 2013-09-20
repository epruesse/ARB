#include "aw_choice.hxx"

AW_choice::AW_choice(AW_choice_list* li, AW_action& act, int32_t val) 
    : AW_action(act),
      list(li),
      value(val)
{}

AW_choice::AW_choice(AW_choice_list* li, AW_action& act, double val) 
    : AW_action(act),
      list(li),
      value(val)
{}

AW_choice::AW_choice(AW_choice_list* li, AW_action& act, const char* val) 
    : AW_action(act),
      list(li),
      value(val)
{}

AW_choice::AW_choice(const AW_choice& o) 
    : AW_action(o),
      list(o.list),
      value(o.value)
{
}

AW_choice& AW_choice::operator=(const AW_choice& o) {
    AW_action::operator=(o);
    list = o.list;
    value = o.value;
    return *this;
}

AW_choice::~AW_choice() {
}

void AW_choice::user_clicked() {
    if (value == list->awar) return; // no event if nothing changed
    // fixme: help?
    // fixme: tracking?
    value.write_to(list->awar);
    clicked.emit();
}



AW_choice_list::AW_choice_list(AW_awar* aw) 
    : awar(aw)
{
}

AW_choice_list::~AW_choice_list() {
}

void AW_choice_list::update() {
    for (choices_t::iterator it = choices.begin(); it != choices.end(); ++it) {
        if (it->value == awar) {
            it->bound_set("active", 1, NULL);
        }
    }
}

AW_choice*
AW_choice_list::add_choice(AW_action& act, int32_t val, bool def) {
    choices.push_back(AW_choice(this, act, val));

    return &choices.back();
}

AW_choice*
AW_choice_list::add_choice(AW_action& act, double val, bool def) {
    choices.push_back(AW_choice(this, act, val));
    return &choices.back();
}

AW_choice*
AW_choice_list::add_choice(AW_action& act, const char* val, bool def) {
    choices.push_back(AW_choice(this, act, val));
    return &choices.back();
}
