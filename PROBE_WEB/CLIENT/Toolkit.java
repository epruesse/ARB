//  ==================================================================== //
//                                                                       //
//    File      : Toolkit.java                                           //
//    Purpose   : Functions uses in all classes go here                  //
//    Time-stamp: <Wed Mar/10/2004 13:05 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

class Toolkit 
{
    public static String clientName    = "arb_probe_client";
    public static String clientVersion = "1.0"; // CLIENT_SERVER_VERSION -- this has to match the version in ../SERVER/getVersion.cgi

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
        AbortWithError_Internal("Internal error in "+clientName+" v"+clientVersion,
                                error+"\n(this seems to be a bug, please report to "+maintainer+")", 666);
    }

    public static void ExpectNoError(String error) throws Exception
    {
        if (error != null) {
            InternalError("Unexpected error: "+error);
        }
    }
}
