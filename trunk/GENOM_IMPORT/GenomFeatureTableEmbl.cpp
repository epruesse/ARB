#include "GenomFeatureTableEmbl.h"
#include "GenomUtilities.h"

GenomFeatureTableEmbl::GenomFeatureTableEmbl()
{
	prepared = false;
	actual_gene_id = 0;
	actual_gene = 0;
	genes_num = 0;
	features.push_back("attenuator");
	features.push_back("C_region");
	features.push_back("CAAT_signal");
	features.push_back("CDS");
	features.push_back("conflict");
	features.push_back("D-loop");
	features.push_back("D_segment");
	features.push_back("enhancer");
	features.push_back("exon");
	features.push_back("gap");
	features.push_back("GC_signal");
	features.push_back("gene");
	features.push_back("iDNA");
	features.push_back("intron");
	features.push_back("J_segment");
	features.push_back("LTR");
	features.push_back("mat_peptide");
	features.push_back("misc_binding");
	features.push_back("misc_difference");
	features.push_back("misc_feature");
	features.push_back("misc_recomb");
	features.push_back("misc_RNA");
	features.push_back("misc_signal");
	features.push_back("misc_structure");
	features.push_back("modified_base");
	features.push_back("mRNA");
	features.push_back("N_region");
	features.push_back("old_sequence");
	features.push_back("operon");
	features.push_back("oriT");
	features.push_back("polyA_signal");
	features.push_back("polyA_site");
	features.push_back("precursor_RNA");
	features.push_back("prim_transcript");
	features.push_back("primer_bind");
	features.push_back("promoter");
	features.push_back("protein_bind");
	features.push_back("RBS");
	features.push_back("repeat_region");
	features.push_back("repeat_unit");
	features.push_back("rep_region");
	features.push_back("rRNA");
	features.push_back("S_region");
	features.push_back("satellite");
	features.push_back("scRNA");
	features.push_back("siq_peptide");
	features.push_back("snRNA");
	features.push_back("snoRNA");
	features.push_back("source");
	features.push_back("stem_loop");
	features.push_back("STS");
	features.push_back("TATA_signal");
	features.push_back("terminator");
	features.push_back("trensit_prptide");
	features.push_back("tRNA");
	features.push_back("unsure");
	features.push_back("V_region");
	features.push_back("V_segment");
	features.push_back("variation");
	features.push_back("3'clip");
	features.push_back("3'UTR");
	features.push_back("5'clip");
	features.push_back("5'UTF");
	features.push_back("-10_signal");
	features.push_back("-35_signal");
	features.push_back("-");
	for(int i = 0; i < 80;i++)
	{
		type_ref_nums[i] = 0;
	}
}

GenomFeatureTableEmbl::~GenomFeatureTableEmbl()
{

}

GenomSourceFeatureEmbl* GenomFeatureTableEmbl::getSource(void)
{
	if(!prepared)
	{
		prepareFeatureTable();
	}
	return &source;
}

int GenomFeatureTableEmbl::featureNameToNumber(string source)
{
	for(int i = 0; i < (int)features.size();i++)
	{
		if(features[i] == source)
		{
			return i;
		}
	}
  return -1;
}

string* GenomFeatureTableEmbl::getGeneID(void)
{
	if(!prepared)
	{
		prepareFeatureTable();
	}
  if(actual_gene_id < (int)gene_ids.size())
  {
	  return &(gene_ids[actual_gene_id++]);
  }
  return NULL;
}

void GenomFeatureTableEmbl::updateContain(string* nl)
{
	row_lines.push_back(*nl);
	prepared = false;
}

GeneEmbl* GenomFeatureTableEmbl::getGeneByID(string* gid)
{
	if(!prepared)
	{
		prepareFeatureTable();
	}
	string str1 = *gid;
	for(int i = 0; i < (int)genes.size(); i++)
	{
		string str2(*genes[i].getID());
		if(str1 == str2)
		{
			return &(genes[i]);
		}
	}
  return NULL;
}

GeneEmbl* GenomFeatureTableEmbl::getGene(void)
{
	if(!prepared)
	{
		prepareFeatureTable();
	}
//  cout << "Gene " << genes.size() << endl;
	if((int)genes.size() > actual_gene)
	{
		return &(genes[actual_gene++]);
	}
	else
	{
		return NULL;	//	or null
	}
}

