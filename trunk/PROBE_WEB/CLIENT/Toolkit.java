//  ==================================================================== //
//                                                                       //
//    File      : Toolkit.java                                           //
//    Purpose   : Functions uses in all classes go here                  //
//    Time-stamp: <Mon Oct/13/2003 01:02 MET Coder@ReallySoft.de>        //
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

public static void AbortWithError(String error)
    {
        System.out.println("Error in "+clientName+": "+error);
        System.exit(1); // error
    }

public static void AbortWithConnectionProblem(String error)
    {
        System.out.println(clientName+" has a connection problem: "+error);
        System.out.println("Please check whether your internet connection works.");
        System.exit(2); // server problem
    }

public static void AbortWithServerProblem(String error)
    {
        System.out.println(clientName+" got error from server:\n    "+error);
        System.out.println("Please report to "+maintainer);
        System.exit(2); // server problem
    }

public static void InternalError(String error)
    {
        System.out.println("Internal error in "+clientName+" v"+clientVersion+": "+error);
        System.out.println("(this seems to be a bug, please report to "+maintainer+")");
        System.exit(666); // internal error
    }

public static void ExpectNoError(String error)
    {
        if (error != null) {
            InternalError("Unexpected error: "+error);
        }
    }
}
