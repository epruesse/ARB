#include "GenomUtilities.h"
#include "GenomEmbl.h"
#include <iostream>
#include <string>


using namespace std;

void showLocation(GenomLocation & loc)
{
  cout << "--In Location--" << endl;
  GenomLocation tloc;
  if(loc.isSingleValue())
  {
    cout << "GenomLocation.isSingleValue()=true" << endl;
    cout << "GenomLocation.isSmallerBegin()=" << loc.isSmallerBegin() << endl;
    cout << "GenomLocation.isBiggerBegin()=" << loc.isBiggerBegin() << endl;
    cout << "GenomLocation.isSmallerEnd()=" << loc.isSmallerEnd() << endl;
    cout << "GenomLocation.isBiggerEnd()=" << loc.isBiggerEnd() << endl;
    cout << "GenomLocation.getSingleValue()=" << loc.getSingleValue() << endl;
  }
  else
  {
    cout << "GenomLocation.isSingleValue()=false" << endl;
    cout << "GenomLocation.isRanged()=" << loc.isRanged() << endl;
    cout << "GenomLocation.isCollection()=" << loc.isCollection() << endl;
    /*if(loc.isCollection())
    {
      cout << "GenomLocation.isCollection()=true" << endl;
    }
    else
    {
      cout << "GenomLocation.isCollection()=false" << endl;
    }*/
    cout << "GenomLocation.isJoin()=" << loc.isJoin() << endl;
    cout << "GenomLocation.isComplement()=" << loc.isComplement() << endl;
    cout << "GenomLocation.isOrder()=" << loc.isOrder() << endl;
    cout << "GenomLocation.isRoof()=" << loc.isRoof() << endl;
    cout << "GenomLocation.isPoint()=" << loc.isPoint() << endl;
    if(loc.isReference())
    {
      loc.getReference();
      cout << "GenomLocation.isReference()=true" << endl;
      cout << "GenomLocation.getReference()=" << loc.getReference() << endl;
    }
    while(loc.hasMore())
    {
      tloc = loc.getNextValue();
      showLocation(tloc);
    }
  }
}

int main()
{
//  GenomEmbl embl("/nfshome/artemov/embl/AL646052.embl");
//  GenomEmbl embl("/nfshome/artemov/embl/BA000007.embl");
//  GenomEmbl embl("/nfshome/artemov/embl/BA000018.embl");
//  GenomEmbl embl("/nfshome/artemov/embl/BA000037.embl");
//  GenomEmbl embl("/nfshome/artemov/embl/BA000038.embl");
//  GenomEmbl embl("/nfshome/artemov/embl/BX119912.embl");
  GenomEmbl embl("/nfshome/artemov/embl/L43967.embl");
  embl.prepareFlatFile();
  cout << endl;
  cout << endl;
  cout << endl;
  cout << endl;
  cout << "------------Interessant------------" << endl;
/*  cout << "GenomEmbl.getID()=" << *(embl.getID()) << endl;
  cout << "GenomEmbl.getAC()=" << *(embl.getAC()) << endl;
  cout << "GenomEmbl.getSV()=" << *(embl.getSV()) << endl;
  cout << "GenomEmbl.getDTCreation()=" << *(embl.getDTCreation()) << endl;
  cout << "GenomEmbl.getDTLastUpdate()=" << *(embl.getDTLastUpdate()) << endl;
  cout << "GenomEmbl.getDE()=" << *(embl.getDE()) << endl;
  cout << "GenomEmbl.getKW()=" << *(embl.getKW()) << endl;
  cout << "GenomEmbl.getOS()=" << *(embl.getOS()) << endl;
  cout << "GenomEmbl.getOC()=" << *(embl.getOC()) << endl;
  cout << "GenomEmbl.getOG()=" << *(embl.getOG()) << endl;
  cout << "GenomEmbl.getDR()=" << *(embl.getDR()) << endl;
  cout << "GenomEmbl.getCO()=" << *(embl.getCO()) << endl;
  cout << "GenomEmbl.getCC()=" << *(embl.getCC()) << endl;
  cout << "GenomEmbl.getSQ()=";
  cout << embl.getSQ() << " " << embl.getSQ() << " " << embl.getSQ() << " " << embl.getSQ();
  cout << " " << embl.getSQ() << " " << embl.getSQ() << endl;
  GenomReferenceEmbl *tref;
  cout << "--------References--------" << endl;  
  while((tref = embl.getReference()) != NULL)
  {
    cout << "-----Next Reference-----" << endl;  
    cout << "GenomReferenceEmbl.getRN()=";
    cout << tref->getRN() << endl;
    cout << "GenomReferenceEmbl.getRC()=" << *(tref->getRC()) << endl;
    cout << "GenomReferenceEmbl.getRPBegin()=" << tref->getRPBegin() << endl;
    cout << "GenomReferenceEmbl.getRPEnd()=" << tref->getRPEnd() << endl;
    cout << "GenomReferenceEmbl.getRX()=" << *(tref->getRX()) << endl;
    cout << "GenomReferenceEmbl.getRG()=" << *(tref->getRG()) << endl;
    cout << "GenomReferenceEmbl.getRA()=" << *(tref->getRA()) << endl;
    cout << "GenomReferenceEmbl.getRT()=" << *(tref->getRT()) << endl;
    cout << "GenomReferenceEmbl.getRL()=" << *(tref->getRL()) << endl;
  }
  delete(tref);*/
  cout << "--------References Ende--------" << endl;  
  GenomFeatureTableEmbl *ttable;
  GenomSourceFeatureEmbl *tsource;
  ttable = embl.getFeatureTable();
  tsource = ttable->getSource();
  const string *astr;
  cout << "-----Source-----" << endl;  
  cout << "Begin : " << tsource->getBegin() << endl;
  cout << "End : " << tsource->getEnd() << endl;
  while((astr = tsource->getQualifier()) != NULL)
  {
    cout << "Qualifier=Value : " << *astr;
    cout << "=" << *(tsource->getValue(astr)) << endl;
  }
  cout << "-----Genes-----" << endl;
  GeneEmbl *tgene;
  GenomLocation gloc;
  int cc = 0;
  while((tgene = ttable->getGene()) != NULL)
  {
    cout << "--New Gene--" << ++cc << endl;
    cout << "GeneEmbl.getID()=" << *(tgene->getID()) << endl;
    cout << "GeneEmbl.getType()=" << tgene->getType() << endl;
    cout << "GeneEmbl.getRefNumOfType()=" << tgene->getRefNumOfType() << endl;
    cout << "GeneEmbl.getRefNumOfGene()=" << tgene->getRefNumOfGene() << endl;
    cout << "GeneEmbl.getTempLocation()=" << tgene->getTempLocation() << endl;
    gloc = tgene->getGeneLocation();
    cout << "---Locations---" << endl;
    showLocation(gloc);
    cout << "---Locations End---" << endl;
  }
  cout << "------------Interessant Ende------------" << endl;  
	return 0;
}
