//  ==================================================================== //
//                                                                       //
//    File      : Toolkit.java                                           //
//    Purpose   : Functions uses in all classes go here                  //
//    Time-stamp: <Thu Mar/18/2004 20:52 MET Coder@ReallySoft.de>        //
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
    public static String clientName = "ARB probe library";

    // search globally for 'CLIENT_SERVER_VERSIONS' (other occurance is in ../SERVER/getVersion.cgi)
    public static String client_version    = "1.2"; // if client_version does not match, a notice is printed
    public static String interface_version = "1.1"; // if interface_version does not match, client terminates!

    private static String maintainer = "probeadmin@arb-home.de";

    private static Client theClient = null;
    public static void registerClient(Client c) { theClient = c; }
    public static void unregisterClient() { theClient = null; }

    public static void showError(String error) {
        try {
            System.out.println(error);
            if (theClient != null) theClient.show_error(error);
        }
        catch (Exception e) {
            System.out.println("Exception caught in showError(): ");
            e.printStackTrace();
            System.out.println("Message: "+e.getMessage());
        }
    }
    public static void showMessage(String message) {
        try {
            System.out.println(message);
            if (theClient != null) theClient.show_message(message);
        }
        catch (Exception e) {
            System.out.println("Exception caught in showMessage(): ");
            e.printStackTrace();
            System.out.println("Message: "+e.getMessage());
        }
    }

    private static void AbortWithError_Internal(String kind_of_error, String error_message, int exitcode) throws Exception {
        System.out.println(kind_of_error+": "+error_message);
        throw new ClientException(kind_of_error, error_message, exitcode);
    }

    public static void AbortWithError(String error) throws Exception {
        AbortWithError_Internal("Error in "+clientName, error, 1);
    }

    public static void AbortWithConnectionProblem(String error) throws Exception {
        AbortWithError_Internal(clientName+" has a connection problem",
                                error+"\nPlease check whether your internet connection works", 2);
    }

    public static void AbortWithServerProblem(String error) throws Exception {
        AbortWithError_Internal(clientName+" got error from server",
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

    // -------------------------------------------------------------------
    // NOTE: if you change encodePath()/decodePath() please keep encodePath/decodePath()
    //       in ./PROBE_SERVER/WORKER/psw_main.cxx up-to-date

    private static final char[] hexToken = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    private static final int bitmask[] = { 8, 4, 2, 1 };

    public static String encodePath(String path) {
        StringBuffer coded      = new StringBuffer();
        int          pathLength = path.length();

        // transform length into two byte hex format
        for (int digit = 0; digit < 4; digit ++) {
            int remain = pathLength%16;
            pathLength = pathLength/16;
            coded.insert(0,hexToken[remain]);
        }

        int value = 0;
        int position;
        for (position = 0; position < path.length(); position++ ) {
            if (path.charAt(position) == '1') {
                value += bitmask[position%4];

//                 switch (position%4) {
//                     case 0: value += 8; break;
//                     case 1: value += 4; break;
//                     case 2: value += 2; break;
//                     case 3: value += 1; break;
//                     default:
//                         Toolkit.InternalError("logical error in encodePath");
//                 }
            }
            if ((position%4) == 3) {
                coded.append(hexToken[value]);
                value = 0;
            }
        }

        if ((position%4) != 0) {
            coded.append(hexToken[value]);
        }
        return coded.toString();
    }

    public static int deHex(char c) throws Exception {
        if (c >= '0' && c <= '9') return (int)(c-'0');
        if (c >= 'A' && c <= 'F') return (int)(c-'A')+10;
        if (c >= 'a' && c <= 'f') return (int)(c-'a')+10;
        throw new ClientException("hex decode error", "Cannot decode '"+c+"'", 1);
    }

    public static String decodePath(String coded) throws Exception {
        int pathLength = 0;
        for (int digit = 0; digit < 4; ++digit) {
            pathLength = pathLength*16 + deHex(coded.charAt(digit));
        }

        StringBuffer buf = new StringBuffer(pathLength);

        int coded_position = 4;
        int coded_bits     = 0;
        int coded_value    = -1;

        for (int position = 0; position<pathLength; ++position) {
            if (coded_bits == 0) {
                coded_bits  = 4;
                coded_value = deHex(coded.charAt(coded_position++));
            }

            buf.append((coded_value & bitmask[4-coded_bits]) == 0 ? '0' : '1');
            coded_bits--;
        }

        return buf.toString();
    }

    // Bitstring (e.g. "000101100") compression

    public static String compressString(String s) {
        return encodePath(s);
    }
    public static String uncompressString(String s) throws Exception {
        return decodePath(s);
    }
}
