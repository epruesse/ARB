
/****************************************************************
 *
 *   This is a set of functions defined for the genome 
 *   project.
 *
 ****************************************************************/


#ifndef _GLOBAL_DEFS_H
#define _GLOBAL_DEFS_H
#include "global_defs.h"
#endif

#define MAXLINELEN   256

static char Default_DNA_Trans[16] = {
    '-', 'a','c','m','g','r','s','v','t','w','y','h','k','d','b','n' };


/***********
 *
 * WriteRecord() outputs one record at a time in HGL format.
 * Only the fields in the fields_array will be output.  All the 
 * fields will be output if fields_array is NULL.
 *
 * fp :          pointer to the output file.
 * tSeq:         pointer to the record.
 * fields_array: contains the field ids of the selected fields.
 * array_size:   number of selected fields.
 *
 * Returns:   1 if any field is printed;
 *            0 if no field is printed;
 *           -1 if anything is wrong.
 *
 **********/

int
WriteRecord(fp, tSeq, fields_array, array_size)
     FILE *fp;
     const Sequence *tSeq;
     int *fields_array;
     int array_size;
{
    int i, save_str_size, tt;
    int all_fields = FALSE;
    int first_field = TRUE;
    char temp_str[256];
    char *save_str;
    char *ptr;

    save_str = (char *)Calloc(256, 1);
    save_str_size = 256;
    
    /* When all the fields are selected. */
    if(fields_array == NULL)
    {
        all_fields = TRUE;
        fields_array = (int *)Calloc(NUM_OF_FIELDS, sizeof(int));

        for(i=0; i<NUM_OF_FIELDS; i++)
        {
            fields_array[i] = i;
        }
        array_size = NUM_OF_FIELDS;
    }

    for (i = 0; i < array_size; i++)
    {
        save_str[0]='\0';

        if (fields_array[i] == e_creation_date &&
            tSeq->creation_date[0] != 0 )
        {
            sprintf(save_str,"\n%s\t%d/%d/%d  ",
                    at[fields_array[i]],
                    tSeq->creation_date[1],
                    tSeq->creation_date[2],
                    tSeq->creation_date[0]);

            if(tSeq->creation_date[3]>=0)
            {
                if(tSeq->creation_date[4] < 0) 
                    tSeq->creation_date[4] = 0;
                if(tSeq->creation_date[5] < 0) 
                    tSeq->creation_date[5] = 0;
                sprintf(save_str, "%s%d:%d:%d",
                        save_str,
                        tSeq->creation_date[3],
                        tSeq->creation_date[4],
                        tSeq->creation_date[5]);
            }
        }
        else if (fields_array[i] == e_probing_date &&
                 tSeq->probing_date[0] != 0 )
        {
            sprintf(save_str,"\n%s\t%d/%d/%d  ",
                    at[fields_array[i]],
                    tSeq->probing_date[1],
                    tSeq->probing_date[2],
                    tSeq->probing_date[0]);

            if(tSeq->probing_date[3]>=0)
            {
                if(tSeq->probing_date[4] < 0) 
                    tSeq->probing_date[4] = 0;
                if(tSeq->probing_date[5] < 0) 
                    tSeq->probing_date[5] = 0;
                sprintf(save_str, "%s%d:%d:%d",
                        save_str,
                        tSeq->probing_date[3],
                        tSeq->probing_date[4],
                        tSeq->probing_date[5]);
            }
        }
        else if (fields_array[i] == e_autorad_date &&
                 tSeq->autorad_date[0] != 0 )
        {
            sprintf(save_str,"\n%s\t%d/%d/%d  ",
                    at[fields_array[i]],
                    tSeq->autorad_date[1],
                    tSeq->autorad_date[2],
                    tSeq->autorad_date[0]);

            if(tSeq->autorad_date[3]>=0)
            {
                if(tSeq->autorad_date[4] < 0) 
                    tSeq->autorad_date[4] = 0;
                if(tSeq->autorad_date[5] < 0) 
                    tSeq->autorad_date[5] = 0;
                sprintf(save_str, "%s%d:%d:%d",
                        save_str,
                        tSeq->autorad_date[3],
                        tSeq->autorad_date[4],
                        tSeq->autorad_date[5]);
            }
        }
        else if ( fields_array[i] == e_c_elem &&
                  tSeq->c_elem != NULL )
        {
            ptr = tSeq->c_elem;
            sprintf(save_str,"\n%s\t\"",at[fields_array[i]]);
            while ( ptr < tSeq->c_elem + tSeq->seqlen )
            {
                if ( ptr != tSeq->c_elem )
                    strcat(save_str,"\n");
                strncpy(temp_str, ptr, MIN(60, tSeq->c_elem +tSeq->seqlen-ptr));
                temp_str[MIN(60, tSeq->c_elem+tSeq->seqlen - ptr)] = '\0';

                /* Gurantee strlen(temp_str) chars for the string, 
                 * one for \n,  one for ", and one for \0. 
                 */
                while(save_str_size - strlen(save_str) < strlen(temp_str)+3)
                {
                    save_str_size *= 2;
                    save_str = (char *)Realloc(save_str,save_str_size);
                }
                strcat(save_str, temp_str);
                ptr += 60;
            }
            strcat(save_str,"\"");
        }
        else if ( fields_array[i] == e_comments &&
                  tSeq->commentslen != 0)
        {
            while(save_str_size < 20+tSeq->commentslen)
            {
                save_str_size *= 2;
                save_str = (char *)Realloc(save_str,save_str_size);
            }
	    
            strcat(save_str,"\n");
            strcat(save_str,at[fields_array[i]]);
            strcat(save_str,"\t\"\n");
	    
            /* put a \0 at the end of comments. */
            while(tSeq->commentslen + 1 > tSeq->commentsmaxlen)
            {
                tSeq->commentsmaxlen *= 2;
                tSeq->comments = (char *)
                    Realloc(tSeq->comments,
                            tSeq->commentsmaxlen);
            }
            tSeq->comments[tSeq->commentslen] = '\0';
		
            /* clean up the leading empty lines.*/
            tt = 0;
            while(tSeq->comments[tt] == '\n' || tSeq->comments[tt] == ' ')
                tt++;
            tSeq->commentslen -= tt;
            strcat(save_str,tSeq->comments+tt);
            strcat(save_str,"\"");
        }
        else if (fields_array[i] == e_laneset && tSeq->laneset != -1)
            sprintf(save_str,"\n%s\t\t%d",
                    at[fields_array[i]],tSeq->laneset);
        else if (fields_array[i] == e_strandedness && tSeq->strandedness != 0)
            sprintf(save_str,"\n%s\t%d",
                    at[fields_array[i]],tSeq->strandedness);
        else if (fields_array[i] == e_direction && tSeq->direction != 0)
            sprintf(save_str,"\n%s\t%d",
                    at[fields_array[i]],tSeq->direction);
        else if (fields_array[i] == e_orig_strand && tSeq->orig_strand != 0)
            sprintf(save_str,"\n%s\t%d",
                    at[fields_array[i]],tSeq->orig_strand);
        else if (fields_array[i] == e_orig_direction && tSeq->orig_direction != 0)
            sprintf(save_str,"\n%s\t%d",
                    at[fields_array[i]],tSeq->orig_direction);
        else if (fields_array[i] == e_offset)
            sprintf(save_str,"\n%s\t\t%d",
                    at[fields_array[i]],tSeq->offset);
        else if (fields_array[i] == e_group_number && tSeq->group_number != 0)
            sprintf(save_str,"\n%s\t%d",
                    at[fields_array[i]],tSeq->group_number);
        else if (fields_array[i] == e_group_ID)
            sprintf(save_str,"\n%s\t%d",
                    at[fields_array[i]],tSeq->group_ID);
        else if (fields_array[i] == e_type && tSeq->type[0] != '\0' )
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->type);
        else if (fields_array[i] == e_barcode && tSeq->barcode[0] != '\0' )
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->barcode);
        else if (fields_array[i] == e_name && tSeq->name[0] != '\0' )
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->name);
        else if (fields_array[i] == e_status && tSeq->status[0] != '\0' )
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->status);
        else if (fields_array[i] == e_walk && tSeq->walk[0] != '\0' )
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->walk);
        else if (fields_array[i] == e_sequence_ID && 
                 tSeq->sequence_ID[0] != '\0' )
            sprintf(save_str,"\n%s\t\"%s\"",
                    at[fields_array[i]],tSeq->sequence_ID);
        else if (fields_array[i] == e_creator  && tSeq->creator[0] != '\0')
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->creator);
        else if (fields_array[i]==e_film  && tSeq->film[0]!='\0')
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->film);
        else if (fields_array[i] == e_membrane  && tSeq->membrane[0] != '\0')
            sprintf(save_str,"\n%s\t\"%s\"",
                    at[fields_array[i]],tSeq->membrane);
        else if (fields_array[i] == e_source_ID  && tSeq->source_ID[0] != '\0')
            sprintf(save_str,"\n%s\t\"%s\"",
                    at[fields_array[i]],tSeq->source_ID);
        else if (fields_array[i] == e_contig  && tSeq->contig[0] != '\0')
            sprintf(save_str,"\n%s\t\t\"%s\"",
                    at[fields_array[i]],tSeq->contig);
        else if (fields_array[i] == e_baggage && tSeq->baglen != 0)
        {
            if(save_str_size < tSeq->baglen+2)
            {
                save_str_size = tSeq->baglen+2;
                save_str = (char *)Realloc(save_str,save_str_size);
            }
	    
            save_str[0] = '\n';
            save_str[1] = '\0';

            /* put a \0 at the end of baggage. */
            strncat(save_str, tSeq->baggage, tSeq->baglen);
            while(save_str[tSeq->baglen-1] == '\n')
            {
                tSeq->baglen--;
            }
            save_str[tSeq->baglen] = '\0';
        }
        if(save_str[0] != '\0')
        {
            if (first_field == TRUE) 
            {
                first_field = FALSE; 
                fprintf(fp,"{");
            }
            fprintf(fp,"%s",save_str);
        }
    }

    if (first_field == FALSE)
    {
        fprintf(fp,"\n}\n");
    }

    if(all_fields == TRUE && fields_array != NULL)
    {
        Cfree(fields_array);
        fields_array = NULL;
    }
    if(save_str != NULL)
    {
        Cfree(save_str);
        save_str = NULL;
    }

    if (first_field == TRUE)
        return 0;
    else 
        return 1;
}



/*********
 *
 * ReadRecord() reads one record from fp into tSeq.  fp remains at
 * the finishing position so that next time when ReadRecord() is 
 * called, it reads the next record.
 *
 * The caller program should LOCATE MEMORY for the tSeq before calling.
 *
 * ReadRecord() returns:
 *           TRUE if no error; 
 *           FALSE if anything is wrong
 *           -1 if end-of-file is reached
 *
 **********/

