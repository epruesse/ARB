// --------------------------------------------------------------------------------
// Copyright (C) 2001
// Ralf Westram
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
// --------------------------------------------------------------------------------

#ifndef XML_HXX
#define XML_HXX

#ifndef __STRING__
#include <string>
#endif
#ifndef __CSTDIO__
#include <cstdio>
#endif

#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#define xml_assert(bed) arb_assert(bed)


/*! @memo Classes used to write xml
    @doc  In order to write xml to a stream just open an XML_Document and create and destroy the needed tags.
*/

class                XML_Document;
extern XML_Document *the_XML_Document; // there can only be one at a time

//  ----------------------------
//      class XML_Attribute

class XML_Attribute : virtual Noncopyable {
private:
    std::string    name;
    std::string    content;
    XML_Attribute *next;

public:
    XML_Attribute(const std::string& name_, const std::string& content_);
    virtual ~XML_Attribute();

    XML_Attribute *append_to(XML_Attribute *queue);

    void print(FILE *out) const;
};


//  -----------------------
//      class XML_Node

class XML_Node : virtual Noncopyable {
protected:
    XML_Node *father;
    bool      opened;
    int       indent;

public:
    XML_Node(bool is_tag);
    virtual ~XML_Node();

    int Indent() const { return indent; }
    bool Opened() const { return opened; }

    virtual void add_son(XML_Node *son_, bool son_is_tag) = 0;
    virtual void remove_son(XML_Node *son_)               = 0;
    virtual void open(FILE *out)                          = 0;
    virtual void close(FILE *out)                         = 0;
};

//  ----------------------------------------
//      class XML_Tag : public XML_Node

//! xml element
class XML_Tag : public XML_Node {  // derived from a Noncopyable
private:
    std::string    name;
    XML_Node      *son;
    XML_Attribute *attribute;
    int            state;       // 0        = no son; 1 = only content; 2 = son-tag;
    bool           onExtraLine; // default = true; if false -> does not print linefeed before tag

public:
    /*! Create a new xml element
        @param name_ element name
    */
    XML_Tag(const std::string &name_);
    virtual ~XML_Tag() OVERRIDE;

    /*! add an attribute to the XML_Tag
        @param name_ attribute name
        @param content_ attribute value
    */
    void         add_attribute(const std::string& name_, const std::string& content_);
    void         add_attribute(const std::string& name_, int value);
    virtual void add_son(XML_Node *son_, bool son_is_tag) OVERRIDE;
    virtual void remove_son(XML_Node *son_) OVERRIDE;
    virtual void open(FILE *out) OVERRIDE;
    virtual void close(FILE *out) OVERRIDE;

    void set_on_extra_line(bool oel) { onExtraLine = oel; }
};

//! a xml text node
class XML_Text : public XML_Node {
private:
    std::string content;

public:
    /*! Create text (content) in xml
        @param content_ the content
    */
    XML_Text(const std::string& content_) : XML_Node(false), content(content_) {}
    virtual ~XML_Text() OVERRIDE;

    virtual void add_son(XML_Node *son_, bool son_is_tag) OVERRIDE __ATTR__NORETURN;
    virtual void remove_son(XML_Node *son_) OVERRIDE __ATTR__NORETURN;
    virtual void open(FILE *) OVERRIDE;
    virtual void close(FILE *out) OVERRIDE;
};

class XML_Comment : public XML_Node {
    std::string content;
public:
    XML_Comment(const std::string& content_) : XML_Node(false), content(content_) {}
    virtual ~XML_Comment() OVERRIDE;

    virtual void add_son(XML_Node *son_, bool son_is_tag) OVERRIDE __ATTR__NORETURN;
    virtual void remove_son(XML_Node *son_) OVERRIDE __ATTR__NORETURN;
    virtual void open(FILE *) OVERRIDE;
    virtual void close(FILE *out) OVERRIDE;
};

//! an entire xml document
class XML_Document : virtual Noncopyable {
private:
    std::string  dtd;
    XML_Tag     *root;
    XML_Node    *latest_son;
    FILE        *out;

public:
    /*! Create and stream (at destruction) a xml document
        @param name_ name of the root node
        @param dtd_ filename of dtd
        @param out_ FILE where xml document will be written to
    */
    XML_Document(const std::string& name_, const std::string& dtd_, FILE *out_);
    virtual ~XML_Document() ;

    //! true -> tags w/o content or attributes are skipped (default = false)
    bool skip_empty_tags;

    //! how many columns are used per indentation level (defaults to 1)
    size_t indentation_per_level;

    XML_Node* LatestSon() { return latest_son; }
    void set_LatestSon(XML_Node* latest_son_) { latest_son = latest_son_; }

    XML_Tag& getRoot() { return *root; }

    void add_attribute(const std::string& name_, const std::string& content_) {
        getRoot().add_attribute(name_, content_);
    }

    FILE *Out() { return out; }
};

#else
#error xml.hxx included twice
#endif // XML_HXX
