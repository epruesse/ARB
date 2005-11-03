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


void ShowMessageBox(Widget widget, char *title, char *text)
{
    Widget msgBox;
    Arg args[3];
    XtSetArg(args[0], XmNmessageString, XmStringCreateLocalized(text));
    msgBox= XmCreateMessageDialog(widget , "messageBox", args, 1);

    XtUnmanageChild(XmMessageBoxGetChild(msgBox, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msgBox, XmDIALOG_CANCEL_BUTTON));

    XtVaSetValues(msgBox, XmNdialogTitle, XmStringCreateLocalized(title), NULL);

    XtManageChild(msgBox);
}


int OkCancelDialog(Widget w, char *title, char *text)
{
    int answer= 0;
    Widget dialog;
    Arg args[3];
    int n= 0;
    XtAppContext context;

    XtSetArg(args[n], XmNmessageString, XmStringCreateLtoR(text, XmSTRING_DEFAULT_CHARSET)); n++;
    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL); n++;

    dialog = XmCreateQuestionDialog(XtParent(w), text, args, n);
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(dialog, XmNdialogTitle, XmStringCreateLocalized(title), NULL);

    XtAddCallback(dialog, XmNokCallback, OkCancelResponse, &answer);
    XtAddCallback(dialog, XmNcancelCallback, OkCancelResponse, &answer);
    XtManageChild(dialog);

    context= XtWidgetToApplicationContext (w);
    while (answer == 0 || XtAppPending(context))
    {
        XtAppProcessEvent (context, XtIMAll);
    }
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
