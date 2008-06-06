#include <stdio.h>
#include "d_classes.hxx"



Element::Element() {
    int i;
    contents        = 0;
    marked          = FALSE;

    first_son       = 0;
    last_son        = 0;
    left_brother    = 0;
    right_brother   = 0;

    width           = 50;   // currently fixed
    height          = 20;   // currently fixed
    width_of_sons   = 0;
    height_of_sons  = 0;
    number_of_sons  = 0;
}


Element::~Element() {

}


class Element *Element::set_contents(int v) {
    contents = v;

    return this;
}


class Element *Element::get_contents(int &v) {
    v = contents;

    return this;
}


class Element *Element::print_contents(void) {
    printf("contents:%i\n", contents);

    return this;
}


class Element *Element::print_contents_recursive(int depth) {
    class Element *tmp = first_son;

    printf("depth:%i  ",depth);
    this->print_contents();
    while(tmp) {
        tmp->print_contents_recursive(depth+1);
        tmp = tmp->right_brother;
    }

    return this;
}


class Element *Element::dump(void) {

    printf("DUMP:: contents: %i  width of sons: %i\n",contents,width_of_sons);

    return this;
}


class Element *Element::dump_recursive(void) {
    class Element *tmp = first_son;

    this->dump();

    while(tmp) {
        tmp->dump_recursive();
        tmp = tmp->right_brother;
    }

    return this;
}


class Element *Element::calculate_sizes(void) {
    class Element *_tmp = first_son;
    int     _width = 0;
    int     _height = 0;

    while(_tmp) {
        _tmp->calculate_sizes();
        _width += width;
        _tmp = _tmp->right_brother;
    }

    width_of_sons = _width;

    return this;
}


class Element *Element::add_son(void) {
    class Element *tmp;

    tmp = new Element;

    if(!first_son) {
        first_son = tmp;
        tmp->left_brother = 0;
    }else{
        tmp->left_brother = last_son;
        last_son->right_brother = tmp;
    }
    last_son = tmp;
    tmp->right_brother = 0;

    number_of_sons++;

    return tmp;
}


class Element *Element::get_number_of_sons(int &n) {
    n = number_of_sons;

    return this;
}


class Element **Element::get_sons(void) {
    return 0; // not finished
}
