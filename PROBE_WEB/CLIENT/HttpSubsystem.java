import java.io.*;
import java.net.*;



public class HttpSubsystem
{
private String host;
private int port = 80;
private Socket probeSocket;
private String treeName;
private String cgiName;
private byte[] byteBuffer;
private PrintWriter toServer;
private InputStream fromServer;
private InputStreamReader charReader;
private char[] textInput;



public HttpSubsystem(String hostname)
    {

        //        try{
        host = hostname;
        textInput = new char[4096];
    }

 public String conductRequest(String filename)
     {

        try{

        probeSocket = new Socket(host, port);
        fromServer = probeSocket.getInputStream();
        charReader = new InputStreamReader(fromServer);
        toServer = new PrintWriter(probeSocket.getOutputStream());

        //        byteBuffer = new byte[4096];
        //        textInput = new char[4096];
        toServer.print("GET " + filename + "\n\n");
        System.out.println("Request:\nGET " + filename + "\n\n");
        toServer.flush();
        StringBuffer strb = new StringBuffer();
        //             int bytes_read;
        int chars_read;
        //             while((bytes_read = fromServer.read(byteBuffer)) != -1)
        while((chars_read = charReader.read(textInput)) != -1)
            {
                //                 System.out.write(byteBuffer, 0, bytes_read);
                PrintWriter testOutput = new PrintWriter(System.out);
                testOutput.write(textInput, 0, chars_read);
                strb.append(textInput, 0, chars_read);
                
            }

        // maybe not necessary but seems safer
        fromServer.close();
        charReader.close();
        toServer.close();
        probeSocket.close();

        //probeSocket.close();
        //            System.out.close();
        return strb.toString();
        
        }catch (Exception e)
            {
                System.err.println(e);
                return new String("no successful retrieve");
            }
        
        
     }
public String retrieveNodeInformation(String nodePath)
    {
     return   conductRequest("/cgi/getProbes.cgi?" + nodePath);

        //        NodeInformation goes here 

    }


}