int
ReadRecord(fp, tSeq)
     FILE *fp;
     Sequence *tSeq;
{
    char field_name[20], line[256], orig_line[256];
    int  temp_str_size, start, end, l, max_len = 255;
    char *fgets_ret, *temp_str, *fgets_ret1;
    int start_rec = FALSE;
    int need_to_read = TRUE;
    char started = 'F'; 
    void InitRecord();
    void FreeRecord();

    temp_str = (char *)Calloc(256, 1);
    temp_str_size = 256;

    InitRecord(tSeq);

    if(tSeq->c_elem == NULL)
    {
        tSeq->c_elem = (char *)Calloc(256, 1);
        tSeq->seqmaxlen = 256;
    }
    tSeq->c_elem[0] = '\0';


    /* read file line-by-line. */
    while (need_to_read == TRUE && 
           ((fgets_ret = fgets(line, max_len, fp)) != NULL || 
            start_rec == TRUE))
    {
        strcpy(orig_line, line);
        end = strlen(line) -1;
        while(end>=0 && (line[end] == ' '  || 
                         line[end] == '\t' ||
                         line[end] == ','  ||
                         line[end] == '\n') )
            end--;

        /* ignore empty lines. */
        if(end == -1)
            continue;

        if(line[end] == '{')
            started = 'T';
	    
        /* to ignore the lines between a } and a {. */
        while(started == 'F' && fgets_ret != NULL)
        {
            fgets_ret = fgets(line, max_len, fp);
            strcpy(orig_line, line);
            end = strlen(line) -1;
            while(end>=0 && (line[end] == ' '  || 
                             line[end] == '\t' ||
                             line[end] == ','  ||
                             line[end] == '\n') )
                end--;

            /* ignore empty lines. */
            if(end == -1)
                continue;

            if(line[end] == '{')
                started = 'T';
        }

        if(fgets_ret == NULL)
            return -1;

        if (end < 0)
        {
        }
        else if ((line[end] == '}') && (end==0))
        {
            start_rec = FALSE;
            need_to_read = FALSE;
        }
        else if (line[end] == '{' && end <= 10)
        {
            start_rec = TRUE;
        }
        else 
        {
            if (line[end]=='}')
            {
                need_to_read = FALSE;
                start_rec = FALSE;
            }

            /* locate the tag. */ 
            start = 0;
            while(line[start] == ' ' || 
                  line[start] == '\t'|| 
                  line[start] == '\n'|| 
                  line[start] == '{' ) 
                start++;
	    
            end = start +1;
            while(line[end] != ' ' && 
                  line[end] != '\t' &&
                  line[end] != '\n' &&
                  line[end] != '\0')
                end++;
            strncpy(field_name, line+start, end-start);
            field_name[end-start] = '\0';
	    
            /* process the field value. */
	    
            /*
             * creation_date, probing_date, or autorad_date
             */
	    
            if ( strcmp(field_name,"creation-date") == 0)
            {
                while(!isdigit(line[end]))
                    end++;
                if(strToDate(line + end, tSeq->creation_date) == -1)
                {
                    return FALSE;
                }
            }
            else if (strcmp(field_name,"probing-date") == 0)
            {
                while(line[end] != '\0' && !isdigit(line[end]))
                    end++;
		
                if(line[end] != '\0' &&
                   strToDate(line + end, tSeq->probing_date) == -1)
                {
                    return FALSE;
                }
            }
            else if ( strcmp(field_name,"autorad-date") == 0)
            {
                while(line[end] != '\0' && !isdigit(line[end]))
                    end++;
                if(line[end] != '\0' && 
                   strToDate(line + end, tSeq->autorad_date) == -1)
                {
                    return FALSE;
                }
            }
		
            /* 
             * sequence or comments.
             */
	    
            else if (strcmp(field_name,"sequence") == 0 ||
                     strcmp(field_name,"comments") == 0 )
            {
                temp_str[0] = '\0';
		
                /* locate the first ". */
                while(line[end++] != '"');
                start = end;
                end = strlen(line);

                /* ---"\n\0. */
                if(line[end-2] == '"')
                    end -= 2;
                else if(line[end-1] == '\n' && 
                        strcmp(field_name,"sequence") == 0)
                    end--;

                while(temp_str_size < end-start+1 )
                {
                    temp_str_size *= 2;
                    temp_str = (char *)Realloc(temp_str, temp_str_size);
                }
                if(end - start > 0)
                    strncat(temp_str, line+start, end-start);
		
                /* Read the second line of the seq. or comments, if any.
                   end-start<0 is the case that " is the only char this line.*/
                if (line[strlen(line)-2] != '"' || end-start<0)
                {
                    while((fgets_ret1 = fgets(line, max_len, fp)) != NULL)
                    {
                        /* IGNORE empty lines. 5/4/92 */
                        int empty_line = 0;
                        while(line[empty_line] == ' ')
                            empty_line++;
                        if(line[empty_line] == '\n')
                        {
                            continue;
                            /* strncat(temp_str, line, end); 5/4/92 */
                        }
			
                        l = strlen(line) -1;
                        if(line[l-1] == '"')
                            end = l-1;
                        else
                            end = l;

                        if(line[end] == '\n' && 
                           strcmp(field_name,"comments") == 0)
                            end++;

                        /*  Gurantee 'end' chars for the string, one for ", 
                         *  and one for \0.
                         */
                        while(temp_str_size - strlen(temp_str) < end+3 )
                        {
                            temp_str_size *= 2;
                            temp_str=(char *)Realloc(temp_str,temp_str_size);
                        }
                        strncat(temp_str, line, end);

                        if(line[l-1] == '"')
                            break;
                    }
                    if(fgets_ret1 == NULL && need_to_read == TRUE)
                    {
                        fprintf(stderr, "ReadRecord(): incomplete record.\n");
                        return FALSE;
                    }
                }
		
                l = strlen(temp_str);
                if(strcmp(field_name,"comments") == 0 )
                {
                    if(tSeq->commentsmaxlen == 0)
                    {
                        tSeq->comments = (char *)Calloc(l+1, 1);
                        tSeq->commentsmaxlen = l+1;
                    }
                    else
                    {
                        while(tSeq->commentslen+l+1>tSeq->commentsmaxlen)
                        {
                            tSeq->commentsmaxlen *= 2;
                            tSeq->comments = (char *)
                                Realloc(tSeq->comments, tSeq->commentsmaxlen);
                        }
                    }
                    tSeq->comments[tSeq->commentslen] = '\0';
                    strcat(tSeq->comments, temp_str);
                    tSeq->commentslen += l;
                }
                else  /* it is the sequence. */
                {
                    if(tSeq->seqmaxlen == 0)
                    {
                        tSeq->c_elem = (char *)Calloc(l+1, 1);
                    }
                    else if(l+1>tSeq->seqmaxlen)
                    {
                        tSeq->c_elem = (char *)Realloc(tSeq->c_elem, l+1);
                    }
                    tSeq->seqmaxlen = l+1;
                    tSeq->seqlen = l;
                    strcpy(tSeq->c_elem, temp_str);
                }
            }

            /* 
             * Integer or String.
             */
		
            else 
            {
                /* locate the value: a string or an integer. */
		
                while(line[end] == ' ' || line[end] == '\t')
                    end++;
                if (line[end] == '"')
                {
                    /* It is a string. */
                    end++;
                    start = end;
                    while(line[end] != '\0' && line[end] != '"')
                        end++;
                    /* 
                     * strncat will not put a \0 at the end of a string
                     * if the copying string is longer than n.
                     */
                    line[end++] = '\0';
                }
                else
                {
                    /* It is an integer. */
                    start = end;
                    while(line[end] != ' '  &&
                          line[end] != '\t' &&
                          line[end] != '\n' &&
                          line[end] != '\0')
                        end++;
                    strncpy(temp_str, line+start, end-start+1); /*4/26 add 1*/
                    temp_str[end-start] = '\0';
                }
		    
                /* assign to an integer field. */
                if (strcmp(field_name,"laneset") == 0 )
                    tSeq->laneset = atoi(temp_str);
                else if (strcmp(field_name,"strandedness") == 0 )
                    tSeq->strandedness = atoi(temp_str);
                else if (strcmp(field_name,"direction") == 0)
                    tSeq->direction = atoi(temp_str);
                else if (strcmp(field_name,"orig_strand") == 0 )
                    tSeq->orig_strand = atoi(temp_str);
                else if (strcmp(field_name,"orig_direction") == 0 )
                    tSeq->orig_direction = atoi(temp_str);
                else if (strcmp(field_name,"offset") == 0 )
                    tSeq->offset = atoi(temp_str);
                else if (strcmp(field_name,"group-number") == 0 )
                    tSeq->group_number = atoi(temp_str);
                else if (strcmp(field_name,"group-ID") == 0 )
                    tSeq->group_ID = atoi(temp_str);
		
                /* assign to a string field. */
                else if (strcmp(field_name,"type") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->type, line+start, end-start);
                    tSeq->type[end-start] = '\0';
                }
                else if (strcmp(field_name,"barcode") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->barcode, line+start, end-start);
                    tSeq->barcode[end-start] = '\0';
                }
                else if (strcmp(field_name,"name") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->name, line+start, end-start);
                    tSeq->name[end-start] = '\0';
                }
                else if (strcmp(field_name,"status") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->status, line+start, end-start);
                    tSeq->status[end-start] = '\0';
                }
                else if (strcmp(field_name,"walk") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->walk, line+start, end-start);
                    tSeq->walk[end-start] = '\0';
                }
                else if (strcmp(field_name,"sequence-ID") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->sequence_ID, line+start, end-start);
                    tSeq->sequence_ID[end-start] = '\0';
                }
                else if (strcmp(field_name,"creator") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->creator, line+start, end-start);
                    tSeq->creator[end-start] = '\0';
                }
                else if (strcmp(field_name,"film") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->film, line+start, end-start);
                    tSeq->film[end-start] = '\0';
                }
                else if (strcmp(field_name,"membrane") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->membrane, line+start, end-start);
                    tSeq->membrane[end-start] = '\0';
                }
                else if (strcmp(field_name,"source-ID") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->source_ID, line+start, end-start);
                    tSeq->source_ID[end-start] = '\0';
                }
                else if (strcmp(field_name,"contig") == 0 )
                {
                    if(end - start > 31) end = start + 31;
                    strncpy(tSeq->contig, line+start, end-start);
                    tSeq->contig[end-start] = '\0';
                }
                else
                {
                    if(tSeq->bagmaxlen == 0)
                    {
                        tSeq->bagmaxlen = 4*strlen(orig_line);
                        tSeq->baggage = (char *)Calloc(tSeq->bagmaxlen, 1);
                    }
                    else
                    {
                        while(tSeq->bagmaxlen<tSeq->baglen+2+strlen(orig_line))
                        {
                            tSeq->bagmaxlen *= 2;
                            tSeq->baggage = (char *)
                                Realloc(tSeq->baggage, tSeq->bagmaxlen);
                        }
                    }
                    if(tSeq->baglen == 0)
                    {
                        /*
                          tSeq->baggage[0] = '\n';
                          tSeq->baggage[1] = '\0';
                          tSeq->baglen = 1;
                        */
                        tSeq->baggage[0] = '\0';
                    }

                    /* strcat(tSeq->baggage, "\n");*/
                    strcat(tSeq->baggage, orig_line);	
                    tSeq->baglen += strlen(orig_line);
                }
            }
        }
    }

    if(temp_str != NULL)
    {
        Cfree(temp_str);
        temp_str = NULL;
    }

    if ( start_rec == FALSE && fgets_ret == NULL)
    {
        /* end of file, did not get a record. */
        return -1;
    }
    else
        return TRUE;
}


