#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arbdb.h>
#include <stdarg.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#if defined(SUN4) || defined(SUN5)
# ifndef __cplusplus
#  define SIG_PF void (*)()
# else
#  include <sysent.h>	/* c++ only for sun */
# endif
#else
# define SIG_PF void (*)(int )
#endif

#ifdef SGI
# include <bstring.h>
#endif

#	define FD_SET_TYPE

// Globals
#define AW_GAUGE_SIZE 40
#define AW_STATUS_KILL_DELAY 4000   // in ms
#define AW_STATUS_LISTEN_DELAY 300   // in ms
#define AW_STATUS_HIDE_DELAY 1000*60   // in ms
#define AW_STATUS_PIPE_CHECK_DELAY 1000*2   // in ms every minute a pipe check

#define AW_MESSAGE_LISTEN_DELAY 500
// look in ms whether a father died
#define AW_MESSAGE_LINES 50

enum {
    AW_STATUS_OK,			// status to main
    AW_STATUS_ABORT,		// status to main
    AW_STATUS_CMD_INIT,		// main to status
    AW_STATUS_CMD_OPEN,		// main to status
    AW_STATUS_CMD_CLOSE,
    AW_STATUS_CMD_TEXT,
    AW_STATUS_CMD_GAUGE,
    AW_STATUS_CMD_MESSAGE
};

struct {
    int	fd_to[2];
    int	fd_from[2];
    int	mode;
    int	hide;
    pid_t	pid;
    int	pipe_broken;
    AW_window *aws;
    AW_window *awm;
    AW_BOOL	status_initialized;
    char	*lines[AW_MESSAGE_LINES];
    int	line_cnt;
    int	local_message;
} aw_stg = { {0,0}, {0,0},AW_STATUS_OK,0,0,0,0,0,AW_FALSE, { 0,0,0,},0,0 } ;

void aw_status_timer_listen_event(AW_root *awr, AW_CL cl1, AW_CL cl2);

void aw_status_write( int fd, int cmd)
{
    char buf[10];
    buf[0] = cmd;

    if( write(fd, buf, 1) <= 0  || aw_stg.pipe_broken ){
        printf("Pipe broken.\n");
        exit(0);
    }
}

int aw_status_read_byte(int fd, int poll_flag)
    /* read one byte from the pipe, if poll ==1 then dont wait for any
	data, but return EOF */
{
    int	erg;
    unsigned char buffer[2];

    if (poll_flag){
        fd_set set;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        FD_ZERO (&set);
        FD_SET (fd, &set);

        erg = select(FD_SETSIZE,FD_SET_TYPE &set,NULL,NULL,&timeout);
        if (erg ==0) return EOF;
    }
    erg = read(fd,(char *)&(buffer[0]),1);
    if (erg<=0) {
        //		process died
        printf("father died, now i kill myself\n");
        exit(0);
    }
    return buffer[0];
}

int aw_status_read_command(int fd, int poll_flag, char*& str)
{
    char buffer[1024];
    int cmd = aw_status_read_byte(fd,poll_flag);
    if (	cmd == AW_STATUS_CMD_TEXT ||
            cmd == AW_STATUS_CMD_OPEN ||
            cmd == AW_STATUS_CMD_MESSAGE ) {
        char *p = buffer;
        int c;
        for (
             c = aw_status_read_byte(fd,0);
             c;
             c = aw_status_read_byte(fd,0)){
            *(p++) = c;
        }
        *(p++) = c;
        str = strdup(buffer);
    }else if (cmd == AW_STATUS_CMD_GAUGE){
        int gauge = aw_status_read_byte(fd,0);
        char *p = buffer;
        int i = 0;
        gauge = gauge*AW_GAUGE_SIZE/256;
        for (;i<AW_GAUGE_SIZE;i++){
            if (i<gauge) {
                *(p++) = '*';
            }else{
                *(p++) = '-';
            }
        }
        *p= 0;
        str = strdup(buffer);
    }else{
        str = 0;
    }
    return cmd;
}

void aw_status_check_pipe()
{
    if (getppid() <=1 ) exit(0);
}

void aw_status_wait_for_open(int fd)
{
    char *str = 0;
    int cmd;
    int	erg;

    for (	cmd = 0;
            cmd != AW_STATUS_CMD_INIT;
            ){
        for (erg = 0; !erg; ){
            struct timeval timeout;
            timeout.tv_sec =  AW_STATUS_PIPE_CHECK_DELAY / 1000;
            timeout.tv_usec = (AW_STATUS_PIPE_CHECK_DELAY % 1000) * 1000;

            fd_set set;

            FD_ZERO (&set);
            FD_SET (fd, &set);

            erg = select(FD_SETSIZE,FD_SET_TYPE &set,NULL,NULL,&timeout);
            if (!erg) aw_status_check_pipe();	// time out
        }
        free(str);
        cmd = aw_status_read_command(fd,0,str);
    }
    aw_stg.mode = AW_STATUS_OK;
    delete str;
}


void aw_status_timer_hide_event( AW_root *awr, AW_CL cl1, AW_CL cl2)
{
    AWUSE(awr);AWUSE(cl1);AWUSE(cl2);

    if(aw_stg.hide){
        aw_stg.aws->show();
        aw_stg.hide = 0;
    }
}

void aw_status_hide(AW_window *aws)
{
    aw_stg.hide = 1;
    aws->hide();

    /** install timer event **/
    aws->get_root()->add_timed_callback(AW_STATUS_HIDE_DELAY,
                                        aw_status_timer_hide_event, 0, 0);
}


