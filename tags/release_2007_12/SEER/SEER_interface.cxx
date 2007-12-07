#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "seer_interface.hxx"

#define CORE *((int *)0) = 0

SeerInterfaceError::SeerInterfaceError(SeerInterfaceErrorType type,const char *templateString , ...){
    // length of string is limited to
    char buffer[maxErrorStringLength * 2];
    errorType = type;
    char *p = buffer;
    va_list	parg;
    memset(buffer,0,maxErrorStringLength);
    sprintf (buffer,"Error: ");
    p += strlen(p);
    va_start(parg,templateString);	

    vsprintf(p,templateString,parg);
    errorString = strdup(buffer);
}


SeerInterfaceError::~SeerInterfaceError(){
    delete errorString;
};

void SeerInterfaceError::print() const {
    printf("***** SeerInterfaceError  %i:'%s'\n",errorType, errorString);
}

SeerInterfaceData::SeerInterfaceData(){
    prev = 0;
    next = 0;
    father = 0;
    key = 0;
};

SeerInterfaceData::~SeerInterfaceData(){
    if (father) father->removeData(this);
    delete key;
}

SeerInterfaceDataSet *SeerInterfaceData::toDataSet(){
    CORE;
    return 0;
}
SeerInterfaceDataString *SeerInterfaceData::toDataString(){
    CORE;
    return 0;
}

SeerInterfaceDataLink *SeerInterfaceData::toDataLink(){
    CORE;
    return 0;
}

void SeerInterfaceData::accept(SeerDataVisitor& visitor) // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
{
  visitor.visitSeerInterfaceData(*this);
}


/******************* STRING *********************/
SeerInterfaceDataString::SeerInterfaceDataString(){
    value = 0;
}

SeerInterfaceDataString::SeerInterfaceDataString(const char *ikey, const char *ivalue){
    value = strdup(ivalue);
    key = strdup(ikey);
}

void SeerInterfaceDataString::set(const char *ikey, const char *ivalue){
    delete value;
    delete key;
    value = strdup(ivalue);
    key = strdup(ikey);
}

SeerInterfaceDataString::~SeerInterfaceDataString(){
    delete value;
};

SeerInterfaceDataString *SeerInterfaceDataString::toDataString(){
    return this;
};

SeerInterfaceDataType SeerInterfaceDataString::type(){
    return SIDT_STRING;
};
void SeerInterfaceDataString::print()const{
    printf("SIDS: '%s'='%s'\n",key,value);
}

void SeerInterfaceDataString::accept(SeerDataVisitor& visitor) // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
{
  visitor.visitSeerInterfaceDataString(*this);
}

/******************* LINK *********************/
SeerInterfaceDataLink::SeerInterfaceDataLink(){
    linktoTableName = 0;
    linktoTableId = 0;
}

SeerInterfaceDataLink::SeerInterfaceDataLink(const char *ikey, const char *table,const char *ivalue){
    linktoTableId = strdup(ivalue);
    linktoTableName = strdup(table);
    key = strdup(ikey);
}

void SeerInterfaceDataLink::set(const char *ikey, const char *table, const char *ivalue){
    delete linktoTableName;
    delete linktoTableId;
    delete key;
    key = strdup(ikey);
    linktoTableId = strdup(ivalue);
    linktoTableName = strdup(table);
}

SeerInterfaceDataLink::~SeerInterfaceDataLink(){
    delete linktoTableName;
    delete linktoTableId;    
};

SeerInterfaceDataLink *SeerInterfaceDataLink::toDataLink(){
    return this;
};

SeerInterfaceDataType SeerInterfaceDataLink::type(){
    return SIDT_LINK;
};

void SeerInterfaceDataLink::print()const{
    printf("LINK: '%s'='%s:%s'\n",key,linktoTableName,linktoTableId);
}

void SeerInterfaceDataLink::accept(SeerDataVisitor& visitor) // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
{
  visitor.visitSeerInterfaceDataLink(*this);
}

/******************* SET *********************/

SeerInterfaceDataSet::SeerInterfaceDataSet(){
    firstElement = 0;
    lastElement = 0;
};

SeerInterfaceDataSet::~SeerInterfaceDataSet(){
    while(firstElement){
	delete firstElement;
    }
}

SeerInterfaceDataSet *SeerInterfaceDataSet::toDataSet(){
    return this;
}

SeerInterfaceDataType SeerInterfaceDataSet::type(){
    return SIDT_SET;
};

SeerInterfaceData *SeerInterfaceDataSet::firstData(){
    localLoopPointer = firstElement;
    if (localLoopPointer) localLoopPointer = localLoopPointer->next;
    return firstElement;
}

SeerInterfaceData *SeerInterfaceDataSet::nextData(){
    SeerInterfaceData *res = localLoopPointer;
    if (localLoopPointer) localLoopPointer = localLoopPointer->next;
    return res;
}

