#ifndef SEQ_QUALITY_H
#define SEQ_QUALITY_H


void       SQ_create_awars(AW_root *awr, AW_default aw_def);
AW_window *SQ_create_seq_quality_window(AW_root *aw_root, AW_CL);

#else
#error seq_quality.h included twice
#endif // SEQ_QUALITY_H