/*********
 *
 * Initialize a record.
 *
 * Note: no memory allocation is performed.
 *
 **********/

void 
InitRecord(tSeq)
     Sequence *tSeq;
{
    int i;
    
    strcpy(tSeq->type, "DNA");
    tSeq->barcode[0]     = '\0';
    tSeq->name[0]        = '\0';
    tSeq->status[0]      = '\0';
    strcpy(tSeq->walk, "FALSE");
    tSeq->sequence_ID[0] = '\0';

    tSeq->c_elem = NULL;
    tSeq->seqlen    = 0;
    tSeq->seqmaxlen = 0;
    
    for (i = 0; i<6; i++)
    {
        tSeq->creation_date[i] = 0;
        tSeq->probing_date[i] = 0;
        tSeq->autorad_date[i] = 0;
    }

    tSeq->creator[0]    = '\0';
    tSeq->film[0]       = '\0';
    tSeq->membrane[0]   = '\0';
    tSeq->source_ID[0]     = '\0';
    tSeq->contig[0]     = '\0';
    tSeq->laneset       = -1;
    tSeq->direction     = 1;    /* (1/-1/0),default:  5 to 3. */
    tSeq->strandedness  = 1;    /* (1/2/0), default:  primary.*/
    tSeq->orig_direction= 0;    /* (0 unknown, -1:3'->5', 1:5'->3')    */
    tSeq->orig_strand   = 0;    /* (0 unknown, 1:primary, 2:secondary) */
    tSeq->offset        = 0;

    tSeq->comments = NULL;
    tSeq->commentslen    = 0;
    tSeq->commentsmaxlen = 0;

    tSeq->baggage = NULL;
    tSeq->baglen         = 0;
    tSeq->bagmaxlen      = 0;
    tSeq->group_number   = 0;
    tSeq->group_ID       = 0;
}



void 
CopyRecord(to, from)
     Sequence *from, *to;
{
    int i;

    InitRecord(to);

    strcpy(to->type, from->type);

    strcpy(to->barcode, from->barcode);
    strcpy(to->name, from->name);
    strcpy(to->status,from->status);
    strcpy(to->walk,from->walk);
    strcpy(to->sequence_ID, from->sequence_ID);

    if(from->c_elem != NULL)
    {
        to->seqlen = from->seqlen;
        to->seqmaxlen = from->seqmaxlen;
        to->c_elem = (char *)Calloc(to->seqmaxlen, 1);
        strncpy(to->c_elem, from->c_elem, to->seqlen);
        to->c_elem[to->seqlen] = '\0';
    }
    
    for (i = 0; i<6; i++)
    {
        to->creation_date[i] = from->creation_date[i];
        to->probing_date[i] = from->probing_date[i];
        to->autorad_date[i] = from->autorad_date[i];
    }

    strcpy(to->creator, from->creator);
    strcpy(to->film, from->film);
    strcpy(to->membrane, from->membrane);
    strcpy(to->source_ID, from->source_ID);
    strcpy(to->contig, from->contig);
    to->laneset = from->laneset;
    to->strandedness = from->strandedness;
    to->orig_direction = from->orig_direction;
    to->orig_strand = from->orig_strand;
    to->direction = from->direction;
    to->offset = from->offset;

    if(from->comments != NULL)
    {
        to->commentsmaxlen = from->commentsmaxlen;
        to->commentslen = from->commentslen;
        to->comments = (char *)Calloc(to->commentsmaxlen, 1);
        strncpy(to->comments, from->comments, to->commentslen);
        to->comments[to->commentslen] = '\0';
    }

    if(from->baggage != NULL)
    {
        to->baglen = from->baglen;
        to->bagmaxlen = from->bagmaxlen;
        to->baggage = (char *)Calloc(to->bagmaxlen, 1);
        strncpy(to->baggage, from->baggage, to->baglen);
        to->baggage[to->baglen] = '\0';
    }
    
    to->group_number = from->group_number;
    to->group_ID = from->group_ID;
}




/*********
 *
 * Clean the contents of a record without changing the memory size.
 *
 **********/

void 
CleanRecord(tSeq)
     Sequence *tSeq;
{
    int i;
    
    strcpy(tSeq->type, "DNA");
    tSeq->name[0]        = '\0';
    tSeq->barcode[0]     = '\0';
    tSeq->status[0]      = '\0';
    strcpy(tSeq->walk, "FALSE");
    tSeq->sequence_ID[0] = '\0';

    if(tSeq->c_elem != NULL)
        tSeq->c_elem[0] = '\0';
    tSeq->seqlen    = 0;
    
    for (i = 0; i<6; i++)
    {
        tSeq->creation_date[i] = 0;
        tSeq->probing_date[i] = 0;
        tSeq->autorad_date[i] = 0;
    }

    tSeq->creator[0]    = '\0';
    tSeq->film[0]       = '\0';
    tSeq->membrane[0]   = '\0';
    tSeq->source_ID[0]     = '\0';
    tSeq->contig[0]     = '\0';
    tSeq->laneset       = -1;
    tSeq->strandedness  = 1;    /* (1/2/0), default.  primary. */
    tSeq->direction     = 1;    /* (1/-1/0),default.  5 to 3. */
    tSeq->orig_direction= 0;
    tSeq->orig_strand   = 0;
    tSeq->offset        = 0;

    if(tSeq->comments != NULL)
        tSeq->comments[0]   = '\0';
    tSeq->commentslen   = 0;

    if(tSeq->baggage != NULL)
        tSeq->baggage[0]    = '\0';
    tSeq->baglen        = 0;
    tSeq->group_number  = 0;
    tSeq->group_ID  = 0;
}



/*********
 *
 * Free memory for a record.
 *
 **********/

void
FreeRecord(tSeq)
     Sequence **tSeq;
{
    Cfree((*tSeq)->c_elem);
    Cfree((*tSeq)->comments);
    Cfree((*tSeq)->baggage);
    Cfree((*tSeq));
    (*tSeq)->c_elem = NULL;
    (*tSeq)->comments = NULL;
    (*tSeq)->baggage = NULL;
    (*tSeq) = NULL;
}


static max_day[2][13] = {
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} };



/***********
 *
 * strToDate() locates first six integers and translates them 
 * into a date.
 *
 * String should have the format of "mm/dd/yy hh/mn/sc xm", 
 * with anything except digit as the delimiters.
 *
 * Order in the date array is (0->5): (yy mm dd hh mn sc).
 *
 * Returns FALSE if anything is wrong, TRUE otherwise.
 *
 **********/

int
strToDate(str, date)
     const char *str;
     int date[];
{
    int leap;
    char temp_str[2];
    char longstr[256];

    /* locate 6 integers. */
    
    strcpy(longstr, str);
    strcat(longstr, "   -1/-1/-1  ");
    sscanf(longstr, "%d%*c%d%*c%d%*c%d%*c%d%*c%d%2s",
           &date[1],&date[2],&date[0],&date[3],
           &date[4],&date[5],temp_str);

    /* verify year. */
    if(date[0] >= 100)
        date[0] -= 1900;

    /* verify month. */
    if(date[1] > 12 || date[1] < 1)
    {
        fprintf(stderr,"invalid month %s\n", str);
        return FALSE;
    }
    
    /* verify day. */
    if ((date[0] % 4 == 0 && date[0] % 100 != 0) ||
        date[0] % 400 == 0)
        leap = 1;
    else
        leap = 0;
    
    if(date[2] > max_day[leap][date[1]] ||
       date[2] < 1)
    {
        fprintf(stderr,"invalid day %s\n", str);
        return FALSE;
    }

    /* verify time. */
    if (strncmp(temp_str,"pm",2)==0)
        date[3] += 12;
    if (date[3]<-1 || date[3]>23 || 
        date[4]<-1 || date[4]>59 || 
        date[5]<-1 || date[5]>59 )
    {
        fprintf(stderr,"invalid time %s\n", str);
        return FALSE;
    }

    return TRUE;
}


/**********
 * 
 *  Default_IUPAC_Trans() translates an ASCII IUPAC code into
 *  an (char) integer.
 *
 **********/

char 
Default_IUPAC_Trans(base)
     char base;
{
    int i;
    char c;
    c = base | 32;

    if(c == 'u')
        return (char ) 8;

    if(c == 'p')
        return (char) 5;

    for(i=0; i<16; i++)
    {
        if(c == Default_DNA_Trans[i])
        {
            return ( (char) i);
        }
    }
    fprintf(stderr, "Character %c is not IUPAC coded.\n", base);
    return -1;
}


/***********
 * 
 * MakeConsensus() takes an array of aligned sequence and an
 * initialized 'Sequence' consensus.  It modifies the consensus.
 *
 * The memory that 'consensus' has located will be reused, and 
 * consensus->seqmaxlen will be modified if necessary.
 *
 * Returns TRUE if successful, FALSE otherwise.
 *
 **********/

char *uniqueID();

