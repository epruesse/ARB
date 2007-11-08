
/* Conventions:	All firstXxxx functions may return NULL, if no elements is available
 *			all nextXxxxx	functions return NULL, if no more elements are available
 */

const int maxErrorStringLength = 4000;
typedef int Boolean;
class SeerInterfaceData;

int aw_message( const char *msg, char *buttons );
void aw_message(const char *msg);

/* ******************** Show a slider *********************************/
void aw_openstatus( const char *title );	// show status
void aw_closestatus( void );		// hide status

int aw_status( const char *text );		// return 1 if exit button is pressed + set statustext
int aw_status( double gauge );	// return 1 if exit button is pressed + set statusslider
int aw_status( void );		// return 1 if exit button is pressed



enum SeerInterfaceErrorType {
    SIET_QUALITY_ERROR,
    SIET_DATA_FORMAT_ERROR,
    SIET_OBJECT_STORE_ERROR
};

enum SeerInterfaceOutputFormat {
    SIOF_ASCII_FORMAT,
    SIOF_HTML_FORMAT,
    SIOF_READABLE_FORMAT
};

enum SeerInterfaceDataType {
    SIDT_STRING,
    SIDT_LINK,
    SIDT_SET
};

enum SeerExportQualityLevel {
    SQEX_TEMPORARY,
    SQEX_MACHINE_GENERATED,
    SQEX_HAND_EDITED,
    SQEX_DOUBLE_CHECKED
};

enum SeerImportQualityLevel {
    SQIM_MASKED_OUT,
    SQIM_UNMODIFIED,
    SQIM_TEMPORARY,
    SQIM_MACHINE_GENERATED,
    SQIM_HAND_EDITED,
    SQIM_DOUBLE_CHECKED
};

enum SeerInterfaceSequenceDataType {
    SISDT_REAL_SEQUENCE,
    SISDT_CONSENSUS,	// can be used as filter ...
    SISDT_FILTER,	// 0100101010001 string
    SISDT_RATES,
    SISDT_ETC
};

enum SeerInterfaceAttributeType {
    SIAT_STRING,
    SIAT_LINK
//    SIAT_INTEGER_ARRAY,
//    SIAT_FLOAT_ARRAY
};

enum SeerInterfaceAlignmentType {
    SIAT_DNA,
    SIAT_RNA,
    SIAT_PRO
};

class SeerInterfaceError {
public:
    enum SeerInterfaceErrorType	errorType;
    char *errorString;
    SeerInterfaceData *errorSource; // the [optional] reason for the error,
    SeerInterfaceError(SeerInterfaceErrorType type,const char *templateString,...); // length of string is limited to
				// maxErrorStringLength
    ~SeerInterfaceError();
    void print() const;
};




class SeerInterfaceDataString;
class SeerInterfaceDataLink;
class SeerInterfaceDataSet;
class SeerDataVisitor;  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  <<<<<<<<<<<<<<<new Class!!!

class SeerInterfaceData {
    friend class SeerInterfaceDataSet;
    SeerInterfaceData *prev;
    SeerInterfaceData *next; // linked list
    SeerInterfaceDataSet *father;
protected:
    SeerInterfaceData();
public:
    char *key;
    enum SeerExportQualityLevel exportQualityLevel;
    enum SeerImportQualityLevel importQualityLevel;
    
    virtual void print() const = 0;
    virtual SeerInterfaceDataType type() = 0;
    virtual ~SeerInterfaceData();
    virtual SeerInterfaceDataString *toDataString(); // converts this savely to String type
    virtual SeerInterfaceDataSet *toDataSet(); //
    virtual SeerInterfaceDataLink *toDataLink(); //

  virtual void accept(SeerDataVisitor&); // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
  
};

class SeerInterfaceDataString: public SeerInterfaceData {
public:
    virtual SeerInterfaceDataType type();
    char *value;
    void set(const char *key, const char *value);
    SeerInterfaceDataString();
    SeerInterfaceDataString(const char *key, const char *value);
    virtual SeerInterfaceDataString *toDataString(); // converts this savely to String type
    virtual void print() const ;
    ~SeerInterfaceDataString();

  virtual void accept(SeerDataVisitor&); // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
  
};

class SeerInterfaceDataLink: public SeerInterfaceData {
public:
    virtual SeerInterfaceDataType type();
    char *linktoTableName;
    char *linktoTableId;
    void set(const char *key, const char *table, const char *link);
    SeerInterfaceDataLink();
    SeerInterfaceDataLink(const char *key, const char *table, const char *link);
    virtual SeerInterfaceDataLink *toDataLink(); // conyverts this savely to String type
    virtual void print() const;
    ~SeerInterfaceDataLink();