void SeerInterfaceDataSet::addData(SeerInterfaceData *data){
    if (firstElement){
	data->prev = lastElement;
	data->next = 0;
	lastElement->next = data;
	lastElement = data;
    }else{
	firstElement = lastElement = data;
	data->prev = data->next = 0;
    }
    data->father = this;
}

void SeerInterfaceDataSet::removeData(SeerInterfaceData *data){
    assert (data->father == this);
    if (data->prev){
	data->prev->next = data->next;
    }else{
	firstElement = data->next;
    }
    if (data->next){
	data->next->prev = data->prev;
    }else{
	lastElement = data->prev;
    }
    data->next = data->prev = 0;
    data->father = 0;
}

SeerInterfaceData *SeerInterfaceDataSet::findData(const char *ikey){
    SeerInterfaceData *l;
    for (l=firstElement; l; l=l->next){
	if (!strcmp(l->key,ikey) ) return l;
    }
    return 0;
}
const SeerInterfaceData *SeerInterfaceDataSet::findData(const char *ikey)const {
    SeerInterfaceData *l;
    for (l=firstElement; l; l=l->next){
	if (!strcmp(l->key,ikey) ) return l;
    }
    return 0;
}
void SeerInterfaceDataSet::print()const{
    SeerInterfaceData *l;
    int i = 0;
    for (l=firstElement; l; l=l->next){ 
	printf("\t%i:",i++);
	l->print();
    }
}

void SeerInterfaceDataSet::accept(SeerDataVisitor& visitor) // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
{
  visitor.visitSeerInterfaceDataSet(*this);
}


SeerInterfaceDataType SeerInterfaceDataStringSet::type(){
    return SIDT_SET;
}