void aw_status_timer_event( AW_root *awr, AW_CL cl1, AW_CL cl2)
{
    AWUSE(awr);AWUSE(cl1);AWUSE(cl2);

    if(aw_stg.mode == AW_STATUS_ABORT){
        if(aw_message("Couldn't quit properly in time.\nDo you prefer to wait for the abortion or shall I kill the calculating process?",
                      "WAIT,KILL") == 0){
            return;
        }else{
            char buf[255];
            sprintf(buf, "kill -9 %i", aw_stg.pid);
            system(buf);
            exit(0);
        }
    }
}

void aw_status_kill(AW_window *aws)
{
    if(aw_stg.mode == AW_STATUS_ABORT){
        aw_status_timer_event( aws->get_root(), 0, 0);
        return;
    }

    if( aw_message("Are you sure to abort running calculation?","YES,NO")==1){
        return;
    }
    aw_status_write(aw_stg.fd_from[1], AW_STATUS_ABORT);
    aw_stg.mode = AW_STATUS_ABORT;

    /** install timer event **/
    aws->get_root()->add_timed_callback(AW_STATUS_KILL_DELAY,
                                        aw_status_timer_event, 0, 0);
}

void aw_insert_message_in_tmp_message(AW_root *awr,const char *message){
    char *str;
    free(aw_stg.lines[0]);
    int i;
    for (i = 1; i< AW_MESSAGE_LINES; i++){
        aw_stg.lines[i-1] = aw_stg.lines[i];
    };
    aw_stg.lines[AW_MESSAGE_LINES-1] = strdup(message);
    void *stru;
    stru = GBS_stropen(1000);
    for (i = AW_MESSAGE_LINES-1; i>=0; i--){
        if (aw_stg.lines[i]) {
            GBS_intcat(stru,i-AW_MESSAGE_LINES+aw_stg.line_cnt+2);
            GBS_strcat(stru,": ");
            GBS_strcat(stru,aw_stg.lines[i]);
            GBS_strcat(stru,"\n");
        }
    };
    aw_stg.line_cnt ++;
    str = GBS_strclose(stru,0);
    awr->awar("tmp/Message")->write_string(str );
    free(str);

}

void aw_status_timer_listen_event(AW_root *awr, AW_CL, AW_CL)
{
    static int delay = AW_STATUS_LISTEN_DELAY;
    int cmd;
    char *str = 0;
    cmd = aw_status_read_command( aw_stg.fd_to[0],1,str);
    if (cmd == EOF){
        aw_status_check_pipe();
        delay = delay*3/2;	// wait a longer time
    }else{
        delay = delay*2/3 +1;		// shorten time
    }
    char *gauge = 0;
    while(cmd != EOF){
        switch(cmd) {
            case AW_STATUS_CMD_OPEN:
                aw_stg.mode	= AW_STATUS_OK;
                aw_stg.aws->show();
                awr->awar("tmp/Titel")->write_string(str);
                awr->awar("tmp/Gauge")->write_string("----------------------------");
                awr->awar("tmp/Text")->write_string("");
                cmd = EOF;
                free(str);
                continue; // break while loop
            case AW_STATUS_CMD_CLOSE:
                aw_stg.mode	= AW_STATUS_OK;
                aw_stg.aws->hide();
                break;
            case AW_STATUS_CMD_TEXT:
                awr->awar("tmp/Text")->write_string(str);
                break;
            case AW_STATUS_CMD_GAUGE:
                free(gauge);
                gauge = str;
                str = 0;
                break;
            case AW_STATUS_CMD_MESSAGE:
                aw_stg.awm->show();
                aw_insert_message_in_tmp_message(awr,str);
                break;
            default:
                break;
        }
        free(str);
        cmd = aw_status_read_command( aw_stg.fd_to[0],1,str);
    }
    if (gauge){
        awr->awar("tmp/Gauge")->write_string(gauge);
        free(gauge);
    }
    if (delay>AW_STATUS_LISTEN_DELAY) delay = AW_STATUS_LISTEN_DELAY;
    awr->add_timed_callback(delay,aw_status_timer_listen_event, 0, 0);
}

void aw_clear_message_cb(AW_window *aww)
{
    int i;
    AW_root *awr = aww->get_root();
    for (i = 0; i< AW_MESSAGE_LINES; i++){
        free(aw_stg.lines[i]);
        aw_stg.lines[i] = 0;
    };
    awr->awar("tmp/Message")->write_string("" );
}

void aw_clear_and_hide_message_cb(AW_window *aww) {
    aw_clear_message_cb(aww);
    AW_POPDOWN(aww);
}