  virtual void accept(SeerDataVisitor&); // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
  
};

class SeerInterfaceDataSet: public SeerInterfaceData {
    SeerInterfaceData *firstElement;
    SeerInterfaceData *lastElement;
    SeerInterfaceData *localLoopPointer;
public:
    virtual SeerInterfaceDataSet *toDataSet(); //
    virtual SeerInterfaceDataType type();
    SeerInterfaceData 	*firstData();
    SeerInterfaceData	*nextData();
    void		addData(SeerInterfaceData *data);
    void		removeData(SeerInterfaceData *data);
    SeerInterfaceData	*findData(const char *key);
    const SeerInterfaceData	*findData(const char *key)const ;
    virtual void	print() const; // for debugging
    SeerInterfaceDataSet();
    ~SeerInterfaceDataSet();

  virtual void accept(SeerDataVisitor&); // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
  
};

class SeerInterfaceDataStringSet: public SeerInterfaceDataSet {
public:
    virtual SeerInterfaceDataType type();
    SeerInterfaceDataString 	*firstData() { return (SeerInterfaceDataString *)SeerInterfaceDataSet::firstData();};
    SeerInterfaceDataString 	*nextData() { return (SeerInterfaceDataString *)SeerInterfaceDataSet::nextData();};
//    void 			addData(SeerInterfaceDataString *data) { SeerInterfaceDataSet::addData(data);};
    SeerInterfaceDataString	*findData(const char *searchKey) { return (SeerInterfaceDataString *)SeerInterfaceDataSet::findData(searchKey);};
    const SeerInterfaceDataString	*findData(const char *searchKey)const { return (const SeerInterfaceDataString *)SeerInterfaceDataSet::findData(searchKey);};

  virtual void accept(SeerDataVisitor&); // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
  
};

class SeerDataVisitor {  // <<<<<<<<<<<<< <<<<<<<<<<<<<<<<<<<<<<<<< <<<<<<< <<<< new Class!!!
public:
  virtual void visitSeerInterfaceData(SeerInterfaceData&);
  virtual void visitSeerInterfaceDataString(SeerInterfaceDataString&);
  virtual void visitSeerInterfaceDataLink(SeerInterfaceDataLink&);
  virtual void visitSeerInterfaceDataSet(SeerInterfaceDataSet&);
  virtual void visitSeerInterfaceDataStringSet(SeerInterfaceDataStringSet&);
protected:
  virtual ~SeerDataVisitor() = 0;
};


class SeerInterfaceSequenceData {
    SeerInterfaceSequenceData(); // no public default constructur
public:
    char *uniqueID;			// rdpSid ????
    SeerInterfaceSequenceDataType sequenceType;			// filter or real sequences
    SeerInterfaceDataStringSet	sequences;	// ??? whats the key of the real sequence ?????
    SeerInterfaceDataSet 	attributes;
    void	print()const; // for debugging
    SeerInterfaceSequenceData(const char *id);
    ~SeerInterfaceSequenceData();
};


class SeerInterfaceTableData {
    SeerInterfaceTableData();
public:
    char *uniqueID;
    SeerInterfaceDataSet 	attributes;
    void	print()const; // for debugging
    SeerInterfaceTableData(const char *id);
    ~SeerInterfaceTableData();
};

class SeerInterfaceAttribute {
public:
    char *name;
    char *pubCmnt;
    enum SeerInterfaceAttributeType	type;
    Boolean	changeable;
    Boolean	deleteable;
    Boolean	queryable;
    Boolean	requested;	// needed by the arb side
    Boolean	element_for_upload; // in arb: at least one element has changes
    int		sortindex;
    void print()const;
    SeerInterfaceAttribute();
    ~SeerInterfaceAttribute();
};



class SeerInterfaceAlignment {
    SeerInterfaceAlignment();
public:
    char *name;
    char *pubCmnt;
    SeerInterfaceAlignmentType typeofAlignment;
    void print()const ;
    SeerInterfaceAlignment(const char *name,const char *pub);
    ~SeerInterfaceAlignment();
};

class SeerInterfaceTableDescription {
    SeerInterfaceTableDescription();
public:
    GB_HASH 			*attribute_hash; // used by arb
    int sort_key;		// attributes are sorted according to this entry
    char *tablename;		// should be a very short string like REF NOM GBT
    char *tabledescription;	// one user readable word
    
    virtual SeerInterfaceAttribute *firstAttribute(); // retrieve attribute_name - attribute_description
    virtual SeerInterfaceAttribute *nextAttribute();

    SeerInterfaceTableDescription(const char *name,const char *desc);
    virtual ~SeerInterfaceTableDescription();
    void print();
};


class SeerInterface {
public:
    
