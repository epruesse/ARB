#ifndef SERGEII_H
#define SERGEII_H

#include "utilities.h"

int My_Function(int argc, char **argv);
int My_main(int argc, char **argv);
void PhyTime_XML(char *xml_file);
phydbl TIMES_Calib_Cond_Prob(t_tree *tree);
int Number_Of_Comb(t_cal *calib);
int Number_Of_Calib(t_cal *calib);
void Check_Node_Time(t_node *a, t_node *d, int *result, t_tree *tree);
void Set_Current_Calibration(int row, t_tree *tree);
void Random_Calibration(t_tree *tree);
int RND_Calibration_And_Node_Number(t_tree *tree);
phydbl Randomize_One_Node_Time(phydbl min, phydbl max);
void Lk_Hastings_Ratio_Times(t_node *a, t_node *d, phydbl *tot_prob, t_tree *tree);
void Update_Descendent_Cond_Jump(t_node *a, t_node *d, phydbl *L_Hast_ratio, t_tree *tree);
void Update_Ancestor_Cond_Jump(t_node *d, phydbl *L_Hast_ratio, t_tree *tree);
void Update_Times_RND_Node_Ancestor_Descendant(int rnd_node, phydbl *L_Hast_ratio, t_tree *tree);
void Update_Times_Down_Tree(t_node *a, t_node *d, phydbl *L_Hastings_ratio, t_tree *tree);
phydbl Slicing_Calibrations(t_tree *tree);
int Number_Of_Comb_Slices(int m, int num_elem, int *n_slice);
void Check_Time_Slices(t_node *a, t_node *d, int *result, phydbl *t_cur_slice_min, phydbl *t_cur_slice_max, t_tree *tree);
void Number_Of_Nodes_In_Slice(t_node *d_start, t_node *d, int *n, phydbl *t_cur_slice_min, phydbl *t_cur_slice_max, t_tree *tree);
void Search_Root_Node_In_Slice(t_node *d_start, t_node *d, int *root_nodes, int *num_elem, phydbl t_slice_min, phydbl t_slice_max, phydbl *t_cur_slice_min, phydbl *t_cur_slice_max, t_tree *tree);
int Factorial(int base);
phydbl *Norm_Constant_Prior_Times(t_tree *tree);
void TIMES_Calib_Partial_Proba(t_tree *tree);
xml_node *XML_Search_Node_Attribute_Value_Clade(char *attr_name, char *value, int skip, xml_node *node);
char **XML_Reading_Clade(xml_node *n_clade, t_tree *tree);
int XML_Number_Of_Taxa_In_Clade(xml_node *n_clade);
void TIMES_Set_All_Node_Priors_S(int *result, t_tree *tree);
void TIMES_Set_All_Node_Priors_Bottom_Up_S(t_node *a, t_node *d, int *result, t_tree *tree);
void TIMES_Set_All_Node_Priors_Top_Down_S(t_node *a, t_node *d, int *result, t_tree *tree);
#endif