void GenomFeatureTableEmbl::prepareFeatureTable(void)
{
//  cout << "Hier_2" << endl;
  int rlsize = row_lines.size();
	string tstr0;
	bool newgene = true;
	bool source_open = false;
	bool gene_open = false;
//  long tlong = 0;
	int tnum = 0;
	GeneEmbl *tgene;
//  cout << "Hier_3" << endl;
	for(int i = 0; i < rlsize;i++)
	{
		tstr0 = row_lines[i];
//    cout << "Line : " << tlong++ << " = " << tstr0 << endl;
		newgene = isNewGene(tstr0);
//    cout << "Hier_4" << endl;
		if(newgene)
		{
//      cout << "Hier_5" << endl;
      if(gene_open)
			{
//        cout << "Hier_6.0" << endl;
        tgene->setGeneNumber(genes_num++);
//        cout << "Hier_6.1" << endl;
        tgene->prepareGene();
//        cout << "Hier_6.2" << endl;
        string astr3 = tgene->getType();
//        cout << "Hier_6.3 -" << astr3 << "-" << endl;
        tnum = featureNameToNumber(astr3);
//        cout << "Hier_6.4 " << tnum << endl;
        string astr = tgene->getTempLocation();
//        cout << "Hier_6.5 " << astr << endl;
        string astr2 = generateID(astr,tnum);
//        cout << "Hier_6.6 " << astr2 << endl;
        tgene->setID(astr2);
//				cout << "Hier_6.7" << endl;
				if(tnum != -1)
				{
//          cout << "Hier_8" << endl;
          type_ref_nums[tnum]++;
					tgene->setRefNumOfType(type_ref_nums[tnum]);
					genes_num++;
					tgene->setRefNumOfGene(genes_num);
          gene_ids.push_back(*(tgene->getID()));
					genes.push_back(*tgene);
          delete(tgene);
				}
				gene_open = false;
			}
			if(source_open)
			{
//        cout << "Hier_11" << endl;
        source_open = false;
			}
			if(isSource(tstr0))
			{
//        cout << "Hier_12" << endl;
        source.updateContain(tstr0);
				source.prepareSource();
				source_open = true;
				gene_open = false;
			}
			else
			{
//        cout << "Hier_13" << endl;
        tgene = new GeneEmbl;
//        cout << "Hier_14" << endl;
        tgene->updateContain(&tstr0);
//        cout << "Hier_15" << endl;
				source_open = false;
//        cout << "Hier_16" << endl;
				gene_open = true;
//        cout << "Hier_17" << endl;
  			genes_num++;
			}
		}
		else
		{
			if(source_open)
			{
				source.updateContain(tstr0);
			}
			else if(gene_open)
			{
				tgene->updateContain(&tstr0);
			}
		}
	}
//  cout << "Hier_8" << endl;
	if(gene_open)
	{
    tgene->setGeneNumber(genes_num++);
    tgene->prepareGene();
		tnum = featureNameToNumber(tgene->getType());
    tgene->setID(generateID(tgene->getTempLocation(),tnum));
		if(tnum != -1)
		{
			type_ref_nums[tnum]++;
			tgene->setRefNumOfType(type_ref_nums[tnum]);
			genes_num++;
			tgene->setRefNumOfGene(genes_num);
      gene_ids.push_back(*(tgene->getID()));
			genes.push_back(*tgene);
      gene_open = false;
      delete(tgene);
		}
	}
//  cout << "Hier_9" << endl;
  /*if(gene_open)
	{
    tgene->setGeneNumber(genes_num++);
    tgene->prepareGene();
		tnum = featureNameToNumber(tgene->getType());
    tgene->setID(generateID(tgene->getTempLocation(),tnum));
		if(tnum != -1)
		{
			type_ref_nums[tnum]++;
			tgene->setRefNumOfType(type_ref_nums[tnum]);
			genes_num++;
			tgene->setRefNumOfGene(genes_num);
      gene_ids.push_back(*(tgene->getID()));
			genes.push_back(*tgene);
      gene_open = false;
      delete(tgene);
		}
  }*/
//  cout << "Hier_10" << endl;
	prepared = true;
}