    virtual SeerInterfaceError *beginSession(const char *username, const char *userpasswd);
    virtual SeerInterfaceError *endSession();
    
    virtual SeerInterfaceError *writeToArbDirectly(GBDATA *gb_main); // called after arb is fully loaded

    virtual SeerInterfaceError *beginTransaction();
    virtual SeerInterfaceError *commitTransaction();
    virtual SeerInterfaceError *abortTransaction();
    
    virtual SeerInterfaceError *beginReadOnlyTransaction();
    virtual SeerInterfaceError *commitReadOnlyTransaction();
    
    /* ************** next functions are called only within the scope of a transaction *****/
    virtual SeerInterfaceAlignment *firstAlignment(); // a list of all alignments in the database
    virtual SeerInterfaceAlignment *nextAlignment(); // a list of all alignments in the database

    virtual SeerInterfaceTableDescription *firstTableDescription();
    virtual SeerInterfaceTableDescription *nextTableDescription();
    
    virtual SeerInterfaceAttribute *firstAttribute(); // retrieve attribute_name - attribute_description
    virtual SeerInterfaceAttribute *nextAttribute();
    /* a list of all available datas
     *	if an data is not directly attached to the sequenceData
     * but seperated by a 1->n link, concatenate data names using the '/'
     * delimiter, example
     *	'reference/authors',  'reference/journal' 'gb_origin/from', 'gb_origin/acc' ...
     */


    /* ****************** query database and retrieve data ************************/
    
    virtual SeerInterfaceError *queryDatabase(const char *alignmentName, const char *attributeName=0,const char *attributeValue=0);
    virtual SeerInterfaceSequenceData *querySingleSequence(const char *alignmentName, const char *uniqueId);
    // query for an optional alignment and optional data=value
    // There will be only one query at a time !!!
    // After query is run the following functons may be called:
    virtual long numberofMatchingSequences();
    virtual SeerInterfaceError *setAttributeFilter(SeerInterfaceDataStringSet *requestedAttributes);// default is to get everything
    virtual void setOutputFormat(SeerInterfaceOutputFormat format);
    
    virtual SeerInterfaceSequenceData *firstSequenceData(); // fills in a SeerInterfaceSequence according to filter
    virtual SeerInterfaceSequenceData *nextSequenceData(); // fills in a SeerInterfaceSequence according to filter

    /******************** table read/write **********************************************/
    virtual SeerInterfaceTableData *querySingleTableData(const char *tablename,const char *tableEntryId); // extra tables, like Nomenclature, References, Strains
    
    virtual SeerInterfaceError *queryTableData(const char *alignmentname, const char *tablename);
    virtual long numberofMatchingTableData(); // called after queryTableData
    virtual SeerInterfaceTableData *firstTableData(); // extra tables, like Nomenclature, References, Strains
    virtual SeerInterfaceTableData *nextTableData();

    virtual SeerInterfaceError *checkTableData(const SeerInterfaceTableData *tableData);
    virtual SeerInterfaceError *storeTableData(const SeerInterfaceTableData *tableData);
    
    /* ****************** storing data in the database ***********************
     * first check then store
     * check should return very detailed error messages !!!!
     */
    virtual SeerInterfaceError *readFromArbDirectly(GBDATA *gb_main); // should use the slider

    virtual SeerInterfaceError *checkAttributeName(const SeerInterfaceAttribute *attribute,SeerImportQualityLevel qLevel);
    virtual SeerInterfaceError *storeAttributeName(const SeerInterfaceAttribute *attribute,SeerImportQualityLevel qLevel);

    virtual SeerInterfaceError *resortAttributeList(const SeerInterfaceDataStringSet *attributeList);

    virtual SeerInterfaceError *checkAlignmentName(const SeerInterfaceAlignment *alignment, SeerImportQualityLevel qLevel);
    virtual SeerInterfaceError *storeAlignmentName(const SeerInterfaceAlignment *alignment, SeerImportQualityLevel qLevel);

    /* store sequences, SeerQualityLevel is taken from the datas !!! */
    virtual SeerInterfaceError *checkSequence(const char *alignmentname, const SeerInterfaceSequenceData *sequence); // error->errorSource should be set
    virtual SeerInterfaceError *storeSequence(const char *alignmentname, const SeerInterfaceSequenceData *sequence);

    virtual void debug_function(void);
};

typedef SeerInterface *(SeerInterfaceCreator)(); // creates an seer interface
extern SeerInterfaceCreator *seerInterfaceCreator;

class SeerInstallInterfaceCreator{
    int dummy;
public:
    SeerInstallInterfaceCreator( SeerInterfaceCreator *ic ){
	seerInterfaceCreator = ic;
    };
};