void aw_initstatus( void )
{
    int	error;

    error = pipe(aw_stg.fd_to);
    if (error) {
        printf("Cannot create socketpair\n");
        exit(-1);
    }
    error = pipe(aw_stg.fd_from);
    if (error) {
        printf("Cannot create socketpair\n");
        exit(-1);
    }


    aw_stg.pid = getpid();
    GB_install_pid(1);
    pid_t clientid = fork();

    if (clientid) {	/* i am the father */
        return;
    } else {
        GB_install_pid(1);
        aw_status_wait_for_open(aw_stg.fd_to[0]);
        AW_root *aw_root;
        aw_root = new AW_root;
        AW_default aw_default = aw_root->open_default(".arb_prop/status.arb");
        aw_root->init_variables(aw_default);
        aw_root->awar_string( "tmp/Titel","------------------------------------",aw_default);
        aw_root->awar_string( "tmp/Text","",aw_default);
        aw_root->awar_string( "tmp/Gauge","------------------------------------",aw_default);
        aw_root->awar_string( "tmp/Message","",aw_default);
        aw_root->init("ARB_STATUS",AW_TRUE);

        AW_window_simple *aws = new AW_window_simple;
        aws->init( aw_root, "STATUS_BOX", "STATUS BOX", 500, 300 );
        aws->load_xfig("status.fig");

        aws->button_length(AW_GAUGE_SIZE+4);
        aws->at("Titel");
        aws->create_button(0,"tmp/Titel");

        aws->at("Text");
        aws->create_button(0,"tmp/Text");

        aws->at("Gauge");
        aws->create_button(0,"tmp/Gauge");

        aws->at("Hide");
        aws->callback(aw_status_hide);
        aws->create_button("HIDE", "HIDE", "h");

        aws->at("Kill");
        aws->callback(aw_status_kill);
        aws->create_button("KILL", "KILL", "k");

        aw_stg.hide = 0;
        aw_stg.aws = (AW_window *)aws;

        AW_window_simple *awm = new AW_window_simple;
        awm->init( aw_root, "MESSAGE_BOX", "MESSAGE BOX", 500, 300 );
        awm->load_xfig("message.fig");

        awm->at("Message");
        awm->create_text_field("tmp/Message",10,2);

        awm->at("Hide");
        awm->callback(AW_POPDOWN);
        awm->create_button("HIDE","HIDE", "h");

        awm->at("Clear");
        awm->callback(aw_clear_message_cb);
        awm->create_button("CLEAR", "CLEAR","C");

        awm->at("HideNClear");
        awm->callback(aw_clear_and_hide_message_cb);
        awm->create_button("HIDE_CLEAR", "OK","O");

        aw_stg.awm = (AW_window *)awm;

        // install irq
        aws->get_root()->add_timed_callback(AW_STATUS_LISTEN_DELAY,
                                            aw_status_timer_listen_event, 0, 0);


        aw_root->main_loop();

    }
}


void aw_openstatus( const char *title )
{
    aw_stg.mode = AW_STATUS_OK;
    if ( !aw_stg.status_initialized) {
        aw_stg.status_initialized = AW_TRUE;
        aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_INIT);
    }
    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_OPEN);
    write(aw_stg.fd_to[1], title, strlen(title)+1 );
}

void aw_closestatus( void )
{
    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_CLOSE);
}

int aw_status( const char *text )
{
    if (!text) text = "";

    aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_TEXT);
    write(aw_stg.fd_to[1], text, strlen(text)+1 );

    return aw_status();
}

#ifdef __cplusplus
extern "C" {
#endif

    int aw_status( double gauge ) {
        static long last_val = -1;
        long val = (int)(gauge*255);
        if (val != last_val) {
            aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_GAUGE);
            aw_status_write(aw_stg.fd_to[1], (int)(gauge*255));
        }
        last_val = val;
        return aw_status();
    }

#ifdef __cplusplus
}
#endif

int aw_status( void )
{
    char *str = 0;
    int cmd;
    if (aw_stg.mode == AW_STATUS_ABORT) return AW_STATUS_ABORT;
    for (	cmd = 0;
            cmd != EOF;
            cmd = aw_status_read_command(aw_stg.fd_from[0],1,str)){
        delete str;
        if (cmd == AW_STATUS_ABORT) 	aw_stg.mode = AW_STATUS_ABORT;
    }

    return aw_stg.mode;
}

/***********************************************************************/
/*****************		AW_MESSAGE	     *******************/
/***********************************************************************/
int aw_message_cb_result;

void message_cb( AW_window *aw, AW_CL cd1 ) {
    AWUSE(aw);
    long result = (long)cd1;
    if ( result == -1 ) { /* exit */
        exit(EXIT_FAILURE);
    }
    aw_message_cb_result = ((int)result);
    return;
}



void aw_message_timer_listen_event(AW_root *awr, AW_CL cl1, AW_CL cl2)
{
    AW_window *aww = ((AW_window *)cl1);
    if (aww->get_show()){
        aww->show();
        awr->add_timed_callback(AW_MESSAGE_LISTEN_DELAY,
                                aw_message_timer_listen_event, cl1, cl2);
    }
}

void aw_set_local_message(){
    aw_stg.local_message = 1;
}

