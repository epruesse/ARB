//  ==================================================================== //
//                                                                       //
//    File      : Toolkit.java                                           //
//    Purpose   : Functions uses in all classes go here                  //
//    Time-stamp: <Thu Mar/04/2004 23:59 MET Coder@ReallySoft.de>        //
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
    public static String clientVersion = "0.9alpha"; // CLIENT_SERVER_VERSION -- this has to match the version in ../SERVER/getVersion.cgi

    private static String maintainer = "probeadmin@arb-home.de";
    
//     private static Client theClient = null;
//     public static void registerClient(Client c) { theClient = c; }
//     public static void unregisterClient() { theClient = null; }


    private static void AbortWithError_Internal(String kind_of_error, String error_message, int exitcode) throws Exception {
//         if (theClient == null) {
        System.out.println(kind_of_error+": "+error_message);
        throw new ClientException(kind_of_error, error_message, exitcode);        
        //             if (exitcode != 0) {
//                 System.exit(exitcode);
//             }
//         }
//         else {
//             MessageDialog popup = new MessageDialog(theClient.getDisplay(), kind_of_error, error_message);
//             while (!popup.okClicked()) ; // @@@ do non-busy wait here (how?)
//             System.exit(exitcode);
//         }
    }

    public static void AbortWithError(String error) throws Exception
    {
        AbortWithError_Internal("Error in "+clientName, error, 1);
        //         if (theClient == null) {
//             System.out.println("Error in "+clientName+": "+error);
//         }
//         else {
//             MessageDialog popup = new MessageDialog(theClient.getDisplay(), error);
//         }
//         System.exit(1);         // error
    }

    public static void AbortWithConnectionProblem(String error) throws Exception
    {
        AbortWithError_Internal(clientName+" has a connection problem:",
                                error+"\nPlease check whether your internet connection works", 2);
//         System.out.println(clientName+" has a connection problem: "+error);
//         System.out.println("Please check whether your internet connection works.");
//         System.exit(2); // server problem
    }

    public static void AbortWithServerProblem(String error) throws Exception
    {
        AbortWithError_Internal(clientName+" got error from server:",
                                error+"\nPlease report to "+maintainer, 2);
//         System.out.println(clientName+" got error from server:\n    "+error);
//         System.out.println("Please report to "+maintainer);
//         System.exit(2);         // server problem
    }

    public static void InternalError(String error) throws Exception
    {
        AbortWithError_Internal("Internal error in "+clientName+" v"+clientVersion,
                                error+"\n(this seems to be a bug, please report to "+maintainer+")", 666);
//         System.out.println("Internal error in "+clientName+" v"+clientVersion+": "+error);
//         System.out.println("(this seems to be a bug, please report to "+maintainer+")");
//         System.exit(666);       // internal error
    }

    public static void ExpectNoError(String error) throws Exception
    {
        if (error != null) {
            InternalError("Unexpected error: "+error);
        }
    }
}
