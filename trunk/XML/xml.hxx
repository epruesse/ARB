/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001
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

#ifndef XML_HXX
#define XML_HXX

#ifndef __STRING__
#include <string>
#endif
#ifndef __IOSTREAM__
#include <iostream>
#endif

namespace rs {

    /** @memo Classes used to write xml
        @doc  In order to write xml to a stream just open an XML_Document and create and destroy the needed tags.
    */

    namespace xml {

        class                XML_Document;
        extern XML_Document *theDocument; // there can only be one

        //  ----------------------------
        //      class XML_Attribute
        //  ----------------------------
        class XML_Attribute {
        private:
            string         name;
            string         content;
            XML_Attribute *next;

        public:
            XML_Attribute(const string& name_, const string& content_);
            virtual ~XML_Attribute();

            XML_Attribute *append_to(XML_Attribute *queue);

            void print(ostream& out) const;
        };


        //  -----------------------
        //      class XML_Node
        //  -----------------------
        class XML_Node {
        protected:
            XML_Node *father;
            bool      opened;
            int       indent;

        public:
            XML_Node();
            virtual ~XML_Node();

            int Indent() const { return indent; }
            bool Opened() const { return opened; }
//             void set_Opened(bool opened_) { opened = opened_; }

            virtual void add_son(XML_Node *son_)    = 0;
            virtual void remove_son(XML_Node *son_) = 0;
            virtual void open(ostream& out)         = 0;
            virtual void close(ostream& out)        = 0;
        };

        //  ----------------------------------------
        //      class XML_Tag : public XML_Node
        //  ----------------------------------------

        /// xml element
        class XML_Tag : public XML_Node {
        private:
            string         name;
            XML_Node      *son;
            XML_Attribute *attribute;

        public:
            /** Create a new xml element
                @param name_ element name
             */
            XML_Tag(const string &name_);
            virtual ~XML_Tag();

            /** add an attribute to the XML_Tag
                @param name_ attribute name
                @param content_ attribute value
            */
            void add_attribute(const string& name_, const string& content_);
            virtual void add_son(XML_Node *son_);
            virtual void remove_son(XML_Node *son_);
            virtual void open(ostream& out);
            virtual void close(ostream& out);
        };

        //  -----------------------------------------
        //      class XML_Text : public XML_Node
        //  -----------------------------------------
        /// a xml text node
        class XML_Text : public XML_Node {
        private:
            string content;

        public:
            /** Create text (content) in xml
                @param content_ the content
             */
            XML_Text(const string& content_) : content(content_) {}
            virtual ~XML_Text();

            virtual void add_son(XML_Node *son_);
            virtual void remove_son(XML_Node *son_);
            virtual void open(ostream& out);
            virtual void close(ostream& out);
        };


        //  ---------------------------
        //      class XML_Document
        //  ---------------------------
        /// an entire xml document
        class XML_Document {
        private:
            string    dtd;
            XML_Tag  *root;
            XML_Node *latest_son;
            ostream&  out;

        public:
            /** Create and stream (at destruction) a xml document
                @param name_ name of the root node
                @param dtd_ filename of dtd
                @param out_ stream where xml document will be written to
            */
            XML_Document(const string& name_, const string& dtd_, ostream& out_);
            virtual ~XML_Document();

            XML_Node* LatestSon() { return latest_son; }
            void set_LatestSon(XML_Node* latest_son_) { latest_son = latest_son_; }

            ostream& Out() { return out; }
        };
    };                          // end of namespace xml
};                              // end of namespace rs

#else
#error xml.hxx included twice
#endif // XML_HXX
