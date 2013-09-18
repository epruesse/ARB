#include "aw_choice.hxx"

AW_choice::AW_choice(AW_awar* awari, AW_action& act, int32_t val) 
    : AW_action(act),
      awar(awari),
      value(val)
{}

AW_choice::AW_choice(AW_awar* awari, AW_action& act, double val) 
    : AW_action(act),
      awar(awari),
      value(val)
{}

AW_choice::AW_choice(AW_awar* awari, AW_action& act, const char* val) 
    : AW_action(act),
      awar(awari),
      value(val)
{}

AW_choice::AW_choice(const AW_choice& o) 
    : AW_action(o),
      awar(o.awar),
      value(o.value)
{
}

AW_choice& AW_choice::operator=(const AW_choice& o) {
    AW_action::operator=(o);
    awar = o.awar;
    value = o.value;
    return *this;
}

AW_choice::~AW_choice() {
}

void AW_choice::user_clicked() {
    if (value == awar) return; // no event if nothing changed
    // fixme: help?
    // fixme: tracking?
    value.write_to(awar);
    clicked.emit();
}

AW_choice*
AW_choice_list::add_choice(AW_awar *awar, AW_action& act, int32_t val, bool def) {
    choices.push_back(AW_choice(awar, act, val));

    return &choices.back();
}

AW_choice*
AW_choice_list::add_choice(AW_awar *awar, AW_action& act, double val, bool def) {
    choices.push_back(AW_choice(awar, act, val));
    return &choices.back();
}

AW_choice*
AW_choice_list::add_choice(AW_awar *awar, AW_action& act, const char* val, bool def) {
    choices.push_back(AW_choice(awar, act, val));
    return &choices.back();
}
