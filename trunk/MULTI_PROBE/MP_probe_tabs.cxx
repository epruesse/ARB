#include <MultiProbe.hxx>
#include <string.h>
#include <math.h>



//**************************************************************
void probe_tabs::print()
{
    int i;


    printf("**********************\n");
    printf("GRUPPENTABELLE:\n");
    for (i=0; i< length_of_group_tabs; i++)
        printf("%d  %d \n", i, group_tab[i]);

    printf("NON_GRUPPENTABELLE:\n");
    for (i=0; i< length_of_group_tabs; i++)
        printf("%d %d\n", i, non_group_tab[i]);

    printf("**********************\n");

}

probe_tabs *probe_tabs::duplicate()
{
    int i;
    int			*new_group_field = new int[length_of_group_tabs];
    int			*new_non_group_field = new int[length_of_group_tabs];

    for (i=0; i< length_of_group_tabs; i++)
        new_group_field[i] = group_tab[i];


    for (i=0; i< length_of_group_tabs; i++)
        new_non_group_field[i] = non_group_tab[i];

    return  new probe_tabs(new_group_field, new_non_group_field, length_of_group_tabs);
}


probe_tabs::probe_tabs(int *new_group_field, int *new_non_group_field, int len_group)
{
    int length;
    memset(this,0, sizeof(probe_tabs));

    if (new_group_field)				// Duplicate !!!
    {
        group_tab = new_group_field;
        non_group_tab = new_non_group_field;
        length_of_group_tabs = len_group;
    }
    else
    {
        length = (int)(pow(3.0, (double)mp_gl_awars.no_of_probes));
        group_tab = new int[length];
        memset(group_tab, 0, sizeof(int)*length);
        non_group_tab = new int[length];
        memset(non_group_tab, 0, sizeof(int)*length);
        length_of_group_tabs = length;
    }
}

probe_tabs::~probe_tabs()
{
    delete [] group_tab;
    delete [] non_group_tab;
}
