#include <MultiProbe.hxx>
#include <string.h>

BOOL GenerationDuplicates::insert(probe_combi_statistic *sondenkombi, BOOL &result, int depth)		//initial muss result TRUE sein
{
    int max_depth = mp_gl_awars.no_of_probes;

    if (depth == max_depth)
    {
        result = FALSE;
        return FALSE;
    }

    if (! next[sondenkombi->get_probe_combi(depth)->probe_index])		//sonde muss auf alle Faelle bis zuletzt eingetragen werden
    {
        if (depth == max_depth-1)
        {
            next[sondenkombi->get_probe_combi(depth)->probe_index] = new GenerationDuplicates(1);
            next_mism[sondenkombi->get_probe_combi(depth)->allowed_mismatches] = 1;
            return TRUE;
        }
        else
        {
            next[sondenkombi->get_probe_combi(depth)->probe_index] = new GenerationDuplicates(intern_size);
            next_mism[sondenkombi->get_probe_combi(depth)->allowed_mismatches] = 1;
            return next[sondenkombi->get_probe_combi(depth)->probe_index]->insert(sondenkombi, result, depth+1);
        }
    }

    result = result && next_mism[sondenkombi->get_probe_combi(depth)->allowed_mismatches];
    next[sondenkombi->get_probe_combi(depth)->probe_index]->insert(sondenkombi, result, depth+1);	//man kann erst ganz unten entscheiden, ob doppelt oder nicht
    return result;
}

GenerationDuplicates::GenerationDuplicates(int size)		//size muss die Groesse des Sondenarrays in ProbeValuation enthalten
{
    intern_size = size;
    next = new GenerationDuplicates*[size];
    next_mism = new int[MAXMISMATCHES];
    memset(next_mism, 0, MAXMISMATCHES * sizeof(int));
    memset(next, 0, size * sizeof(GenerationDuplicates*));
}

GenerationDuplicates::~GenerationDuplicates()
{
    for (int i=0; i<intern_size; i++)
        delete next[i];

    delete next_mism;
    delete next;
}