void SeerInterfaceDataStringSet::accept(SeerDataVisitor& visitor) // <<<<<<<<<<<<<<<<<<< <<<<<<<<<<new function!!!!
{
  visitor.visitSeerInterfaceDataStringSet(*this);
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< new Class!!
void SeerDataVisitor::visitSeerInterfaceData(SeerInterfaceData& ) {;}
void SeerDataVisitor::visitSeerInterfaceDataString(SeerInterfaceDataString& ) {;}
void SeerDataVisitor::visitSeerInterfaceDataLink(SeerInterfaceDataLink& ) {;}
void SeerDataVisitor::visitSeerInterfaceDataSet(SeerInterfaceDataSet& ) {;}
void SeerDataVisitor::visitSeerInterfaceDataStringSet(SeerInterfaceDataStringSet& ) {;}
SeerDataVisitor::~SeerDataVisitor() {;}


SeerInterfaceSequenceData::SeerInterfaceSequenceData(const char *id){
    uniqueID = strdup(id);
}

SeerInterfaceSequenceData::~SeerInterfaceSequenceData(){
    delete uniqueID;
}

void SeerInterfaceSequenceData::print()const {
    printf("************* SequenceData '%s'********************\n",uniqueID);
    printf("Sequences\n");
    sequences.print();
    printf("Attributes\n");
    attributes.print();
}

SeerInterfaceAttribute::SeerInterfaceAttribute(){
    memset((char *)this,0,sizeof(*this));
}

SeerInterfaceAttribute::~SeerInterfaceAttribute(){
    delete name;
    delete pubCmnt;
}

SeerInterfaceTableData::SeerInterfaceTableData(const char *id){
    uniqueID = strdup(id);
}

SeerInterfaceTableData::~SeerInterfaceTableData(){
    delete uniqueID;
}

void SeerInterfaceTableData::print()const{
    printf("***** Table Element **** '%s'\n",uniqueID);
}

void SeerInterfaceAttribute::print()const{
    printf("****************** Attribute Description *********");
    printf("Name '%s'	Description '%s'\n",name,(pubCmnt)?pubCmnt:"");
    printf("type %i	changeable %i deleteable %i\n",type,changeable,deleteable);
}

SeerInterfaceAlignment::SeerInterfaceAlignment(const char *iname,const char *pub){
    name = strdup(iname);
    pubCmnt = strdup(pub);
}

SeerInterfaceAlignment::~SeerInterfaceAlignment(){
    delete name;
    delete pubCmnt;
}

void SeerInterfaceAlignment::print()const{
    printf("********Algnment******\n");
    printf("Name: '%s'  pubCmnt  '%s'",name,pubCmnt);
}
/******************* Table description ******************************/

SeerInterfaceTableDescription::SeerInterfaceTableDescription(const char *name,const char *desc){
    tablename = strdup(name);
    tabledescription = strdup(desc);
}
SeerInterfaceTableDescription::~SeerInterfaceTableDescription(){
    delete tablename;
    delete tabledescription;
}

void SeerInterfaceTableDescription::print(){
    printf("***** TABLE DESCRIPTION *****\n");
    printf("Name '%s'	Desc '%s'\n",tablename,tabledescription);
}



/* ***************** debug functions for SeerInterface ***************/

SeerInterfaceError *SeerInterface::beginSession(const char *username, const char *userpasswd){
    printf("%s logs in with '%s'\n",username,userpasswd);
    return 0;
}

SeerInterfaceError *SeerInterface::endSession(){
    return 0;
};
    
SeerInterfaceError *SeerInterface::beginTransaction(){return 0;};
SeerInterfaceError *SeerInterface::commitTransaction(){return 0;};
SeerInterfaceError *SeerInterface::abortTransaction(){return 0;};
    
SeerInterfaceError *SeerInterface::beginReadOnlyTransaction(){return beginTransaction();};
SeerInterfaceError *SeerInterface::commitReadOnlyTransaction(){return commitTransaction();};
    
    /* ************** next functions are called only within the scope of a transaction *****/
SeerInterfaceAlignment *SeerInterface::firstAlignment(){
    SeerInterfaceAlignment *ali = new SeerInterfaceAlignment("16s","this is my comment");
    ali->typeofAlignment = SIAT_RNA;
    return ali;
}
    
SeerInterfaceAlignment *SeerInterface::nextAlignment(){ return 0; };

SeerInterfaceTableDescription *SeerInterface::firstTableDescription(){
    SeerInterfaceTableDescription *td = new SeerInterfaceTableDescription("REF","Reference Dataset");
    return td;
}

SeerInterfaceTableDescription *SeerInterface::nextTableDescription(){ return 0;}

int seer_attribut_counter = 0;

SeerInterfaceAttribute *SeerInterface::firstAttribute(){
    if (seer_attribut_counter > 10 ) return 0;
    char buffer[100];
    sprintf(buffer,"attribute_%i",seer_attribut_counter++);
    SeerInterfaceAttribute *at = new SeerInterfaceAttribute;
    at->name = strdup(buffer);
    at->pubCmnt = strdup(buffer);
    at->type = SIAT_STRING;
    at->changeable = 1;
    at->sortindex = seer_attribut_counter;
    if (seer_attribut_counter>5){
	at->changeable = 0;
    }
    at->deleteable = 1;
    at->queryable = seer_attribut_counter & 1;
    return at;
}

SeerInterfaceAttribute *SeerInterface::nextAttribute(){ return firstAttribute();};

static int seer_tattribut_counter = 0;
SeerInterfaceAttribute *SeerInterfaceTableDescription::firstAttribute(){
    if (seer_tattribut_counter > 10 ) return 0;
    char buffer[100];
    sprintf(buffer,"field_%i",seer_tattribut_counter++);
    SeerInterfaceAttribute *at = new SeerInterfaceAttribute;
    at->name = strdup(buffer);
    at->pubCmnt = strdup(buffer);
    at->type = SIAT_STRING;
    at->changeable = 1;
    at->sortindex = seer_tattribut_counter;
    if (seer_tattribut_counter>5){
	at->changeable = 0;
    }
    at->deleteable = 1;
    at->queryable = seer_attribut_counter & 1;
    return at;
}

SeerInterfaceAttribute *SeerInterfaceTableDescription::nextAttribute(){ return firstAttribute();};

SeerInterfaceError *SeerInterface::queryDatabase(const char *alignmentName, const char *attributeName,const char *attributeValue){return 0;};

long SeerInterface::numberofMatchingSequences(){ return 100; };
SeerInterfaceError *SeerInterface::setAttributeFilter(SeerInterfaceDataStringSet *requestedAttributes){ return 0; };
void SeerInterface::setOutputFormat(SeerInterfaceOutputFormat format) { return; };

static int sd_counter = 0;

SeerInterfaceSequenceData *SeerInterface::firstSequenceData(){
    if (sd_counter>200) return 0;
    char buffer[256];
    char attr_name[100];
    sprintf(buffer,"species_%i",sd_counter++);
    SeerInterfaceSequenceData *sd = new SeerInterfaceSequenceData(buffer);
    int i;
    for (i = 0;i<8;i++){
	sprintf(attr_name,"attribute_%i",i);
	sprintf(buffer,"value %i",i);
	sd->attributes.addData(new SeerInterfaceDataString(attr_name,buffer));
    }
    sd->sequences.addData(new SeerInterfaceDataString("16s","akjshxiuoqcbuyoerttyvuewcrtycerussvyuksztbq3b789"));
    sd->sequenceType = SISDT_REAL_SEQUENCE;
    return sd;
}

SeerInterfaceSequenceData *SeerInterface::nextSequenceData(){
    return firstSequenceData();
}

SeerInterfaceTableData *SeerInterface::querySingleTableData(const char *tablename,const char *tableEntryId){
    char buffer[256];
    sprintf(buffer,"Dummy data for %s %s",tablename,tableEntryId);
    SeerInterfaceTableData *td = new SeerInterfaceTableData(tableEntryId);
    td->attributes.addData(new SeerInterfaceDataString("key",buffer));
    return td;
}

SeerInterfaceSequenceData *SeerInterface::querySingleSequence(const char *alignmentName, const char *uniqueId){
    char buffer[256];
    char attr_name[100];
    sprintf(buffer,"HELIX");
    SeerInterfaceSequenceData *sd = new SeerInterfaceSequenceData(buffer);
    int i;
    for (i = 0;i<8;i++){
	sprintf(attr_name,"attribute_%i",i);
	sprintf(buffer,"atrribut number %i",i);
	sd->attributes.addData(new SeerInterfaceDataString(attr_name,buffer));
    }
    sd->sequences.addData(new SeerInterfaceDataString("16s","akjshxiuoqcbuyoerttyvuewcrtycerussvyuksztbq3b789"));
    sd->sequenceType = SISDT_FILTER;
    return sd;
}


    
SeerInterfaceError *SeerInterface::queryTableData(const char *alignmentname, const char *tablename){
    return 0;
}
static int td_counter = 0;


long SeerInterface::numberofMatchingTableData(){ return 200;};

SeerInterfaceTableData *SeerInterface::firstTableData(){
    if (td_counter>20) return 0;
    char buffer[256];
    char attr_name[100];
    sprintf(buffer,"table_entry_%i",td_counter++);
    SeerInterfaceTableData *sd = new SeerInterfaceTableData(buffer);
    int i;
    for (i = 0;i<8;i++){
	sprintf(attr_name,"field_%i",i);
	sprintf(buffer,"value %i",i);
	sd->attributes.addData(new SeerInterfaceDataString(attr_name,buffer));
    }
    return sd;
}

SeerInterfaceTableData *SeerInterface::nextTableData(){
    return firstTableData();
}    

    /* ****************** storing data in the database ***********************
     * first check then store
     * check should return very detailed error messages !!!!
     */

SeerInterfaceError *SeerInterface::checkAttributeName(const SeerInterfaceAttribute *attribute,SeerImportQualityLevel qLevel){
    return 0;
    return new SeerInterfaceError(SIET_QUALITY_ERROR,"checkAttributeName Error");
};

SeerInterfaceError *SeerInterface::checkTableData(const SeerInterfaceTableData *){ return 0;}
SeerInterfaceError *SeerInterface::storeTableData(const SeerInterfaceTableData *){ return 0;}
    

SeerInterfaceError *SeerInterface::storeAttributeName(const SeerInterfaceAttribute *attribute,SeerImportQualityLevel qLevel){ attribute->print();return 0;};

SeerInterfaceError *SeerInterface::resortAttributeList(const SeerInterfaceDataStringSet *attributeList){ return 0;};

SeerInterfaceError *SeerInterface::checkAlignmentName(const SeerInterfaceAlignment *alignment, SeerImportQualityLevel qLevel){ return 0; }
SeerInterfaceError *SeerInterface::storeAlignmentName(const SeerInterfaceAlignment *alignment, SeerImportQualityLevel qLevel){ alignment->print();return 0; }
    /* store sequences, SeerQualityLevel is taken from the datas !!! */
SeerInterfaceError *SeerInterface::checkSequence(const char *alignmentname, const SeerInterfaceSequenceData *sequence){ return 0; }
SeerInterfaceError *SeerInterface::storeSequence(const char *alignmentname, const SeerInterfaceSequenceData *sequence){ sequence->print(); return 0; }


void SeerInterface::debug_function(void){
    ;
}

/********************************* direct interface to ARB ********************************/
SeerInterfaceError *SeerInterface::readFromArbDirectly(GBDATA *gb_main){ // should use the slider
    printf("readFromArbDirectly called\n");
    return 0;
}

SeerInterfaceError *SeerInterface::writeToArbDirectly(GBDATA *gb_main){ // called after arb is fully loaded
    printf("writeToArbDirectly called\n");
    return 0;
}



#if 0
int main(int argc, char **argv){
    SeerInterfaceSequenceData sd("test");
    sd.sequences.addData(new SeerInterfaceDataString("data","adsfafdsa"));
    sd.sequences.addData(new SeerInterfaceDataString("remark","..................asdf..."));

    sd.attributes.addData(new SeerInterfaceDataString("name","ecoli"));
    sd.attributes.addData(new SeerInterfaceDataString("acc","M09234"));

    sd.print();

    SeerInterfaceData *l;
    for (l = sd.attributes.firstData(); l; l = sd.attributes.nextData()){
	l->print();
    }
    
    return 0;
}
#endif
