#pragma once

#include "aw_action.hxx"
#include "aw_scalar.hxx"
#include "aw_awar.hxx"
#include <vector>

typedef struct _GtkRadioButton GtkRadioButton;

class AW_choice : public AW_action {
private:
    AW_awar   *awar;
    AW_scalar  value;
    
    explicit AW_choice(AW_awar*, AW_action&, int32_t);
    explicit AW_choice(AW_awar*, AW_action&, double);
    explicit AW_choice(AW_awar*, AW_action&, const char*);

    friend class AW_choice_list;
public:
    // cpy ctors need to be public for vector to access them
    AW_choice(const AW_choice&);
    AW_choice& operator=(const AW_choice&);

    virtual ~AW_choice() OVERRIDE;
    
    virtual void user_clicked() OVERRIDE;

};

class AW_choice_list {
private:
    std::vector<AW_choice> choices;
    size_t                 default_idx;
public:
    AW_choice* add_choice(AW_awar*, AW_action&, int32_t, bool);
    AW_choice* add_choice(AW_awar*, AW_action&, double, bool);
    AW_choice* add_choice(AW_awar*, AW_action&, const char*, bool);
};