int
MakeConsensus(aligned, numOfAligned, consensus, group)
     Sequence aligned[];    /* input.  */
     int numOfAligned;      /* input.  */
     Sequence *consensus;   /* input and output. */
     int group;             /* Group number (if zero, use all groups) */
{
    char occurence;
    int i, j, index;
    int max_cons = INT_MIN;
    int min_offset = INT_MAX;
    char temp_str[2];
    unsigned char case_bit;

    /*
     *  Search for the minimun offset.
     */

    for (i=0; i<numOfAligned; i++)
    {
        if(group == 0 || aligned[i].group_number == group)
        {
            SeqNormal(&aligned[i]);
            min_offset = MIN(min_offset, aligned[i].offset);
            max_cons = MAX(max_cons, aligned[i].offset+aligned[i].seqlen);
        }
    }

    /*
     *  Decide consensus base by base.
     */

    CleanRecord(consensus);
    consensus->offset = min_offset;

    if(aligned[0].contig[0] != '\0')
    {
        strcpy(consensus->name, aligned[0].contig);
        strcat(consensus->name, ".");
    }
    else if(strncmp(aligned[0].name, "cons.", 5) != 0)
    {
        strcpy(consensus->name, "cons.");
        strcat(consensus->name, aligned[0].name);
    }
    strcpy(consensus->sequence_ID, uniqueID());
    strcpy(consensus->contig, aligned[0].contig);

    for(j=min_offset; j<max_cons; j++)
    {
        occurence =  00;
    	case_bit = 0;
        for(i=0; i<numOfAligned; i++)
        {
            if(group == 0 || aligned[i].group_number == group)
            {
                if (j >= aligned[i].offset &&
                    j < aligned[i].offset+aligned[i].seqlen)
                {
                    index = j-aligned[i].offset;
		    
                    if(aligned[i].c_elem[index] == '-')
                        case_bit = 32;
                    else if(case_bit == 0)
                        case_bit |= (aligned[i].c_elem[index] & 32);

                    occurence = occurence | 
                        Default_IUPAC_Trans(aligned[i].c_elem[index]);

                    if(occurence != 1 && occurence != 2 && 
                       occurence != 4 && occurence != 8)
                        case_bit = 32;
                    /*
                      printf("%1c", aligned[i].c_elem[index]);
                    */
                }
                /*
                  else
                  printf(" "); 
                */
            }
        }

        sprintf(temp_str, "%1c", Default_DNA_Trans[(int) occurence]);
        if(case_bit == 0)
            temp_str[0] = toupper(temp_str[0]);

        if(InsertElems(consensus, j, temp_str)== FALSE)
            return FALSE;
        /*
          printf("   cons[%d]=%1c\n", j - min_offset,
          consensus->c_elem[j - min_offset]);
        */
    }
    return TRUE;
}



/***********
 * 
 * MakeScore() takes an array of aligned sequence, and generates
 * a consensus.  Note, memory for (Sequence* consensus) should be 
 * located before it is passed to this function.
 * 
 * Returns TRUE if successful, FALSE otherwise.
 *
 **********/

int
MakeScore(aligned, numOfAligned, consensus, group)
     Sequence aligned[];    /* input.  */
     int numOfAligned;      /* input.  */
     Sequence *consensus;   /* input and output. */
     int group;
{
    int i, j, index, score;
    int max_cons = INT_MIN;
    int min_offset = INT_MAX;
    int As, Cs, Ts, Gs, Ns, tot_in_grp;
    char temp_str[2], occurence, base;
    int max_occ;

    static char map[17] = "0123456789ABCDEF"; 

    /*
     *  Search for the minimum offset.
     */

    for (i=0; i<numOfAligned; i++)
    {
        if(group == 0 || aligned[i].group_number == group)
        {
            SeqNormal(&aligned[i]);
            min_offset = MIN(min_offset, aligned[i].offset);
            max_cons = MAX(max_cons, aligned[i].offset+aligned[i].seqlen);
        }
    }

    /*
     *  Decide consensus base by base.
     */
    CleanRecord(consensus);
    consensus->offset = min_offset;

    if(aligned[0].contig[0] != '\0')
    {
        strcpy(consensus->name, aligned[0].contig);
        strcat(consensus->name, ".");
    }
    else if(strncmp(aligned[0].name, "cons.", 5) != 0)
    {
        strcpy(consensus->name, "cons.");
        strcat(consensus->name, aligned[0].name);
    }
    strcpy(consensus->sequence_ID, uniqueID());
    strcpy(consensus->contig, aligned[0].contig);

    for(j=min_offset; j<max_cons; j++)
    {
        As = Cs = Ts = Gs = Ns = 0;
        tot_in_grp = 0;
        occurence = 00;

        for(i=0; i<numOfAligned; i++)
        {
            if(group == 0 || aligned[i].group_number == group)
            {
                if (j >= aligned[i].offset &&
                    j < aligned[i].offset+aligned[i].seqlen)
                {
                    tot_in_grp++;
                    index = j-aligned[i].offset;
		    
                    /*
                      occurence = Default_IUPAC_Trans(aligned[i].c_elem[index]);
                      if((occurence & 01) == 01)
                      As++;
                      if((occurence & 02) == 02)
                      Cs++;
                      if((occurence & 04) == 04)
                      Gs++;
                      if((occurence & 010) == 010)
                      Ts++;
                    */
		    
                    base = (aligned[i].c_elem[index]|32);

                    if(base == 'a')
                        As++;
                    else if(base == 'c')
                        Cs++;
                    else if(base == 'g')
                        Gs++;
                    else if(base == 't')
                        Ts++;
                    else if(base == 'n' || base == '-')
                        Ns++;
                    /*
                      printf("%1c", aligned[i].c_elem[index]);
                    */
                }
                /*
                  else
                  printf(" ");
                */
            }
        }

        max_occ = MAX(As, MAX(Cs, MAX(Gs,Ts)));

        /* socre = [0,E], F:all mismatches are either 'n' or '-' */
        if(Ns != 0 && max_occ+Ns == tot_in_grp)
            score = 15;
        else
            score = max_occ*14/tot_in_grp;

        /*
          if( score > 0xF )
          {
          if (InsertElems(consensus, j, "F") == FALSE)
          {
          return FALSE;
          }
          }
          else
          {
        */

        sprintf(temp_str,"%1c", map[score]);
        if(InsertElems(consensus, j, temp_str) == FALSE)
        {
            return FALSE;
        }
	
        /*
          printf("   %2d-%2d-%2d-%2d  %2d   cons[%d]=%1c\n",
          Ts, Gs, Cs, As, score, j, 
          consensus->c_elem[j]);
        */
    }
    return TRUE;
}


/***********
 * 
 * MakePhyloMask() takes an array of aligned sequence, and generates
 * a mask that has a '0' for all columns except the columns which contain
 * a, c, g, t and u only.
 * 
 * Returns TRUE if successful, FALSE otherwise.
 *
 **********/

int
MakePhyloMask(aligned, numOfAligned, consensus, group, acgtu)
     Sequence aligned[];    /* input.  */
     int numOfAligned;      /* input.  */
     Sequence *consensus;   /* input and output. */
     int acgtu[];
     int group;
{
    int i, j, cnt, max_cons = INT_MIN, min_offset = INT_MAX;

    /*
     *  Search for the minimum offset.
     */

    for (i=0; i<numOfAligned; i++)
    {
        if(group == 0 || aligned[i].group_number == group)
        {
            SeqNormal(&aligned[i]);
            min_offset = MIN(min_offset, aligned[i].offset);
            max_cons = MAX(max_cons, aligned[i].offset+aligned[i].seqlen);
        }
    }

    /*
     *  Decide consensus base by base.
     */
    CleanRecord(consensus);
    consensus->offset = min_offset;
    strcpy(consensus->name, "mask");
    strcpy(consensus->type, "MASK");
    strcpy(consensus->sequence_ID, uniqueID());
    strcpy(consensus->contig, aligned[0].contig);
    
    consensus->seqlen = max_cons - min_offset;
    if(consensus->seqmaxlen == 0)
    {
        consensus->c_elem = (char *)Calloc(max_cons - min_offset+5, 1);
        consensus->seqmaxlen = max_cons - min_offset + 5;
    }
    else if(consensus->seqmaxlen < max_cons - min_offset)
    {
        consensus->seqmaxlen = max_cons - min_offset + 5;
        consensus->c_elem = (char *)Realloc(consensus->c_elem,
                                            max_cons - min_offset + 5);
    }

    cnt = 0;
    for(j=min_offset; j<max_cons; j++)
    {
        consensus->c_elem[j-min_offset] = '1';
        for(i=0; i<numOfAligned; i++)
        {
            if(group == 0 || aligned[i].group_number == group)
            {
                if (j < aligned[i].offset ||
                    j >= aligned[i].offset+aligned[i].seqlen ||
                    acgtu[aligned[i].c_elem[j-aligned[i].offset]] == 0)
                {
                    consensus->c_elem[j-min_offset] = '0';
                    cnt++;
                    break;
                }
            }
        }
    }
    fprintf(stderr, "\nNumber of 1s in mask: %d\n", max_cons-min_offset-cnt);
    fprintf(stderr,   "Number of 0s in mask: %d\n\n", cnt);
    return TRUE;
}


/***********
 * 
 * MajorityCons() takes an array of aligned sequence, and generates
 * a MAJORITY consensus.  
 * Note, memory for (Sequence* consensus) should be 
 * located before it is passed to this function.
 * 
 * Returns TRUE if successful, FALSE otherwise.
 *
 **********/