int aw_message(const char *msg, const char *buttons, bool fixedSizeButtons) {
    // return 0 for first button, 1 for second button, 2 for third button, ...
    //
    // the single buttons are seperated by kommas (i.e. "YES,NO")
    // If the button-name starts with ^ it starts a new row of buttons
    // (if a button named 'EXIT' is pressed the program terminates using exit(EXIT_FAILURE))
    //
    // If fixedSizeButtons is true all buttons have the same size
    // otherwise the size for every button is set depending on the text length

    if (!buttons){		        /* asynchronous message */
#if defined(DEBUG)
        printf("aw_message: '%s'\n", msg);
#endif // DEBUG
        if (aw_stg.local_message){
            aw_insert_message_in_tmp_message(AW_root::THIS,msg);
        }else{
            if ( !aw_stg.status_initialized) {
                aw_stg.status_initialized = AW_TRUE;
                aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_INIT);
            }
            aw_status_write(aw_stg.fd_to[1], AW_STATUS_CMD_MESSAGE);
            write(aw_stg.fd_to[1], msg, strlen(msg)+1 );
        }
        return 0;
    }

    AW_root *root = AW_root::THIS;

    AW_window_message *aw_msg;
    char              *button_list;

    if (buttons) button_list = strdup( buttons );
    else button_list         = strdup("OK");

    static GB_HASH *hash_windows = 0;
    if (!hash_windows) hash_windows = GBS_create_hash(256,0);
    if (!msg) msg = "Unknown Message";
    char *hindex = (char *)calloc(sizeof(char),strlen(msg) + strlen(button_list) + 3);
    sprintf(hindex,"%s&&%s",msg,button_list);

    aw_msg = (AW_window_message *)GBS_read_hash(hash_windows,hindex);
    if (!aw_msg) {
        aw_msg = new AW_window_message;
        GBS_write_hash(hash_windows,hindex,(long)aw_msg);
        int counter = 0;

        aw_msg->init( root, "QUESTION BOX", 1000, 1000, 300, 300 );

        aw_msg->label_length( 10 );
        aw_msg->button_length( 0 );

        aw_msg->at( 10, 10 );
        aw_msg->auto_space( 10, 10 );
        {
            char *hmsg = GBS_string_eval(msg,"*/*= */*",0);
            aw_msg->create_button( 0,hmsg );
            free(hmsg);
        }

        aw_msg->at_newline();

        if (fixedSizeButtons) {
            size_t  max_button_length = 0;
            char   *pos               = button_list;

            while (1) {
                char *komma       = strchr(pos, ',');
                if (!komma) komma = strchr(pos, 0);

                size_t len                                   = komma-pos;
                if (len>max_button_length) max_button_length = len;

                if (!komma[0]) break;
                pos = komma+1;
            }

            aw_msg->button_length(max_button_length+1);
        }

        char *ret = strtok( button_list, "," );
        while( ret ) {
            if (ret[0] == '^') {
                aw_msg->at_newline();
                ++ret;
            }
            if ( strcmp( ret, "EXIT" ) == 0 ) {
                aw_msg->callback     ( message_cb, -1 );
            }else {
                aw_msg->callback     ( message_cb, (AW_CL)counter++ );
            }

            if (fixedSizeButtons) {
                aw_msg->create_button( 0,ret);
            }
            else {
                aw_msg->create_autosize_button( 0,ret);
            }
            ret = strtok( NULL, "," );
        }

        aw_msg->window_fit();
    }
    free(hindex);
    aw_msg->show_grabbed();

    free(button_list);
    aw_message_cb_result = -13;

    root->add_timed_callback(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = AW_TRUE;
    while (aw_message_cb_result == -13) {
        root->process_events();
    }
    root->disable_callbacks = AW_FALSE;
    aw_msg->hide();

    switch ( aw_message_cb_result ) {
        case -1: /* exit with core */
            GB_CORE;
            break;
        case -2: /* exit without core */
            exit( -1 );
            break;
    }
    return aw_message_cb_result;
}

void aw_message(const char *msg)
{
    aw_message(msg,0);
}

char AW_ERROR_BUFFER[1024];

void aw_message(void){ aw_message(AW_ERROR_BUFFER,0); }

void aw_error(const char *text,const char *text2){
    char	buffer[1024];
    sprintf(buffer,"An internal error occur:\n\n%s %s\n\nYou may:",text,text2);
    aw_message(buffer,"CONTINUE,EXIT");
}

/***********************************************************************/
/*****************		AW_INPUT	     *******************/
/***********************************************************************/
char  *aw_input_cb_result = 0;

static int aw_string_selected_button = -1;

int aw_string_selection_button() {
    aw_assert(aw_string_selected_button != -1);
    return aw_string_selected_button;
}

#define AW_INPUT_AWAR "tmp/input/string"
#define AW_INPUT_TITLE_AWAR "tmp/input/title"

void input_cb( AW_window *aw, AW_CL cd1 ) {
    aw_input_cb_result        = 0;
    aw_string_selected_button = int(cd1);
    if (cd1<0) return;
    aw_input_cb_result = aw->get_root()->awar(AW_INPUT_AWAR)->read_as_string();
    return;
}

