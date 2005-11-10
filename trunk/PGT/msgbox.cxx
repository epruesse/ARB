// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#include "msgbox.hxx"
#include "xm_defs.hxx"


void ShowMessageBox(Widget widget, const char *title, const char *text)
{
    Widget msgBox;
    Arg args[3];

    // CREATE X11 TITLE- AND TEXT-STRING
    XmString xm_text=  PGT_XmStringCreateLocalized(text);
    XmString xm_title= PGT_XmStringCreateLocalized(title);

    XtSetArg(args[0], XmNmessageString, xm_text);
    msgBox= XmCreateMessageDialog(widget , const_cast<char*>("messageBox"), args, 1);

    XtUnmanageChild(XmMessageBoxGetChild(msgBox, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msgBox, XmDIALOG_CANCEL_BUTTON));

    XtVaSetValues(msgBox, XmNdialogTitle, xm_title, NULL);

    XtManageChild(msgBox);

    // FREE STRINGS
    XmStringFree(xm_text);
    XmStringFree(xm_title);
}


int OkCancelDialog(Widget w, const char *title, const char *text)
{
    int answer= 0;
    Widget dialog;
    Arg args[3];
    int n= 0;
    XtAppContext context;

    // CREATE X11 TITLE- AND TEXT-STRING
    XmString xm_text=  XmStringCreateLtoR(const_cast<char*>(text), XmSTRING_DEFAULT_CHARSET);
    XmString xm_title= PGT_XmStringCreateLocalized(title);

    XtSetArg(args[n], XmNmessageString, xm_text); n++;
    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL); n++;

    dialog = XmCreateQuestionDialog(XtParent(w), const_cast<char*>(text), args, n);
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(dialog, XmNdialogTitle, xm_title, NULL);

    XtAddCallback(dialog, XmNokCallback, OkCancelResponse, &answer);
    XtAddCallback(dialog, XmNcancelCallback, OkCancelResponse, &answer);
    XtManageChild(dialog);

    context= XtWidgetToApplicationContext (w);
    while (answer == 0 || XtAppPending(context))
    {
        XtAppProcessEvent (context, XtIMAll);
    }

    // FREE STRINGS
    XmStringFree(xm_text);
    XmStringFree(xm_title);

    XtDestroyWidget(dialog);

    return answer;
}


void OkCancelResponse(Widget, XtPointer client, XtPointer call)
{
    int *answer= (int *)client;
    XmAnyCallbackStruct *reason= (XmAnyCallbackStruct *)call;
    switch(reason->reason)
    {
        case XmCR_OK:
            *answer= 1;
            break;
        case XmCR_CANCEL:
            *answer= 2;
            break;
        default:
            return;
    }
}