int
MajorityCons(aligned, numOfAligned, consensus, group, major_perc)
     Sequence aligned[];    /* input.  */
     int numOfAligned;      /* input.  */
     Sequence *consensus;   /* input and output. */
     int group, major_perc;
{
    int i, j, index, score, ii, base, max;
    int max_cons = INT_MIN;
    int min_offset = INT_MAX;
    char temp_str[2], occurence;
    int *cnts, tot_in_grp;
    unsigned char case_bit;

    cnts = (int *)Calloc(16, sizeof(int));

    /*
     *  Search for the minimum offset.
     */

    for (i=0; i<numOfAligned; i++)
    {
        if(group == 0 || aligned[i].group_number == group)
        {
            SeqNormal(&aligned[i]);
            min_offset = MIN(min_offset, aligned[i].offset);
            max_cons = MAX(max_cons, aligned[i].offset+aligned[i].seqlen);
        }
    }

    /*
     *  Decide consensus base by base.
     */

    CleanRecord(consensus);
    consensus->offset = min_offset;

    if(aligned[0].contig[0] != '\0')
    {
        strcpy(consensus->name, aligned[0].contig);
        strcat(consensus->name, ".");
    }
    else if(strncmp(aligned[0].name, "cons.", 5) != 0)
    {
        strcpy(consensus->name, "cons.");
        strcat(consensus->name, aligned[0].name);
    }
    strcpy(consensus->sequence_ID, uniqueID());
    strcpy(consensus->contig, aligned[0].contig);

    for(j=min_offset; j<max_cons; j++)
    {
        case_bit = 0;
        occurence = 00;
        tot_in_grp = 0;
        for(ii = 0; ii < 16; ii++)
            cnts[ii] = 0;

        for(i=0; i<numOfAligned; i++)
        {
            if(group == 0 || aligned[i].group_number == group)
            {
                if (j >= aligned[i].offset &&
                    j < aligned[i].offset+aligned[i].seqlen)
                {
                    tot_in_grp++;
                    index = j-aligned[i].offset;

                    if(aligned[i].c_elem[index] == '-')
                        case_bit = 32;
                    else if(case_bit == 0)
                        case_bit |= (aligned[i].c_elem[index] & 32);
		    
                    occurence |= 
                        Default_IUPAC_Trans(aligned[i].c_elem[index]);
                    cnts[(int)Default_IUPAC_Trans(aligned[i].c_elem[index])]++;

                    if(case_bit == 0 &&
                       occurence != 1 && occurence != 2 && 
                       occurence != 4 && occurence != 8)
                        case_bit = 32;
                }
            }
        }

        max = 0;
        for(ii = 0; ii < 16; ii++)
        {
            if(cnts[ii] > max)
            {
                max = cnts[ii];
                base = ii;
            }
        }
        if(max*100/tot_in_grp >= major_perc)
        {
            /* follow the majority rule. */ 
            sprintf(temp_str,"%1c", Default_DNA_Trans[base]);
        }
        else
        {
            /* use IUPAC code. */
            sprintf(temp_str,"%1c", 
                    Default_DNA_Trans[(int) occurence]);
        }

        if(case_bit == 0)
            temp_str[0] = toupper(temp_str[0]);
	
        if(InsertElems(consensus, j, temp_str) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}


/***********
 *
 * ReadGDEtoHGL() reads a GDE formated file into an array of HGL structure.
 *
 * Return -1 if anything is wrong, number_of_sequence otherwise.
 *
 ***********/

int
ReadGDEtoHGL(fp, tSeq_arr)
     FILE *fp;
     Sequence **tSeq_arr;
{
    char line[MAXLINELEN];
    int ptr, num_seq, max_num_seq = 20;
    int seq_len = 200;
    char *newline;

    (*tSeq_arr) = (Sequence *)Calloc(max_num_seq, sizeof(Sequence));
    num_seq = -1;
    while(fgets(line, MAXLINELEN-2, fp) != NULL) /* spaces for \n\0 */
    {
        /* ptr points to the last char. */
        ptr = strlen(line)-1;

        /* clear up the tail. */
        while(ptr>=0 && (line[ptr] == '\n' || 
                         line[ptr] == ' '  || 
                         line[ptr] == '\t'))
            ptr--;
        line[ptr+1] = '\0';

        if(ptr <= 0)
        {   
            /* it is an empty line. */
        }
        else if(line[0] == '#')
        {
            if(++num_seq == max_num_seq)
            {
                max_num_seq *= 2;
                /* printf("max_num_seq = %d\n", max_num_seq); */
                (*tSeq_arr) = (Sequence *)Realloc((*tSeq_arr), 
                                                  max_num_seq*sizeof(Sequence));
            }
	    
            InitRecord((*tSeq_arr)[num_seq]);

            if (line[ptr] == '<')
            {
                (*tSeq_arr)[num_seq].direction = 2; /* 3to5 */
                line[ptr] = '\0';
            }
            else if (line[ptr] == '>')
            {
                (*tSeq_arr)[num_seq].direction = 1; /* 5to3 */
                line[ptr] = '\0';
            }
            strcpy((*tSeq_arr)[num_seq].sequence_ID, line+1);
        }
        else
        {
            ptr = 0;
            if((*tSeq_arr)[num_seq].seqlen == 0)
            {
                /* determine the offset. */
                while(line[ptr] != '\0' && line[ptr] == '-')
                {
                    ptr++;
                }
                (*tSeq_arr)[num_seq].offset += ptr;
            }

            if(line[ptr] != '\0')
            {
                newline = line + ptr; 

                if((*tSeq_arr)[num_seq].seqmaxlen == 0)
                {
                    (*tSeq_arr)[num_seq].c_elem =
                        (char *)Calloc(seq_len, 1);
                    (*tSeq_arr)[num_seq].c_elem[0] = '\0';
                    (*tSeq_arr)[num_seq].seqmaxlen = seq_len;
                }
                else
                {
                    while((*tSeq_arr)[num_seq].seqlen + strlen(newline) + 1
                          > (*tSeq_arr)[num_seq].seqmaxlen)
                    {
                        seq_len *= 2;
                        (*tSeq_arr)[num_seq].c_elem = (char *)
                            Realloc((*tSeq_arr)[num_seq].c_elem, seq_len);
                        (*tSeq_arr)[num_seq].seqmaxlen = seq_len;
                    }
                }
                strcat((*tSeq_arr)[num_seq].c_elem, newline);
                (*tSeq_arr)[num_seq].seqlen = strlen((*tSeq_arr)[num_seq].c_elem);
            }
        }
    }

    return (num_seq + 1);
}




/********
 *
 * InsertElems returns TRUE if successful, FALSE otherwise.
 *
 ********/

int
InsertElems(seq,pos,c)
     Sequence *seq;  /* Sequence */
     int pos;        /* Position (in respect to the master consensus)
                      * to insert BEFORE
                      * always move string to the right. */
     char c[];       /*Null terminated array of elements to insert */
{
    int dashes, j,len;

    len = strlen(c);

    if(seq->seqlen == 0)
    {
        /* get rid of '-'s at right. */
        /*
          dashes = len-1;
          while(dashes >= 0 && c[dashes] == '-')
          dashes--;
          if(dashes < 0)
          {
          seq->offset = pos;
          return TRUE;
          }
          c[dashes+1] = '\0';
        */

        /* clear out '-'s at left. */
        dashes = 0;
        /*
          while(c[dashes] == '-') 
          dashes++;

          c += dashes;
          len = strlen(c);
          pos += dashes;
        */

        if(seq->seqmaxlen == 0)
        {
            seq->c_elem = (char *)Calloc(len+1, 1);
            seq->seqmaxlen = len + 1;
        }
        else if(len+1 >= seq->seqmaxlen)
        {
            seq->c_elem = (char *)Realloc(seq->c_elem, len+1);
            seq->seqmaxlen = len+1;
        }

        strcpy(seq->c_elem, c);
        seq->seqlen = len;
        seq->offset = pos;
        return TRUE;
    }
    
    /* to make sure there is a space for '\0'. */
    if(seq->seqlen > seq->seqmaxlen)
    {
        fprintf(stderr,
                "InsertElems(): seqlen>seqmaxlen. Something is wrong.\n");
        return FALSE;
    }
    else 
    {
        while(seq->seqlen+1 >= seq->seqmaxlen)
        {
            seq->seqmaxlen *= 2;
            seq->c_elem = (char *)Realloc(seq->c_elem, seq->seqmaxlen);
        }
    }
    seq->c_elem[seq->seqlen] = '\0';

    if(pos < seq->offset) /* insert to the left of the seq. */
    {
        /* ignore the dashes at the left. */
        dashes = 0;
        /*
          while(dashes < len && c[dashes] == '-')
          dashes++;
          if(c[dashes] == '\0')
          {
          seq->offset += len;
          return TRUE;
          }
          c += dashes;
          len -= dashes;
        */

        if(seq->seqlen + len + seq->offset - pos > seq->seqmaxlen)
        {
            seq->seqmaxlen = seq->seqlen+len+seq->offset-pos+256;
            seq->c_elem = (char *)Realloc(seq->c_elem, seq->seqmaxlen);
        }

        /* copy the old string including the last '\0'. */
        for(j=seq->seqlen; j>=0; j--)
            seq->c_elem[j+len+seq->offset-pos] = seq->c_elem[j];

        /* insert dashes. */
        for(j=len; j<len+seq->offset-pos; j++)
            seq->c_elem[j] = '-';

        /* copy the inserted string. */
        for(j=0; j<len; j++)
            seq->c_elem[j] = c[j];

        /* detector. */
        if(c[j] != '\0')
            fprintf(stderr, "InsertElems:  Problem.....\n");

        seq->seqlen = strlen(seq->c_elem);

        /*  seq->offset = pos;  commented on 6-3-91 */
        seq->offset = pos + dashes;
        if(dashes > 0)
            printf("\nInsertElems(): dashes is not zero.\n\n");
    }

    else if(pos - seq->offset >= seq->seqlen) /* insert to the right. */
    {
        /* ignore the dashes at the right. */
        /*
          dashes = len -1;
          while(dashes >= 0 && c[dashes] == '-')
          dashes--;
          if(dashes < 0)
          return TRUE;
          len = dashes+1;
          c[len] = '\0';
        */

        if(pos - seq->offset + len > seq->seqmaxlen)
        {
            seq->seqmaxlen = pos - seq->offset + len + 256;
            seq->c_elem = (char *)Realloc(seq->c_elem, seq->seqmaxlen);
        }

        /* insert dashes. */
        for(j=seq->seqlen; j<pos-seq->offset; j++)
            seq->c_elem[j] = '-';

        /* copy the inserted string. */
        for(j=0; j<len; j++)
            seq->c_elem[pos - seq->offset + j] = c[j];
        seq->c_elem[pos-seq->offset+len] = '\0';

        /* detector. */
        if(c[j] != '\0')
            fprintf(stderr, "InsertElems:  Problem too .....\n");

        seq->seqlen = strlen(seq->c_elem);
    }
    else /* insert into the seq. */
    {
        if(seq->seqlen + len > seq->seqmaxlen)
        {
            seq->seqmaxlen = seq->seqlen + len + 256;
            seq->c_elem = (char *)Realloc(seq->c_elem, seq->seqmaxlen);
        }

        /* move the bottom part of the older string including the last '\0'. */
        for(j=seq->seqlen; j>=pos-seq->offset; j--)
            seq->c_elem[j+len] = seq->c_elem[j];

        /* copy the inserted string. */
        for(j=0; j<len; j++)
            seq->c_elem[pos - seq->offset + j] = c[j];

        /* detector. */
        if(c[j] != '\0')
            fprintf(stderr, "InsertElems:  Problem too too .....\n");

        seq->seqlen = strlen(seq->c_elem);
    }

    return TRUE;
}




/******************************************************************
 *
 *   int GetArgs(argArray, numArgs)
 *       Arg  *argArray;
 *       int   numArgs;
 *
 *   Return TRUE if successful, FALSE otherwise.
 *
 ******************************************************************/

#define MAX_ARGS 50		/* maximum args this can process */

int
GetArgs(argArray, numArgs, argc, argv)
     Args *argArray;
     int  numArgs;
     int argc;
     char **argv;
{
    int i, j;
    Args *curarg;
    int noArgOK = TRUE;

    if ((argArray == NULL) || (numArgs == 0) || (numArgs > MAX_ARGS))
    {
        fprintf(stderr, "GetArgs:  Invalid number of args.\n");
        return FALSE;
    }

    /* 
     * Test if all are either 'default' or 'optional'. 
     */
    curarg = argArray;
    for (i=0; i<numArgs; i++, curarg++)
    {
        if(curarg->strvalue[0] == '\0' && curarg->optional == 'F')
        {
            noArgOK = FALSE;
            break;
        }
    }
    
    /*
     * show usage if some arg is required but no arg is 
     * supllied on command line. 
     */
    if(noArgOK == FALSE && argc == 1)
    {
        fprintf(stderr, "\n%s   arguments:\n\n", argv[0]);
        curarg = argArray;

        for (i = 0; i < numArgs; i++, curarg++) 
        {
            fprintf(stderr, "  -%c  %s ", curarg->tag, curarg->prompt);
            if (curarg->optional == 'T')
                fprintf(stderr, "  [Optional]");
            fprintf(stderr, "\n");
            if (curarg->strvalue[0] != '\0')
                fprintf(stderr, "    default = %s\n", curarg->strvalue);
        }
        fprintf(stderr, "\n");
        return FALSE;
    }

    /*  
     *  Process
     */
    for (i = 1; i < argc; i++) 
    {
        if (argv[i][0] != '-') 
        {
            fprintf(stderr, "Arguments must start with -");
            return FALSE;
        }

        /* check the tag. */
        curarg = argArray;
        for (j = 0; j < numArgs; j++, curarg++) 
        {
            if ((argv[i][1]|32) == (curarg->tag|32))
                break;
        }
        if (j == numArgs) 
        {
            fprintf(stderr, "Invalid argument tag in %s\n", argv[i]);
            return FALSE;
        }
	
        strcpy(curarg->strvalue, argv[i]+2);
        if(curarg->strvalue[0] == '\''
           && curarg->strvalue[strlen(curarg->strvalue)-1] == '\'')
        {
            char ttmm[256];
            strcpy(ttmm, curarg->strvalue+1);
            ttmm[strlen(ttmm)-1] = '\0';
            strcpy(curarg->strvalue, ttmm);
        }
    }
    return TRUE;
}


/*********
 *
 *  GetCond interprets the -c argument, the condition.
 *
 *  The condition will be set to NULL if no condition is specified,
 *  that is, if you pass '&p' as the address of a cond* structure,
 *  p will be set to NULL if no condition [(p == NULL) = TRUE].
 *
 *  Return TRUE if successful, FALSE otherwise.
 *
 *********/

int
GetCond(arg, cond)
     char *arg;
     str_cond **cond;
{
    int start, end, i, found;
    char message_buf[1000];

    if ( strcmp(arg, "null")==0)
    {
        (*cond) = NULL;
        return TRUE;
    }
    else
    {
        (*cond) = (str_cond *)Calloc(1, sizeof(str_cond));

        start = end = 0;
	
        /* find the field name. */
        while (('a'<= arg[end] && arg[end]<='z') ||
               ('A'<= arg[end] && arg[end]<='Z') ||
               arg[end] == '-' )
            end++;
	
        found = FALSE;
        for (i=0; i<NUM_OF_FIELDS && found == FALSE; i++)
        {
            if (strncmp(arg, at[i], strlen(at[i]))==0 )
            {
                (*cond)->field = i;   /* condition on field &at[i]. */
                found = TRUE;
                break;
            }
        }
        if (found == FALSE)
        {
            strncpy(message_buf, arg, end-start);
            message_buf[end-start] = '\0';
            fprintf(stderr, "Field %s not found.\n", message_buf);
            return FALSE;
        }
	
        start = end;
        end++;
        while (arg[end] == '=' ||
               arg[end] == '!' ||
               arg[end] == '>' ||
               arg[end] == '<' )
            end++;
        strncpy((*cond)->symbol, arg+start, end-start);
        (*cond)->symbol[end-start] = '\0';
        if (strlen((*cond)->symbol)>2 ||
            strlen((*cond)->symbol)<1 ||
            (strlen((*cond)->symbol)==1 && 
             *((*cond)->symbol) !='>' && 
             *((*cond)->symbol) != '<') ||
            (strlen((*cond)->symbol)==2 &&
             (strncmp((*cond)->symbol,"!=",2)!= 0 ) &&
             (strncmp((*cond)->symbol,"==",2)!= 0 ) &&
             (strncmp((*cond)->symbol,">=",2)!= 0 ) &&
             (strncmp((*cond)->symbol,"<=",2)!= 0 )
             )
            )
        {
            fprintf(stderr, "Invalid condition.\n");
            return FALSE;
        }

        if(arg[end] == '"' && arg[strlen(arg) - 1] == '"')
        {
            end++;
            arg[strlen(arg) - 1] = '\0';
        }

        (*cond)->value = (char *)Calloc(strlen(arg) - end + 2, 1);
        strcpy((*cond)->value, arg+end);
    }
    return TRUE;
}


/*********
 *
 *  GetFields interprets the -f arguments, the fields list.
 *
 *  Returns number of selected fields, 0 if anything is wrong. 
 *
 *********/

int 
GetFields(arg, selected_fields)
     char *arg;
     int selected_fields[];
{
    int start, end, i, found, list_done, i_selected;
    char message_buf[1000];

    if ( strcmp(arg, "all") == 0 )
    {
        selected_fields[0] = -1;
        return NUM_OF_FIELDS;
    }
    else
    {
        start = end = 0;
        list_done = FALSE;
        i_selected = 0;

        while ( list_done == FALSE )
        {
            while (arg[end] != '\0' && arg[end] != ',')
            {
                end++ ;
            }
            if (arg[end] == '\0')
            {
                list_done = TRUE;
            }
            found = FALSE;
            for (i=0; i<NUM_OF_FIELDS && found == FALSE; i++)
            {
                if (strncmp(arg+start, at[i], strlen(at[i])) == 0)
                {
                    selected_fields[i_selected++] = i;
                    found = TRUE;
                    start = end+1;
                    break;
                }
            }
            if (found == FALSE)
            {
                strncpy(message_buf, (arg+start),end-start);
                message_buf[end-start] = '\0';
                fprintf(stderr, "Field %s not found.\n", message_buf);
                return 0;
            }
            end++;
        }
    }
    
    return i_selected;
}




static char *pairs[] = {"aa","ac","ag","at",
                        "ca","cc","cg","ct",
                        "ga","gc","gg","gt",
                        "ta","tc","tg","tt" };

static int stemp[16] = {55, 98,  58, 57,
                        55, 86,  73, 58,
                        87, 136, 86, 98,
                        37, 87,  55, 55 };

/*******
 *
 *  MST() returns Mean Stacking Temperature for the given sequence,
 *  returns -1 if anything is wrong.
 *
 *******/

float
MST(c_elem)
     const char *c_elem;
{
    int i, j, l;
    int tot_stemp = 0, non_amb_pairs = 0;
    char *seq;

    l = strlen(c_elem);
      
    seq = (char *)Calloc(l, 1+1);

    /* clean out dashes. */
    j = 0;
    for(i = 0; i<l; i++)
    {
        if(c_elem[i] != '-')
        {
            seq[j] = c_elem[i]|32;
            if(seq[j] == 'u')
                seq[j] = 't';
            j++;
        }
    }
    seq[j] = '\0';
    l = j;

    for(i=0; i<l-1; i++)
    {
        j = 0;
        while(j<16 && strncmp(seq+i, pairs[j], 2) != 0)
        {
            j++;
        }

        /* ignore the pairing of an ambiguous base. */
        if(j!=16)
        {
            tot_stemp += stemp[j];
            non_amb_pairs++;
        }
    }

    if(seq != NULL)
    {
        Cfree(seq);
        seq = NULL;
    }
    return ((float)tot_stemp/(float)non_amb_pairs);
}


/********
 *
 *  SubStr() fill ss with a substring of at most 'length' chars and returns
 *  TRUE.  If anything is wrong, it sets ss to be empty and returns FALSE.
 *
 ********/

int
SubStr(string, start, length, ss)
     const char *string;
     int start, length;
     char *ss;
{
    int i;

    if(strlen(string)<=start)
    {
        fprintf(stderr, "SubStr(): starting point is beyond the boundary.\n");
        ss[0] = '\0';
        return FALSE;
    }

    for(i=start; string[i] != '\0' && i<start+length; i++)
    {
        ss[i-start] = string[i];
    }
    ss[i-start] = '\0';

    return TRUE;
}



/*******
 *
 *  FindPattern() searches string for pattern.
 *  Returns the number of appearences.
 *
 *******/

int
FindPattern(string, pattern)
     const char *string;
     const char *pattern;
{
    int i, sl, pl, num_app = 0;

    if(string == NULL || (sl = strlen(string)) == 0)
        return 0;

    pl = strlen(pattern);

    for(i = 0; i <= sl-pl; i++)
    {
        if(strncmp(string+i, pattern, pl) == 0)
            num_app++;
    }

    return num_app;
}


/*******
 *
 *  FindPattern2(), same as FindPattern(), but returns the #
 *  of appearences that do not overlap only.
 *
 *******/

int
FindPattern2(string, pattern, orig_loc)
     const char *string;
     const char *pattern;
     int orig_loc;
{
    int i, sl, pl, num_app = 0;

    if(string == NULL || (sl = strlen(string)) == 0)
        return 0;

    pl = strlen(pattern);

    for(i = 0; i <= sl-pl; i++)
    {
        if(abs(i - orig_loc) >= pl && 
           strncmp(string+i, pattern, pl) == 0)
            num_app++;
    }

    return num_app;
}


/*******
 *
 *  FindPatternNC() searches string for pattern , CASE INSENSITIVE.
 *  Returns the number of appearences. 
 *
 *******/

int
FindPatternNC(string, pattern)
     const char *string;
     const char *pattern;
{
    int i, j, sl, pl, num_app = 0;

    if(string == NULL || (sl = strlen(string)) == 0)
        return 0;

    pl = strlen(pattern);

    for(i = 0; i <= sl-pl; i++)
    {
        j = 0; 
        while(j < pl && (string[i+j]|32) == (pattern[j]|32))
            j++;

        if(j == pl)
            num_app++;
    }

    return num_app;
}


/*******
 *
 *  Complementary() CHANGES the given DNA/RNA string to its complementary,
 *  and returns TRUE.  Returns FALSE if anything is wrong and keep the
 *  given string unchanged.
 *
 *******/

int
Complementary(sequence, type)
     char *sequence;
     char type;
{
    int i, l;
    char *temp_str;

    l = strlen(sequence);
    temp_str = (char *)Calloc(l+1, sizeof(char));
    if( type == 'D' ||  type == 'd')
        type = 0;
    else if(type == 'R' ||  type == 'r')
        type = 1;
    else
    {
        fprintf(stderr,
                "Complementary():  type unknown.  Type is D/d/R/r\n");
        return NULL;
    }

    for(i=0; i<l; i++)
    {
        switch(sequence[i])
        {
            case 'A':
                temp_str[i] = (type == 0) ? 'T' : 'U';
                break;
            case 'a':
                temp_str[i] = (type == 0) ? 't' : 'u';
                break;
            case 'C':
                temp_str[i] = 'G';
                break;
            case 'c':
                temp_str[i] = 'g';
                break;
            case 'G':
                temp_str[i] = 'C';
                break;
            case 'g':
                temp_str[i] = 'c';
                break;
            case 'T':
            case 'U':
                temp_str[i] = 'A';
                break;
            case 't':
            case 'u':
                temp_str[i] = 'a';
                break;
        }
    }
    temp_str[i] = '\0';
    strcpy(sequence, temp_str);
    if(temp_str != NULL)
    {
        Cfree(temp_str);
        temp_str = NULL;
    }

    return TRUE;
}


/********
 *
 *  KnownSeq() returns an integer which is the index of the first
 *  occurence of an ambiguous base in the seq.  -1 if no ambiguous
 *  base in the seq.
 *
 ********/

int KnownSeq(seq)
     char *seq;
{
    int i;
    char c;

    for(i=0; i<strlen(seq); i++)
    {
        c = seq[i]|32;
        if(c != 'a' && c != 't' && c != 'g' && c != 'c' && c != 'u')
            return i;
    }
    return -1;
}


/********
 *
 *  Reverse() reverses the given string and returns TRUE.
 *  (NOTE: Reverse() actually changes the string).  
 *  If anything goes wrong, leave seq unchanged.
 *  
 *
 ********/

int Reverse(seq)
     char *seq;
{
    int i, l;
    char c;

    l = strlen(seq);

    if(l<2)
    {
        return TRUE;
    }

    for(i=0; i < l/2; i++)
    {
        c = seq[i];
        seq[i] = seq[l-i-1];
        seq[l-i-1] = c;
    }
    return TRUE;
}


/********
 *
 *  GoodOligos() returns a pointer to an array of subsequences that
 *  do not contant secondary structure, nor self complementary structure.
 *  Returns NULL if anything is wrong.
 *
 *  l_bnd and r_bnd are regards to the head of the probe.
 *
 *  Note: this program Calloc-s memory for the returned pointer.
 *  The caller program is responsible of Freeing the memory when
 *  not needed.
 *
 ********/

char **
GoodOligos(c_elem, check_len, min_len, max_len, l_bnd, r_bnd)
     char *c_elem;
     int check_len, min_len, max_len, l_bnd, r_bnd;
     /* l_bnd and r_bnd are relative to c_elem, so they should be in 
   [0,strlen(c_elem)] */
{
    int i, l, seq_len, max_num_probe, seq_cnt = 0;
    char **seq_set;
    char *seq, *subseq, *scd_str, *PossibleOligo;
    int BadOligo, PO_len, PO_index, PO_l;

    /* constant(s): */
    /* to check if there is a substr of length 'no_repeat_len' appears
     * more than once in the PossibleOligo. */
    int no_repeat_len = 15;

    seq_len = strlen(c_elem);

    /* A lower case copy of the c_elem. */
    seq = (char *)Calloc(seq_len+1, sizeof(char));

    /* String used to check the PossibleOligo. */
    PossibleOligo = (char *)Calloc(max_len+1, sizeof(char));
    subseq = (char *)Calloc(max_len+1, sizeof(char));
    scd_str= (char *)Calloc(max_len+1, sizeof(char));

    /* The output.  A set of possibly good oligos. */
    max_num_probe = 20;
    seq_set = (char **)Calloc(max_num_probe, sizeof(char *));

    for(i=0; i<seq_len; i++)
    {
        seq[i] = c_elem[i]|32;
    }

    i = MAX(l_bnd, 0);
    while(i <= MIN(r_bnd, seq_len - min_len))
    {
        BadOligo = FALSE;
        for(l = min_len;
            BadOligo == FALSE && l <= seq_len - i && l <= max_len;
            l++)
        {
            int uk;

            SubStr(seq, i, l, PossibleOligo);

            /* Any unknow base? 
             */

            if((uk = KnownSeq(PossibleOligo)) != -1)
            {
                fprintf(stderr, "%s has ambiguous base(s)\n", PossibleOligo);
                i += uk+1;
                BadOligo = TRUE;
            }
	    
            PO_len = strlen(PossibleOligo);

            /* check if there is a substr of len(no_repeat_len)
             * repeat itself in the PossibleOligo.
             DOESN'T MATTER!  IT COULD MESS UP AT MOST SEVERAL
             BASES READ INTO THE PROBE.  CUT_SITE IS WHAT REALLY
             MATTERS.

             for(PO_index = 0;
             BadOligo==FALSE && PO_index<=PO_len-no_repeat_len;
             PO_index++)
             {
             SubStr(PossibleOligo,PO_index,no_repeat_len,subseq);
             if(FindPattern(PossibleOligo, subseq) > 1)
             {
             fprintf(stderr, 
             "%s has 15 repatitive base(s) %s\n", 
             PossibleOligo, subseq);
             i++;
             BadOligo = TRUE;
             }
             }
            */

            /* 
             * To ensure that the probe is not going to hybridize
             * with itself:
             */
            for(PO_index = 0;
                BadOligo==FALSE && PO_index<=PO_len-no_repeat_len;
                PO_index++)
            {
                SubStr(PossibleOligo, PO_index, no_repeat_len, subseq);
                strcpy(scd_str, subseq);
                Complementary(scd_str, 'd');
                Reverse(scd_str);

                if(FindPattern(PossibleOligo, scd_str) > 0)
                {
                    fprintf(stderr, 
                            "%s may hybridize with itself: %s vs. %s.\n", 
                            PossibleOligo, subseq, scd_str);
                    i++;
                    BadOligo = TRUE;
                }
            }

            for(PO_index = 0;
                BadOligo == FALSE && PO_index <= PO_len-2*check_len;
                PO_index++)
            {
                SubStr(PossibleOligo, PO_index, check_len, subseq);
                Complementary(subseq, 'd');
                strcpy(scd_str, subseq);
                Reverse(scd_str);
		    
                /*
                  if(FindPattern2(PossibleOligo,subseq,PO_index)>0)
                  {
                  fprintf(stderr, "%s has self-compl %s\n", 
                  PossibleOligo, subseq);
                  i += PO_index+1;
                  BadOligo = TRUE;
                  }
                  else 
                */

                if(FindPattern2(PossibleOligo,scd_str,PO_index)>0)
                {
                    fprintf(stderr, "%s has 2nd struct %s\n", 
                            PossibleOligo, scd_str); 
                    i += PO_index+1;
                    BadOligo = TRUE;
                }
            }
            if(BadOligo == FALSE)
            {
                seq_set[seq_cnt] = (char *)
                    Calloc(strlen(PossibleOligo)+1, sizeof(char));
                strcpy(seq_set[seq_cnt], PossibleOligo);

                if(++seq_cnt == max_num_probe)
                {
                    max_num_probe *= 2;
                    seq_set = (char **)
                        Realloc(seq_set, max_num_probe*sizeof(char *));
                }
                i++;
            }
        } /* end of l. */
    } /* end of i. */

    seq_set[seq_cnt] = NULL;

    if(seq_cnt == 0)
        return NULL;

    return seq_set;
}



/* ALWAYS COPY the result from uniqueID() to a char[32],
 * (strlen(hostname)+1+10).  Memory is lost when the function
 * is finished.
 */

char *uniqueID()
{
    char hname[32], vname[32], tstr[32];
    time_t *tp;
    static cnt = 0;
    int ll;

    tp = (time_t *)Calloc(1, sizeof(time_t));

    if(gethostname(hname, 32) == -1)
    {
        fprintf(stderr, "UniqueID(): Failed to get host name.\n");
        exit(1);
    }

    time(tp);
    sprintf(tstr, ":%d:%ld", cnt, *tp);
    if((ll = strlen(tstr)) > 31)
    {
        strncpy(vname, tstr, 31);
        vname[31] = '\0';
    }
    else
    {
        ll = strlen(hname)-(31-ll);
        if(ll < 0)
            ll = 0;
        sprintf(vname, "%s%s", hname+ll, tstr);
    }
    cnt++;
    Cfree(tp);
    return(vname);
}



/* return the percentage of GCcontents. */

int GCcontent(seq)
     char *seq;
{
    int l, gc=0, j;

    l = strlen(seq);

    for (j=0; j<l; j++)
    {
        if((seq[j]|32) == 'g' || (seq[j]|32) == 'c')
        {
            gc++;
        }
    }
    return ((int) (gc*100/l));
}



/******
 *
 *  HGLtoIQ() outputs a HGL format record to an ASCII file with
 *  the Input-Queue format, the format for the synthesizer.
 *
 ******/

void HGLtoIQ(fname, tSeq)
     const char *fname;
     Sequence *tSeq;
{
    FILE *fp;

    if((fp = fopen(fname, "w")) == NULL)
    {
        fprintf(stderr, "Can't open IQ file: %s\n", fname);
        exit(1);
    }
    fprintf(fp, "%s  %s\n", tSeq->comments, tSeq->c_elem); 
}



Find2(string,key)
     char *key,*string;
     /*
      *       Like find, but returns the index of the leftmost
      *       occurence, and -1 if not found.
      *       Note in this program, T==U, and case insensitive.
      */
{
    int i,j,len1,len2,dif,flag = FALSE;
    char *target;

    if(string == NULL || string[0] == '\0')
        return -1;

    len2 = strlen(string);
    target = (char *) Calloc(len2+1, 1);
    for(i = 0; i<len2; i++)
    {
        target[i] = string[i]|32;
        if(target[i] == 'u')
            target[i] = 't';
    }

    len1 = strlen(key);
    for(i = 0; i<len1; i++)
    {
        key[i] |= 32;
        if(key[i] == 'u')
            key[i] = 't';
    }

    dif = len2 - len1 +1;

    if(len1>0)
        for(j=0;j<dif && flag == FALSE;j++)
        {
            flag = TRUE;
            for(i=0; i < len1 && flag; i++)
                flag = (key[i] == target[i+j]) ? TRUE : FALSE;
        }
    Cfree(target);
    return(flag?j-1:-1);
}




/* return -1 if end-of-file.
   FALSE if anything is wrong.
 */
int
ReadGDE(fp, seq)
     FILE *fp;
     Sequence *seq;
{
    char temp_line[1000], waste[64];
    int ii, l1;

    while(fgets(temp_line, 1000, fp) != NULL )
    {
        if(strncmp(temp_line, "sequence-ID", 11) == 0)
        {
            sscanf(temp_line,"%s%s",waste,seq->sequence_ID);
        }
        else if(temp_line[0] == '#')
        {
            strncpy(seq->name, temp_line+1, 31);
            seq->name[31] = '\0';
            ii = 0;
            while(ii < strlen(seq->name) &&
                  seq->name[ii] != ' ' &&
                  seq->name[ii] != '\n')
                ii++;
            seq->name[ii] = '\0';

            seq->seqmaxlen = 256;
            seq->c_elem=(char *)Calloc(seq->seqmaxlen,1);
            seq->seqlen = 0;
            while(fgets(temp_line, 1000, fp) != NULL)
            {
                l1 = strlen(temp_line);

                if(temp_line[l1 - 1] == '\n')
                {
                    l1--;
                    temp_line[l1] = '\0';
                }

                while(seq->seqmaxlen <
                      seq->seqlen + strlen(temp_line) + 1)
                {
                    seq->seqmaxlen *= 2;
                    seq->c_elem = (char *)
                        Realloc(seq->c_elem, seq->seqmaxlen);
                }

                strcat(seq->c_elem, temp_line);
                seq->seqlen += strlen(temp_line);
            }

            if(seq->seqlen == 0)
            {
                fprintf(stderr, "\n%s\n","Sequence is empty.");
                return FALSE;
            }
        }
    }
    return -1;
}


void heapify(seq_set, seq_size, heap_size, elem, Pkey, Skey, order)
     int seq_size, elem, heap_size, **order;
     char Pkey[], Skey[];
     Sequence *seq_set;
{
    int l, r, temp, largest;

    l = 2*elem+1;
    r = 2*elem+2;

    if(l <= heap_size && 
       CompKey(seq_set[(*order)[l]], seq_set[(*order)[elem]],
               Pkey, Skey) > 0)
        largest = l;
    else
        largest = elem;

    if(r <= heap_size && 
       CompKey(seq_set[(*order)[r]], seq_set[(*order)[largest]],
               Pkey, Skey) > 0)
        largest = r;

    if(largest != elem)
    {
        temp = (*order)[elem];
        (*order)[elem] = (*order)[largest];
        (*order)[largest] = temp;
        heapify(seq_set,seq_size,heap_size,largest,Pkey,Skey,order);
    }
}


heapsort(seq_set, seq_size, Pkey, Skey, order)
     int seq_size, **order;
     char Pkey[], Skey[]; 
     Sequence *seq_set;
{
    int ii, temp, heap_size;

    /* 
     * build_heap(seq_set, seq_size, &heap_size, order);
     */
    heap_size = seq_size-1;

    for(ii = (seq_size-1)/2; ii>=0; ii--) /* (L-1)/2-1?? */
    {
        heapify(seq_set, seq_size, heap_size, ii,Pkey,Skey,order);
    }

    for(ii = seq_size-1; ii>0; ii--)
    {
        temp = (*order)[0];
        (*order)[0] = (*order)[ii];
        (*order)[ii] = temp;
        heap_size--;
        heapify(seq_set, seq_size, heap_size, 0, Pkey,Skey,order);
    }
}




/*
 * Return >0, ==0, <0. 
 */

int CompKey(seq1, seq2, Pkey, Skey)
     Sequence seq1, seq2;
     char Pkey[], Skey[];
{
    int ii, jj, Pret;
    char b1[32], b2[32];

    if(strcmp(Pkey, "type") == 0)
    {
        Pret = strcmp(seq1.type, seq2.type);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "name") == 0)
    {
        Pret = strcmp(seq1.name, seq2.name);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "sequence-ID") == 0)
    {
        Pret = strcmp(seq1.sequence_ID, seq2.sequence_ID);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "creator") == 0)
    {
        Pret = strcmp(seq1.creator, seq2.creator);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "offset") == 0)
    {
        Pret = seq1.offset - seq2.offset;
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "group-ID") == 0)
    {
        Pret = seq1.group_ID - seq2.group_ID;
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "barcode") == 0)
    {
        if(seq1.barcode[0] == 'P')
            strcpy(b1, seq1.barcode+2);
        else
            strcpy(b1, seq1.barcode);

        if(seq2.barcode[0] == 'P')
            strcpy(b2, seq2.barcode+2);
        else
            strcpy(b2, seq2.barcode);
	    
        Pret = strcmp(b1, b2);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "seqlen") == 0)
    {
        Pret = seq1.seqlen - seq2.seqlen;
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "creation-date") == 0)
    {
        seq1.creation_date[0] %= 100;
        seq2.creation_date[0] %= 100;
        Pret = seq1.creation_date[0]*10000
            + seq1.creation_date[1]*100
            + seq1.creation_date[2] 
            - seq2.creation_date[0]*10000 
            - seq2.creation_date[1]*100 
            - seq2.creation_date[2];
        if(Pret == 0)
        {
            Pret = seq1.creation_date[3]*10000
                + seq1.creation_date[4]*100
                + seq1.creation_date[5] 
                - seq2.creation_date[3]*10000 
                - seq2.creation_date[4]*100 
                - seq2.creation_date[5];
        }
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "probing-date") == 0)
    {
        seq1.probing_date[0] %= 100;
        seq2.probing_date[0] %= 100;
        Pret = seq1.probing_date[0]*10000
            + seq1.probing_date[1]*100
            + seq1.probing_date[2] 
            - seq2.probing_date[0]*10000 
            - seq2.probing_date[1]*100 
            - seq2.probing_date[2];
        if(Pret == 0)
        {
            Pret = seq1.probing_date[3]*10000
                + seq1.probing_date[4]*100
                + seq1.probing_date[5] 
                - seq2.probing_date[3]*10000 
                - seq2.probing_date[4]*100 
                - seq2.probing_date[5];
        }
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "autorad_date") == 0)
    {
        seq1.autorad_date[0] %= 100;
        seq2.autorad_date[0] %= 100;
        Pret = seq1.autorad_date[0]*10000
            + seq1.autorad_date[1]*100
            + seq1.autorad_date[2] 
            - seq2.autorad_date[0]*10000 
            - seq2.autorad_date[1]*100 
            - seq2.autorad_date[2];
        if(Pret == 0)
        {
            Pret = seq1.autorad_date[3]*10000
                + seq1.autorad_date[4]*100
                + seq1.autorad_date[5] 
                - seq2.autorad_date[3]*10000 
                - seq2.autorad_date[4]*100 
                - seq2.autorad_date[5];
        }
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "film") == 0)
    {
        Pret = strcmp(seq1.film, seq2.film);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "membrane") == 0)
    {
        Pret = strcmp(seq1.membrane, seq2.membrane);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    else if(strcmp(Pkey, "contig") == 0)
    {
        Pret = strcmp(seq1.contig, seq2.contig);
        if(Pret != 0 || Skey[0] == '\0') return Pret;
    }
    
    else 
    {
        fprintf(stderr,"CompKey(): Invalid primary key %s.\n",Pkey);
        exit(1);
    }

    if(strcmp(Skey, "type") == 0)
    {
        return (strcmp(seq1.type, seq2.type));
    }
    else if(strcmp(Skey, "name") == 0)
    {
        return (strcmp(seq1.name, seq2.name));
    }
    else if(strcmp(Skey, "sequence-ID") == 0)
    {
        return (strcmp(seq1.sequence_ID, seq2.sequence_ID));
    }
    else if(strcmp(Skey, "creator") == 0)
    {
        return (strcmp(seq1.creator, seq2.creator));
    }
    else if(strcmp(Skey, "offset") == 0)
    {
        return (seq1.offset - seq2.offset);
    }
    else if(strcmp(Skey, "group-ID") == 0)
    {
        return (seq1.group_ID - seq2.group_ID);
    }
    else if(strcmp(Skey, "barcode") == 0)
    {
        if(seq1.barcode[0] == 'P')
            strcpy(b1, seq1.barcode+2);
        else
            strcpy(b1, seq1.barcode);

        if(seq2.barcode[0] == 'P')
            strcpy(b2, seq2.barcode+2);
        else
            strcpy(b2, seq2.barcode);
	    
        return (strcmp(b1, b2));
    }
    else if(strcmp(Skey, "seqlen") == 0)
    {
        return(seq1.seqlen - seq2.seqlen);
    }
    else if(strcmp(Skey, "creation-date") == 0)
    {
        seq1.creation_date[0] %= 100;
        seq2.creation_date[0] %= 100;
        Pret = seq1.creation_date[0]*10000
            + seq1.creation_date[1]*100
            + seq1.creation_date[2] 
            - seq2.creation_date[0]*10000 
            - seq2.creation_date[1]*100 
            - seq2.creation_date[2];
        if(Pret != 0)
            return Pret;

        return(seq1.creation_date[3]*10000
               + seq1.creation_date[4]*100
               + seq1.creation_date[5] 
               - seq2.creation_date[3]*10000 
               - seq2.creation_date[4]*100 
               - seq2.creation_date[5]);
    }
    else if(strcmp(Skey, "probing-date") == 0)
    {
        seq1.probing_date[0] %= 100;
        seq2.probing_date[0] %= 100;
        Pret = seq1.probing_date[0]*10000
            + seq1.probing_date[1]*100
            + seq1.probing_date[2] 
            - seq2.probing_date[0]*10000 
            - seq2.probing_date[1]*100 
            - seq2.probing_date[2];
        if(Pret != 0)
            return Pret;

        return(seq1.probing_date[3]*10000
               + seq1.probing_date[4]*100
               + seq1.probing_date[5] 
               - seq2.probing_date[3]*10000 
               - seq2.probing_date[4]*100 
               - seq2.probing_date[5]);
    }
    else if(strcmp(Skey, "autorad_date") == 0)
    {
        seq1.autorad_date[0] %= 100;
        seq2.autorad_date[0] %= 100;
        Pret = seq1.autorad_date[0]*10000
            + seq1.autorad_date[1]*100
            + seq1.autorad_date[2] 
            - seq2.autorad_date[0]*10000 
            - seq2.autorad_date[1]*100 
            - seq2.autorad_date[2];
        if(Pret != 0)
            return Pret;

        return(seq1.autorad_date[3]*10000
               + seq1.autorad_date[4]*100
               + seq1.autorad_date[5] 
               - seq2.autorad_date[3]*10000 
               - seq2.autorad_date[4]*100 
               - seq2.autorad_date[5]);
    }
    else if(strcmp(Skey, "film") == 0)
    {
        return(strcmp(seq1.film, seq2.film));
    }
    else if(strcmp(Skey, "membrane") == 0)
    {
        return(strcmp(seq1.membrane, seq2.membrane));
    }
    else if(strcmp(Skey, "contig") == 0)
    {
        return(strcmp(seq1.contig, seq2.contig));
    }
    else
    {
        fprintf(stderr, "CompKey(): Invalid secondary key %s.\n",Skey);
        exit(1);
    }
}



