//  ==================================================================== //
//                                                                       //
//    File      : Toolkit.java                                           //
//    Purpose   : Functions uses in all classes go here                  //
//    Time-stamp: <Thu Mar/11/2004 23:45 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

import java.awt.*;
import java.util.*;

class Toolkit
{
    public static String clientName = "arb_probe_client";

    // search globally for 'CLIENT_SERVER_VERSIONS' (other occurance is in ../SERVER/getVersion.cgi)
    public static String client_version    = "1.0"; // if client_version does not match, a notice is printed
    public static String interface_version = "1.0"; // if interface_version does not match, client terminates!

    private static String maintainer = "probeadmin@arb-home.de";

    private static Client theClient = null;
    public static void registerClient(Client c) { theClient = c; }
    public static void unregisterClient() { theClient = null; }

    public static void showError(String error) {
        System.out.println(error);
        if (theClient != null) theClient.show_error(error);
    }
    public static void showMessage(String message) {
        System.out.println(message);
        if (theClient != null) theClient.show_message(message);
    }

    private static void AbortWithError_Internal(String kind_of_error, String error_message, int exitcode) throws Exception {
        System.out.println(kind_of_error+": "+error_message);
        throw new ClientException(kind_of_error, error_message, exitcode);
    }

    public static void AbortWithError(String error) throws Exception {
        AbortWithError_Internal("Error in "+clientName, error, 1);
    }

    public static void AbortWithConnectionProblem(String error) throws Exception {
        AbortWithError_Internal(clientName+" has a connection problem:",
                                error+"\nPlease check whether your internet connection works", 2);
    }

    public static void AbortWithServerProblem(String error) throws Exception {
        AbortWithError_Internal(clientName+" got error from server:",
                                error+"\nPlease report to "+maintainer, 2);
    }

    public static void InternalError(String error) throws Exception {
        AbortWithError_Internal("Internal error in "+clientName+" v"+client_version,
                                error+"\n(this seems to be a bug, please report to "+maintainer+")", 666);
    }

    public static void ExpectNoError(String error) throws Exception
    {
        if (error != null) {
            InternalError("Unexpected error: "+error);
        }
    }

    // dialogs

    private static Frame getFrame() {
        if (theClient == null) {
            System.out.println("Can't get frame!");
            return null;
        }
        return theClient.getDisplay();
    }

    public static String askUser(String title, String text, String buttonlist) {
	Vector buttons = new Vector();

        int start = 0;
        int len   = buttonlist.length();
        while (start<len) {
            int comma = buttonlist.indexOf(',', start);
            if (comma == -1) {
                buttons.add(buttonlist.substring(start));
                start = len;
            }
            else {
                buttons.add(buttonlist.substring(start, comma));
                start = comma+1;
            }
        }

 	DialogWrapper dw = new DialogWrapper(getFrame(), title, text, buttons);
        return dw.getResult();
    }

    public static void clickButton(String title, String text, String button) {
        Vector buttons = new Vector();
        buttons.add(button);
        
 	DialogWrapper dw     = new DialogWrapper(getFrame(), title, text, buttons);
        String        answer = dw.getResult();
    }
    public static void clickOK(String title, String text) {
        clickButton(title, text, "OK");
    }
}
