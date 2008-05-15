//  ==================================================================== // 
//                                                                       // 
//    File      : ClientException.java                                   // 
//    Purpose   :                                                        // 
//    Time-stamp: <Thu Mar/04/2004 23:55 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//                                                                       // 
//  ==================================================================== // 


public class ClientException extends java.lang.Exception
{
    private String kind_of_error;
    private String message;
    private int    exitcode;

    public ClientException(String kind_of_error, String message, int exitcode) {
        this.kind_of_error = kind_of_error;
        this.message       = message;
        this.exitcode      = exitcode;
    }
    
    //     public String toString() { return message; }
    public String getMessage() {
        return kind_of_error+": "+message;
    }
    public String get_kind() { return kind_of_error; }
    public String get_plain_message() { return message; }
    public int get_exitcode() { return exitcode; }
}