char *aw_input( const char *title, const char *prompt, const char *awar_value, const char *default_input)
{
    AW_root *root = AW_root::THIS;

    static AW_window_message *aw_msg = 0;
    root->awar_string(AW_INPUT_TITLE_AWAR)->write_string((char *)prompt);
    if (awar_value) {
        root->awar_string(AW_INPUT_AWAR)->map(awar_value);
    }else{
        root->awar_string(AW_INPUT_AWAR)->write_string(default_input ? default_input : "");
    }

    if (!aw_msg) {
        aw_msg = new AW_window_message;

        aw_msg->init( root, title, 1000, 1000, 300, 300 );

        aw_msg->label_length( 0 );
        aw_msg->button_length( 30 );

        aw_msg->at( 10, 10 );
        aw_msg->auto_space( 10, 10 );
        aw_msg->create_button( 0,AW_INPUT_TITLE_AWAR );

        aw_msg->at_newline();
        aw_msg->create_input_field((char *)AW_INPUT_AWAR,30);
        aw_msg->at_newline();

        aw_msg->button_length( 0 );

        aw_msg->callback     ( input_cb, 0 );
        aw_msg->create_button( "OK", "OK", "O" );

        aw_msg->callback     ( input_cb, -1 );
        aw_msg->create_button( "CANCEL", "CANCEL", "C" );
    }

    aw_msg->window_fit();
    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback(AW_MESSAGE_LISTEN_DELAY,	aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = AW_TRUE;
    while (aw_input_cb_result == dummy) {
        root->process_events();
    }
    root->disable_callbacks = AW_FALSE;
    aw_msg->hide();

    if (awar_value){
        root->awar_string(AW_INPUT_AWAR)->unmap();
    }
    return aw_input_cb_result;
}

char *aw_input( const char *prompt, const char *awar_value, const char *default_input)
{
    return aw_input("ENTER A STRING", prompt, awar_value, default_input);
}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//      char *aw_string_selection(const char *title, const char *prompt, const char *awar_name, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*))
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// A modal input window. A String may be entered by hand or selected from value_list
//
//      title           window title
//      prompt          prompt at input field
//      awar_name       name of awar to use for inputfield (if NULL => internal awar)
//      default_value   default value (if NULL => ""). Not used in case of awar_name != NULL
//      value_list      Existing selections (seperated by ';' ; if NULL => uses aw_input)
//      check_fun       function to correct input (or 0 for no check). The function may return 0 to indicate no correction
//
// returns the value of the inputfield
//
char *aw_string_selection(const char *title, const char *prompt, const char *awar_name, const char *default_value, const char *value_list, const char *buttons, char *(*check_fun)(const char*))
{
    if (!value_list) return aw_input(title, prompt, awar_name, default_value);

    AW_root *root = AW_root::THIS;

    static AW_window_message *aw_msg = 0;
    static AW_selection_list *sel = 0;

    root->awar_string(AW_INPUT_TITLE_AWAR)->write_string((char *)prompt);
    if (awar_name) {
        root->awar_string(AW_INPUT_AWAR)->map(awar_name);
    }else{
        root->awar_string(AW_INPUT_AWAR)->write_string(default_value ? default_value : "");
    }

    if (!aw_msg) {
        int width           = strlen(prompt)+1;
        if (width<30) width = 30;

        aw_msg = new AW_window_message;

        aw_msg->init( root, title, 1000, 1000, 300, 300 );

        aw_msg->label_length( 0 );
        aw_msg->button_length(width);

        aw_msg->at( 10, 10 );
        aw_msg->auto_space( 10, 10 );
        aw_msg->create_button( 0,AW_INPUT_TITLE_AWAR );

        aw_msg->at_newline();
        aw_msg->create_input_field((char *)AW_INPUT_AWAR,width);
        aw_msg->at_newline();

        sel = aw_msg->create_selection_list(AW_INPUT_AWAR, 0, 0, width, 10);
        aw_msg->insert_default_selection(sel, "", "");
        aw_msg->update_selection_list(sel);

        aw_msg->at_newline();

        if (buttons)  {
            char *but    = GB_strdup(buttons);
            char *word;
            int   result = 0;

            aw_msg->button_length(9);

            for (word = strtok(but, ","); word; word = strtok(0, ","), ++result) {
                aw_msg->callback(input_cb, result);
                aw_msg->create_button(word, word, "");
            }

            free(but);
        }
        else {
            aw_msg->button_length( 0 );

            aw_msg->callback     ( input_cb, 0 );
            aw_msg->create_button( "OK", "OK", "O" );

            aw_msg->callback     ( input_cb, -1 );
            aw_msg->create_button( "CANCEL", "CANCEL", "C" );
        }

        aw_msg->window_fit();
    }

    // update the selection box :
    aw_assert(sel);
    aw_msg->clear_selection_list(sel);
    {
        char *values = GB_strdup(value_list);
        char *word;

        for (word = strtok(values, ";"); word; word = strtok(0, ";")) {
            aw_msg->insert_selection(sel, word, word);
        }
        free(values);
    }
    aw_msg->insert_default_selection(sel, "<new>", "");
    aw_msg->update_selection_list(sel);

    // do modal loop :
    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback(AW_MESSAGE_LISTEN_DELAY,	aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = AW_TRUE;

    char *last_input = root->awar(AW_INPUT_AWAR)->read_string();
    while (aw_input_cb_result == dummy) {
        root->process_events();

        char *this_input = root->awar(AW_INPUT_AWAR)->read_string();
        if (strcmp(this_input, last_input) != 0) {
            if (check_fun) {
                char *corrected_input = check_fun(this_input);
                if (corrected_input) {
                    if (strcmp(corrected_input, this_input) != 0) {
                        root->awar(AW_INPUT_AWAR)->write_string(corrected_input);
                    }
                    free(corrected_input);
                }
            }
            free(last_input);
            last_input = this_input;
            this_input = 0;
        }
        free(this_input);
    }

    free(last_input);

    root->disable_callbacks = AW_FALSE;
    aw_msg->hide();

    if (awar_name){
        root->awar_string(AW_INPUT_AWAR)->unmap();
    }
    return aw_input_cb_result;
}

/***********************************************************************/
/*****************		AW_FILESELECTION     *******************/
/***********************************************************************/

#define AW_FILE_SELECT_DIR_AWAR "tmp/file_select/directory"
#define AW_FILE_SELECT_FILE_AWAR "tmp/file_select/file_name"
#define AW_FILE_SELECT_FILTER_AWAR "tmp/file_select/filter"
#define AW_FILE_SELECT_TITLE_AWAR "tmp/file_select/title"


char *aw_file_selection( const char *title, const char *dir, const char *def_name, const char *suffix )

{
    aw_assert(0); // function is not used yet
    AW_root *root = AW_root::THIS;

    static AW_window_message *aw_msg = 0;
    root->awar_string(AW_FILE_SELECT_TITLE_AWAR)->write_string((char *)title);
    root->awar_string(AW_FILE_SELECT_DIR_AWAR)->write_string((char *)dir);
    root->awar_string(AW_FILE_SELECT_FILE_AWAR)->write_string((char *)def_name);
    root->awar_string(AW_FILE_SELECT_FILTER_AWAR)->write_string((char *)suffix);

    if (!aw_msg) {
        aw_msg = new AW_window_message;

        aw_msg->init( root, "ENTER A STRING", 1000, 1000, 300, 300 );

        aw_msg->label_length( 0 );
        aw_msg->button_length( 30 );

        aw_msg->at( 10, 10 );
        aw_msg->auto_space( 10, 10 );
        aw_msg->create_button( 0,AW_FILE_SELECT_TITLE_AWAR );

        aw_msg->at_newline();
        // awt_create_selection_box(aw_msg,"tmp/file_select",80,30);

        aw_msg->at_newline();

        aw_msg->button_length( 0 );

        aw_msg->callback     ( input_cb, 0 );
        aw_msg->create_button( "OK", "OK", "O" );

        aw_msg->callback     ( input_cb, -1 );
        aw_msg->create_button( "CANCEL", "CANCEL", "C" );

        aw_msg->window_fit();
    }

    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback(AW_MESSAGE_LISTEN_DELAY,	aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = AW_TRUE;
    while (aw_input_cb_result == dummy) {
        root->process_events();
    }
    root->disable_callbacks = AW_FALSE;
    aw_msg->hide();

    return aw_input_cb_result;
}

/***********************************************************************/
/**********************		HELP WINDOW	************************/
/***********************************************************************/

struct {
    AW_window *aww;
    AW_selection_list	*upid;
    AW_selection_list	*downid;
    char	*history;
}	aw_help_global;

//  ---------------------------------------------------------------------------------------------------------
//      static char *get_full_qualified_help_file_name(const char *helpfile, bool path_for_edit = false)
//  ---------------------------------------------------------------------------------------------------------
static char *get_full_qualified_help_file_name(const char *helpfile, bool path_for_edit = false) {
    GB_CSTR  result             = 0;
    char    *user_doc_path      = strdup(GB_getenvDOCPATH());
    char    *devel_doc_path     = GBS_global_string_copy("%s/HELP_SOURCE/oldhelp", GB_getenvARBHOME());
    size_t   user_doc_path_len  = strlen(user_doc_path);
    size_t   devel_doc_path_len = strlen(devel_doc_path);

    const char *rel_path = 0;
    if (strncmp(helpfile, user_doc_path, user_doc_path_len) == 0 && helpfile[user_doc_path_len] == '/') {
        rel_path = helpfile+user_doc_path_len+1;
    }
    else if (strncmp(helpfile, devel_doc_path, devel_doc_path_len) == 0 && helpfile[devel_doc_path_len] == '/') {
        rel_path = helpfile+devel_doc_path_len+1;
    }

    if (helpfile[0]=='/' && !rel_path) {
        result = GBS_global_string("%s", helpfile);
    }
    else {
        if (!rel_path) rel_path = helpfile;

        if (path_for_edit) {
            result = GBS_global_string("%s/HELP_SOURCE/oldhelp/%s", GB_getenvARBHOME(), rel_path);
        }
        else {
            result = GBS_global_string("%s/%s", GB_getenvDOCPATH(), rel_path);
        }
    }

#if defined(DEBUG)
    printf("Helpfile='%s'\n", result);
#endif // DEBUG

    free(devel_doc_path);
    free(user_doc_path);

    return strdup(result);
}

//  ------------------------------------------------------------------------
//      static GB_CSTR get_full_qualified_help_file_name(AW_root *root)
//  ------------------------------------------------------------------------
static char *get_full_qualified_help_file_name(AW_root *awr, bool path_for_edit = false) {
    char *helpfile = awr->awar("tmp/aw_window/helpfile")->read_string();
    char *qualified = get_full_qualified_help_file_name(helpfile, path_for_edit);
    free(helpfile);
    return qualified;
}

void aw_help_edit_help(AW_window *aww)
{
    AWUSE(aww);
    char buffer[1024];
    char *helpfile = get_full_qualified_help_file_name(aww->get_root(), true);

    if (GB_size_of_file(helpfile)<=0){
        sprintf(buffer,"cp %s/HELP_SOURCE/oldhelp/FORM.hlp %s", GB_getenvARBHOME(), helpfile);
        printf("%s\n",buffer);
        system(buffer);
    }

    sprintf(buffer,"textedit %s &", helpfile); // we use textedit to edit help-files to ensure correct format
    printf("%s\n",buffer);
    system(buffer);

    free(helpfile);
}
char *aw_ref_to_title(char *ref){
    if (!ref) return 0;

    if (!GBS_string_cmp(ref,"*.ps",0) ){	// Postscript file
        return GBS_global_string_copy("Postscript: %s",ref);
    }

    char *result = 0;
    char *file = 0;
    {
        char *helpfile = get_full_qualified_help_file_name(ref);
        file = GB_read_file(helpfile);
        free(helpfile);
    }

    if (file) {
        result = GBS_string_eval(file,"*\nTITLE*\n*=*2:\t=",0);
        if (strcmp(file, result)==0) {
            free(result);
            result = 0;
        }
        free(file);
    }

    if (result==0) {
        result = strdup(ref);
    }

    return result;
}
void aw_help_new_helpfile(AW_root *awr){
    char *help_file = get_full_qualified_help_file_name(awr);

    if (!strlen(help_file)) {
        awr->awar("tmp/aw_window/helptext")->write_string("no help\0");
    }
    else if (!GBS_string_cmp(help_file,"*.ps",0) ){ // Postscript file
        struct stat st;
        char        sys[1024];

        sys[0] = 0;

        if (stat(help_file, &st) == 0) { // *.ps exists
            sprintf(sys,"%s %s &",GB_getenvGS(), help_file);
        }
        else {
            char *compressed = GBS_global_string_copy("%s.gz", help_file);

            if (stat(compressed, &st) == 0) { // *.ps.gz exists
                sprintf(sys,"(gunzip <%s | %s -) &", compressed, GB_getenvGS());
            }
            else {
                sprintf(AW_ERROR_BUFFER, "Neither %s nor %s where found", help_file, compressed);
                aw_message();
            }
            free(compressed);
        }

        if (system(sys)){
            sprintf(AW_ERROR_BUFFER,"Error calling: %s",sys);
            aw_message();
        }
    }
    else{
        if (aw_help_global.history){
            if (strncmp(help_file,aw_help_global.history,strlen(help_file))){
                char comm[1024];
                char *h;
                sprintf(comm,"*=%s#*1",help_file);
                h = GBS_string_eval(aw_help_global.history,comm,0);
                free(aw_help_global.history);
                aw_help_global.history = h;
            }
        }
        else {
            aw_help_global.history = strdup(help_file);
        }

        char *helptext=GB_read_file(help_file);
        if(helptext) {
            char *ptr;
            char *h,*h2,*tok;

            ptr = strdup(helptext);
            aw_help_global.aww->clear_selection_list(aw_help_global.upid);
            h2 = GBS_find_string(ptr,"\nUP",0);
            while (	(h = h2) ){
                h2 = GBS_find_string(h2+1,"\nUP",0);
                tok = strtok(h+3," \n\t");	// now I got UP
                char *title = aw_ref_to_title(tok);
                if (tok) aw_help_global.aww->insert_selection(aw_help_global.upid,title,tok);
                free(title);
            }
            free(ptr);
            aw_help_global.aww->insert_default_selection(aw_help_global.upid,"   ","");
            aw_help_global.aww->update_selection_list(aw_help_global.upid);

            ptr = strdup(helptext);
            aw_help_global.aww->clear_selection_list(aw_help_global.downid);
            h2 = GBS_find_string(ptr,"\nSUB",0);
            while (	(h = h2) ){
                h2 = GBS_find_string(h2+1,"\nSUB",0);
                tok = strtok(h+4," \n\t");	// now I got SUB
                char *title = aw_ref_to_title(tok);
                if (tok) aw_help_global.aww->insert_selection(
                                                              aw_help_global.downid,	title,tok);
                free(title);
            }
            free(ptr);
            aw_help_global.aww->insert_default_selection(aw_help_global.downid,"   ","");
            aw_help_global.aww->update_selection_list(aw_help_global.downid);

            ptr = GBS_find_string(helptext,"TITLE",0);
            if (!ptr) ptr = helptext;
            ptr = GBS_string_eval(ptr,"{*\\:*}=*2",0);

            awr->awar("tmp/aw_window/helptext")->write_string(ptr);
            free(ptr);
            free(helptext);
        }else{
            sprintf(AW_ERROR_BUFFER,"I cannot find the help file '%s'\n\n"
                    "Please help us to complete the ARB-Help by submitting\n"
                    "this missing helplink via ARB_NT/File/About/SubmitBug\n"
                    "Thank you.\n",help_file);
            awr->awar("tmp/aw_window/helptext")->write_string(AW_ERROR_BUFFER);
        }
    }
    free(help_file);
}

void aw_help_back(AW_window *aww)
{
    if (!aw_help_global.history) return;
    if (!strchr(aw_help_global.history,'#') ) return;
    char *newhist = GBS_string_eval(aw_help_global.history,"*#*=*2",0);	// delete first
    free(aw_help_global.history);
    aw_help_global.history = newhist;
    char *helpfile = GBS_string_eval(aw_help_global.history,"*#*=*1",0);	// first word
    aww->get_root()->awar("tmp/aw_window/helpfile")->write_string(helpfile);
    free(helpfile);

}

//  --------------------------------------------
//      void aw_help_search(AW_window *aww)
//  --------------------------------------------
void aw_help_search(AW_window *aww) {
    char *searchtext = aww->get_root()->awar("tmp/aw_window/search_expression")->read_string();

    if (searchtext[0]==0) {
        aw_message("Enter a searchstring");
    }
    else {
        char        *helpfilename = 0;
        static char *last_help;

        {
            char *searchtext2 = GBS_string_eval(searchtext, " =.*", 0); // replace all spaces by '.*'
            if (searchtext2) {
                free(searchtext);
                searchtext = searchtext2;
            }
        }

        {
            static int counter;
            char buffer[1024];
            sprintf(buffer, "/tmp/arb_tmp_%s_%i_%i.hlp", GB_getenv("USER"), getpid(), ++counter);
            helpfilename = strdup(buffer);

            sprintf(buffer,
                    "cd %s;grep -i '%s' `find . -name \"*.hlp\"` | sed -e \"s/:.*//g\" -e \"s/^\\.\\///g\" | sort | uniq > %s",
                    GB_getenvDOCPATH(), searchtext, helpfilename);

            printf("%s\n", buffer);
            system(buffer);
        }

        char *result = GB_read_file(helpfilename);
        if (result) {
            // write temporary helpfile:
            FILE *helpfp = fopen(helpfilename, "wt");
            if (!helpfp) {
                aw_message(GBS_global_string("Can't create tempfile '%s'", helpfilename));
            }
            else {
                fprintf(helpfp, "\nUP arb.hlp\n");
                if (last_help) fprintf(helpfp, "UP %s\n", last_help);
                fputc('\n', helpfp);

                int results = 0;
                char *rp = result;
                while (1) {
                    char *eol = strchr(rp, '\n');
                    if (!eol) {
                        eol = rp;
                        while (*eol) ++eol;
                    }
                    if (eol>rp) {
                        char old = eol[0];
                        eol[0] = 0;
                        fprintf(helpfp, "SUB %s\n", rp);
                        results++;
                        eol[0] = old;
                    }
                    if (eol[0]==0) break; // all results inserted
                    rp = eol+1;
                }

                fprintf(helpfp,"\nTITLE\t\tResult of search for '%s'\n\n", searchtext);
                if (results>0)  fprintf(helpfp, "\t\t%i results are shown as subtopics\n",  results);
                else            fprintf(helpfp, "\t\tThere are no results.\n");

                if (results>0) {
                    if (last_help) free(last_help);
                    last_help = strdup(helpfilename);
                }

                fclose(helpfp);
                aww->get_root()->awar("tmp/aw_window/helpfile")->write_string(helpfilename); // display results in helpwindow
            }
            free(result);
        }
        else {
            aw_message("Can't read search results");
        }
        free(helpfilename);
    }
    free(searchtext);
}

void AW_POPUP_HELP(AW_window *aw,AW_CL /*char */ helpcd)
{
    AW_root *awr=aw->get_root();
    static AW_window_simple *helpwindow=0;


    char *help_file= (char*)helpcd;


    if(!helpwindow) {
        awr->awar_string( "tmp/aw_window/helptext", "" , AW_ROOT_DEFAULT);
        awr->awar_string( "tmp/aw_window/search_expression", "" , AW_ROOT_DEFAULT);
        awr->awar_string( "tmp/aw_window/helpfile", "" , AW_ROOT_DEFAULT);
        awr->awar("tmp/aw_window/helpfile")->add_callback(aw_help_new_helpfile);

        helpwindow=new AW_window_simple;
        helpwindow->init(awr,"HELP","HELP WINDOW",200,10);
        helpwindow->load_xfig("help.fig");

        helpwindow->button_length(10);

        helpwindow->at("close");
        helpwindow->callback((AW_CB0)AW_POPDOWN);
        helpwindow->create_button("CLOSE", "CLOSE","C");

        helpwindow->at("edit");
        helpwindow->callback(aw_help_edit_help);
        helpwindow->create_button("EDIT", "EDIT","E");

        helpwindow->at("back");
        helpwindow->callback(aw_help_back);
        helpwindow->create_button("BACK", "BACK","B");


        helpwindow->at("super");
        aw_help_global.upid = helpwindow->create_selection_list("tmp/aw_window/helpfile",0,0,3,3);
        helpwindow->insert_default_selection(aw_help_global.upid,"   ","");
        helpwindow->update_selection_list(aw_help_global.upid);

        helpwindow->at("sub");
        aw_help_global.downid = helpwindow->create_selection_list("tmp/aw_window/helpfile",0,0,3,3);
        helpwindow->insert_default_selection(aw_help_global.downid,"   ","");
        helpwindow->update_selection_list(aw_help_global.downid);
        aw_help_global.aww = helpwindow;
        aw_help_global.history = 0;

        helpwindow->at("text");
        helpwindow->create_text_field("tmp/aw_window/helptext",3,3);

        helpwindow->at("expression");
        helpwindow->create_input_field("tmp/aw_window/search_expression");

        helpwindow->at("search");
        helpwindow->callback(aw_help_search);
        helpwindow->create_button("SEARCH", "SEARCH", "S");
    }
    free(aw_help_global.history);
    aw_help_global.history = 0;

    awr->awar("tmp/aw_window/helpfile")->write_string("");
    if(help_file) awr->awar("tmp/aw_window/helpfile")->write_string(help_file);
    if (!GBS_string_cmp(help_file,"*.ps",0)) return;// dont open help if postscript file
    helpwindow->show();
}

/***********************************************************************/
/**********************		HELP WINDOW	************************/
/***********************************************************************/



void AW_ERROR( const char *templat, ...) {
    char buffer[10000];
    va_list	parg;
    char *p;
    sprintf(buffer,"Internal ARB Error: ");
    p = buffer + strlen(buffer);

    va_start(parg,templat);

    vsprintf(p,templat,parg);
    fprintf(stderr,"%s\n",buffer);

    if (GBS_do_core()){
        GB_CORE;
    }else{
        fprintf(stderr,"Debug file $ARBHOME/do_core not found -> continuing operation \n");
    }
    aw_message(buffer);
}
