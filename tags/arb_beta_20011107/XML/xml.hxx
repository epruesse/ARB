/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001
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

#ifndef XML_HXX
#define XML_HXX

#ifndef __STRING__
#include <string>
#endif
#ifndef __CSTDIO__
#include <cstdio>
#endif

#ifndef NDEBUG
# define xml_assert(bed) do { if (!(bed)) *(int *)0 = 0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define xml_assert(bed)
#endif /* NDEBUG */


// namespace rs {

    /** @memo Classes used to write xml
        @doc  In order to write xml to a stream just open an XML_Document and create and destroy the needed tags.
    */

//     namespace xml {

        class                XML_Document;
        extern XML_Document *theDocument; // there can only be one at a time

        //  ----------------------------
        //      class XML_Attribute
        //  ----------------------------
        class XML_Attribute {
        private:
            std::string         name;
            std::string         content;
            XML_Attribute *next;

        public:
            XML_Attribute(const std::string& name_, const std::string& content_);
            virtual ~XML_Attribute();

            XML_Attribute *append_to(XML_Attribute *queue);

            void print(FILE *out) const;
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
        //  ----------------------------------------

        /// xml element
        class XML_Tag : public XML_Node {
        private:
            std::string         name;
            XML_Node      *son;
            XML_Attribute *attribute;
            int            state; // 0 = no son; 1 = only content; 2 = son-tag;

        public:
            /** Create a new xml element
                @param name_ element name
            */
            XML_Tag(const std::string &name_);
            virtual ~XML_Tag();

            /** add an attribute to the XML_Tag
                @param name_ attribute name
                @param content_ attribute value
            */
            void         add_attribute(const std::string& name_, const std::string& content_);
            virtual void add_son(XML_Node *son_, bool son_is_tag);
            virtual void remove_son(XML_Node *son_);
            virtual void open(FILE *out);
            virtual void close(FILE *out);
        };

        //  -----------------------------------------
        //      class XML_Text : public XML_Node
        //  -----------------------------------------
        /// a xml text node
        class XML_Text : public XML_Node {
        private:
            std::string content;

        public:
            /** Create text (content) in xml
                @param content_ the content
             */
            XML_Text(const std::string& content_) : XML_Node(false), content(content_) {}
            virtual ~XML_Text();

            virtual void add_son(XML_Node *son_, bool son_is_tag);
            virtual void remove_son(XML_Node *son_);
            virtual void open(FILE *out);
            virtual void close(FILE *out);
        };

        //  --------------------------------------------
        //      class XML_Comment : public XML_Text
        //  --------------------------------------------
        class XML_Comment : public XML_Text {
        public:
            XML_Comment(const std::string& content_) : XML_Text("<!-- "+content_+" -->") {}
            virtual ~XML_Comment() {}
        };

        //  ---------------------------
        //      class XML_Document
        //  ---------------------------
        /// an entire xml document
        class XML_Document {
        private:
            std::string    dtd;
            XML_Tag  *root;
            XML_Node *latest_son;
            FILE     *out;

        public:
            /** Create and stream (at destruction) a xml document
                @param name_ name of the root node
                @param dtd_ filename of dtd
                @param out_ FILE where xml document will be written to
            */
            XML_Document(const std::string& name_, const std::string& dtd_, FILE *out_);
            virtual ~XML_Document();

            /// true -> tags w/o content or attributes are skipped (default = false)
            bool skip_empty_tags;

            XML_Node* LatestSon() { return latest_son; }
            void set_LatestSon(XML_Node* latest_son_) { latest_son = latest_son_; }

            FILE *Out() { return out; }
        };
//     };                          // end of namespace xml
// };                              // end of namespace rs

#else
#error xml.hxx included twice
#endif // XML_HXX
