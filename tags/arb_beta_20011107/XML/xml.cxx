/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000
// Ralf Westram
// Time-stamp: <Tue Oct/02/2001 18:09 MET Coder@ReallySoft.de>
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Ralf Westram makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//
// This code is part of my library.
// You may find a more recent version at http://www.reallysoft.de/
//
/////////////////////////////////////////////////////////////////////////////

#include "xml.hxx"

using namespace std;
// using namespace rs;
// using namespace rs::xml;

XML_Document *the_XML_Document = 0;

// ********************************************************************************

//  ----------------------------------------------------------------------------------
//      XML_Attribute::XML_Attribute(const string& name_, const string& content_)
//  ----------------------------------------------------------------------------------
XML_Attribute::XML_Attribute(const string& name_, const string& content_)
    : name(name_), content(content_), next(0)
{}

//  ----------------------------------------
//      XML_Attribute::~XML_Attribute()
//  ----------------------------------------
XML_Attribute::~XML_Attribute() {
    delete next;
}
//  ----------------------------------------------------------------------
//      XML_Attribute *XML_Attribute::append_to(XML_Attribute *queue)
//  ----------------------------------------------------------------------
XML_Attribute *XML_Attribute::append_to(XML_Attribute *queue) {
    if (!queue) return this;
    queue->next = append_to(queue->next);
    return queue;
}
//  ---------------------------------------------------
//      void XML_Attribute::print(FILE *out) const
//  ---------------------------------------------------
void XML_Attribute::print(FILE *out) const {
    fprintf(out, " %s=\"%s\"", name.c_str(), content.c_str());
//     out << " " << name << "=\"" << content << "\"";
    if (next) next->print(out);
}

// ********************************************************************************

//  ----------------------------------------
//      XML_Node::XML_Node(bool is_tag)
//  ----------------------------------------
XML_Node::XML_Node(bool is_tag) {
    xml_assert(the_XML_Document);

    father = the_XML_Document->LatestSon();
    the_XML_Document->set_LatestSon(this);
    indent = 0;

    if (father) {
        father->add_son(this, is_tag);
        indent = father->Indent()+1;
    }

    opened = false;
}

//  ------------------------------
//      XML_Node::~XML_Node()
//  ------------------------------
XML_Node::~XML_Node() {
    if (father) father->remove_son(this);
    the_XML_Document->set_LatestSon(father);
}

// ********************************************************************************

inline void to_indent(FILE *out, int indent) { while (indent--) fputc(' ', out); }

//  ---------------------------------------------
//      XML_Tag::XML_Tag(const string &name_)
//  ---------------------------------------------
XML_Tag::XML_Tag(const string &name_)
: XML_Node(true), name(name_), son(0), attribute(0), state(0)
{
}
//  ----------------------------
//      XML_Tag::~XML_Tag()
//  ----------------------------
XML_Tag::~XML_Tag() {
    FILE *out = the_XML_Document->Out();
    if (son) {
        throw string("XML_Tag has son in destructor");
    }
    close(out);
}

//  ---------------------------------------------------------------------------------
//      void XML_Tag::add_attribute(const string& name_, const string& content_)
//  ---------------------------------------------------------------------------------
void XML_Tag::add_attribute(const string& name_, const string& content_) {
    XML_Attribute *newAttr = new XML_Attribute(name_, content_);
    attribute = newAttr->append_to(attribute);
}
//  ---------------------------------------------------------------
//      void XML_Tag::add_son(XML_Node *son_, bool son_is_tag)
//  ---------------------------------------------------------------
void XML_Tag::add_son(XML_Node *son_, bool son_is_tag) {
    if (son) throw string("Tried to add a second son! Destroy previous son first.");
    son                           = son_;
    int wanted_state              = son_is_tag?2:1;
    if (state<wanted_state) state = wanted_state;
}
//  -------------------------------------------------
//      void XML_Tag::remove_son(XML_Node *son_)
//  -------------------------------------------------
void XML_Tag::remove_son(XML_Node *son_) {
    if (son != son_) throw string("Tried to remove wrong son!");
    son = 0;
}

//  --------------------------------------
//      void XML_Tag::open(FILE *out)
//  --------------------------------------
void XML_Tag::open(FILE *out) {
    if (father && !father->Opened()) father->open(out);
    fputc('\n', out); to_indent(out, Indent());
    fputc('<', out); fputs(name.c_str(), out);
    if (attribute) attribute->print(out);
    fputc('>', out);
    opened = true;
}
//  ----------------------------------------
//      void XML_Tag::close(FILE *out)
//  ----------------------------------------
void XML_Tag::close(FILE *out)  {
    if (!opened) {
        if (!the_XML_Document->skip_empty_tags || attribute || !father) {
            if (father && !father->Opened()) father->open(out);
            fputc('\n', out); to_indent(out, Indent());
            fputc('<', out); fputs(name.c_str(), out);
            if (attribute) attribute->print(out);
            fputs("/>", out);
        }
    }
    else {
        if (state >= 2) { fputc('\n', out); to_indent(out, Indent()); }
        fprintf(out, "</%s>", name.c_str());
    }
}

// ********************************************************************************

//  ------------------------------
//      XML_Text::~XML_Text()
//  ------------------------------
XML_Text::~XML_Text() {
    FILE *out = the_XML_Document->Out();
    close(out);
}
//  -------------------------------------------------
//      void XML_Text::add_son(XML_Node *, bool)
//  -------------------------------------------------
void XML_Text::add_son(XML_Node *, bool) {
    throw string("Can't add son to XML_Text-Node");
}
//  ------------------------------------------------------
//      void XML_Text::remove_son(XML_Node */*son_*/)
//  ------------------------------------------------------
void XML_Text::remove_son(XML_Node */*son_*/) {
    throw string("Can't remove son from XML_Text-Node");
}
//  -----------------------------------
//      void XML_Text::open(FILE*)
//  -----------------------------------
void XML_Text::open(FILE*) {
}
//  ----------------------------------------
//      void XML_Text::close(FILE *out)
//  ----------------------------------------
void XML_Text::close(FILE *out) {
    if (father && !father->Opened()) father->open(out);
    //fputc('\n', out); to_indent(out, Indent());
    fputs(content.c_str(), out);
}

// ********************************************************************************

//  ---------------------------------------------------------------------------------------
//      XML_Document::XML_Document(const string& name_, const string& dtd_, FILE *out_)
//  ---------------------------------------------------------------------------------------
XML_Document::XML_Document(const string& name_, const string& dtd_, FILE *out_)
    : dtd(dtd_), root(0), out(out_), skip_empty_tags(false)
{
    if (the_XML_Document) string Error("You can only have one XML_Document at a time.");
    the_XML_Document = this;
    latest_son  = 0;
    root        = new XML_Tag(name_);
    xml_assert(latest_son == root);

    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n", out);
    fprintf(out, "<!DOCTYPE %s SYSTEM '%s'>\n", name_.c_str(), dtd.c_str());
}

//  --------------------------------------
//      XML_Document::~XML_Document()
//  --------------------------------------
XML_Document::~XML_Document() {
    delete root;
    xml_assert(the_XML_Document == this);
    the_XML_Document = 0;
}