int Lock(fname)
     char *fname;
{
    char buffer[1024];
    FILE *fp;
    int wait = 0;

    while((fp = fopen(fname, "r")) == NULL)
    {
        sleep(1);
        if(++wait == 30)
        {
            fprintf(stderr, "File %s not available,  Try later.\n\n", fname);
            return FALSE;
        }
    }
    fclose(fp);
    sprintf(buffer, "mv %s %s.locked", fname, fname);
    system(buffer);
    return TRUE;
}


void Unlock(fname)
     char *fname;
{
    char buffer[1024];
    sprintf(buffer, "mv %s.locked %s", fname, fname);
    system(buffer);
}


AppendComments(seq, str)
     Sequence *seq;
     char *str;
{
    int ii, jj, kk;

    kk = strlen(str);

    if(seq->commentsmaxlen == 0)
    {
        seq->comments = (char *)Calloc(kk+1, 1);
        seq->commentsmaxlen = kk+1;
        seq->commentslen = 0;
    }
    else if(seq->commentslen+kk+1>seq->commentsmaxlen)
    {
        seq->commentsmaxlen += 2*(kk+1);
        seq->comments = (char *)
            Realloc(seq->comments, seq->commentsmaxlen);
    }
    seq->comments[seq->commentslen] = '\0';
    seq->comments[seq->commentslen] = '\0';
    strcat(seq->comments, str);
    seq->commentslen = strlen(seq->comments);
}
