/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000
// Ralf Westram
// Time-stamp: <Sat Mar/24/2001 16:36 MET Coder@ReallySoft.de>
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
using namespace rs;
using namespace rs::xml;

XML_Document *rs::xml::theDocument = 0;

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
//  ------------------------------------------------
//      void XML_Attribute::print(ostream& out)
//  ------------------------------------------------
void XML_Attribute::print(ostream& out) const {
    out << " " << name << "=\"" << content << "\"";
    if (next) next->print(out);
}

// ********************************************************************************

//  -----------------------------
//      XML_Node::XML_Node()
//  -----------------------------
XML_Node::XML_Node() {
    assert(theDocument);

    father = theDocument->LatestSon();
    theDocument->set_LatestSon(this);
    indent = 0;

    if (father) {
        father->add_son(this);
        indent = father->Indent()+1;
    }

    opened = false;
}

//  ------------------------------
//      XML_Node::~XML_Node()
//  ------------------------------
XML_Node::~XML_Node() {
    if (father) father->remove_son(this);
    theDocument->set_LatestSon(father);
}

// ********************************************************************************

inline void to_indent(ostream& out, int indent) { while (indent--) out << ' '; }

//  ---------------------------------------------
//      XML_Tag::XML_Tag(const string &name_)
//  ---------------------------------------------
XML_Tag::XML_Tag(const string &name_)
: name(name_), son(0), attribute(0)
{
}
//  ----------------------------
//      XML_Tag::~XML_Tag()
//  ----------------------------
XML_Tag::~XML_Tag() {
    ostream& out = theDocument->Out();
    if (son) throw string("XML_Tag has son in destructor");
    close(out);
}

//  ---------------------------------------------------------------------------------
//      void XML_Tag::add_attribute(const string& name_, const string& content_)
//  ---------------------------------------------------------------------------------
void XML_Tag::add_attribute(const string& name_, const string& content_) {
    XML_Attribute *newAttr = new XML_Attribute(name_, content_);
    attribute = newAttr->append_to(attribute);
}
//  ----------------------------------------------
//      void XML_Tag::add_son(XML_Node *son_)
//  ----------------------------------------------
void XML_Tag::add_son(XML_Node *son_) {
    if (son) throw string("Tried to add a second son! Destroy previous son first.");
    son = son_;
}
//  -------------------------------------------------
//      void XML_Tag::remove_son(XML_Node *son_)
//  -------------------------------------------------
void XML_Tag::remove_son(XML_Node *son_) {
    if (son != son_) throw string("Tried to remove wrong son!");
    son = 0;
}

//  -----------------------------------------
//      void XML_Tag::open(ostream& out)
//  -----------------------------------------
void XML_Tag::open(ostream& out) {
    if (father && !father->Opened()) father->open(out);
    out << "\n"; to_indent(out, Indent());
    out << "<" << name;
    if (attribute) attribute->print(out);
    out << ">";
    opened = true;
}
//  -------------------------------------------
//      void XML_Tag::close(ostream& out)
//  -------------------------------------------
void XML_Tag::close(ostream& out)  {
    if (!opened) {
        if (father && !father->Opened()) father->open(out);
        out << "\n"; to_indent(out, Indent());
        out << "<" << name;
        if (attribute) attribute->print(out);
        out << "/>";
    }
    else {
        out << "\n"; to_indent(out, Indent());
        out << "</" << name << ">";
    }
}

// ********************************************************************************

//  ------------------------------
//      XML_Text::~XML_Text()
//  ------------------------------
XML_Text::~XML_Text() {
    ostream& out = theDocument->Out();
    close(out);
}
//  ---------------------------------------------------
//      void XML_Text::add_son(XML_Node */*son_*/)
//  ---------------------------------------------------
void XML_Text::add_son(XML_Node */*son_*/) {
    throw string("Can't add son to XML_Text-Node");
}
//  ------------------------------------------------------
//      void XML_Text::remove_son(XML_Node */*son_*/)
//  ------------------------------------------------------
void XML_Text::remove_son(XML_Node */*son_*/) {
    throw string("Can't remove son from XML_Text-Node");
}
//  ----------------------------------------------
//      void XML_Text::open(ostream& /*out*/)
//  ----------------------------------------------
void XML_Text::open(ostream& /*out*/) {
}
//  -------------------------------------------
//      void XML_Text::close(ostream& out)
//  -------------------------------------------
void XML_Text::close(ostream& out) {
    if (father && !father->Opened()) father->open(out);
    out << "\n"; to_indent(out, Indent());
    out << content;
}

// ********************************************************************************

//  -------------------------------------------------------------------------------------------
//      XML_Document::XML_Document(const string& name_, const string& dtd_, ostream& out_)
//  -------------------------------------------------------------------------------------------
XML_Document::XML_Document(const string& name_, const string& dtd_, ostream& out_)
    : dtd(dtd_), root(0), out(out_)
{
    if (theDocument) string Error("You can only have one XML_Document at a time.");
    theDocument = this;
    latest_son  = 0;
    root        = new XML_Tag(name_);
    assert(latest_son == root);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    out << "<!DOCTYPE " << name_ << " SYSTEM '" << dtd << "'>\n";
}

//  --------------------------------------
//      XML_Document::~XML_Document()
//  --------------------------------------
XML_Document::~XML_Document() {
    delete root;
    assert(theDocument == this);
    theDocument = 0;
}

