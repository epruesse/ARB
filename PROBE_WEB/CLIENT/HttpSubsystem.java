import java.io.*;
import java.net.*;
import java.util.jar.*;


public class HttpSubsystem
{
    // old version variables

private String host;
private int port;
private Socket probeSocket;
private String treeName;
private String cgiName;
private byte[] byteBuffer;
private PrintWriter toServer;
private InputStream fromServer;
private InputStreamReader charReader;
private char[] textInput;

    // new version variables
private URL hostUrl;
private String currentVersion;
private String localVersion;

public HttpSubsystem(String name)
    {
        // check URL  
      try{
            hostUrl = new URL(name);
            host = hostUrl.getHost();

            port = hostUrl.getPort();
            System.out.println("port: " + port);
            if ((port = hostUrl.getPort()) == -1) port = 80;

        }catch (Exception e)
            {
                System.out.println(e.getMessage());
                System.out.println("Program terminates, please repeat with correct URL");
                System.exit(1);
            }

        //        try{
        //        host = hostname;
        textInput = new char[4096];
        // check server available
        try
            {
                probeSocket = new Socket(host, port);
                probeSocket.close();
            }
        catch (Exception e)
            {
                System.out.println(e.getMessage());
                System.out.println("Cannot connect to server " + host + "\n Program terminates now!");
                System.exit(1);
            }


        // check tree version number
        // server version
        currentVersion = conductRequest("/pservercgi/getTreeVersion.cgi");
        System.out.println("server version: " + ">>>" + currentVersion + "<<<");
        //        String localVersion = new String("[CURRENTVERSION_16S_27081969]");


        // check local version number
        // access local jar file;  obsolete
        // change to local gzip file

        try
            {

                // adopt to local zipped version of the tree
                //         String archiveName = new String("allProbes.jar");
                String treeName = new String("probetree.gz");
                File treeFile = new File(treeName);
                if (!treeFile.exists())
                    {


                        ////

                        downloadZippedTree(treeName);

                        ////
                    }
            //            FileReader in = new FileReader(f);
//         JarFile archive = new JarFile(archiveName, false);
//         Manifest locMan = archive.getManifest();
//         System.out.println("Manifest: >>>" + locMan.getMainAttributes().getValue("LOCALVERSION") + "<<<");  

//           InputStreamReader in = new InputStreamReader(archive.getInputStream(archive.getEntry("demo.newick")));



//             char[] buffer = new char[4096];
//             int len;
//             while((len = in.read(buffer)) != -1)
//                 {
//                     String s = new String(buffer,0,len);
//                     inputTree.append(s);
//                 }

            }        
        catch (Exception e) {
            System.out.println("Couldn't find treefile or was not able to download");
            System.exit(1);
        }


                //                System.out.println(cl.treeString.substring(0,40));


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
        int charsRead;
        //             while((bytes_read = fromServer.read(byteBuffer)) != -1)
        while((charsRead = charReader.read(textInput)) != -1)
            {
                //                 System.out.write(byteBuffer, 0, bytes_read);
                PrintWriter testOutput = new PrintWriter(System.out);
                testOutput.write(textInput, 0, charsRead);
                strb.append(textInput, 0, charsRead);
                
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
     return   conductRequest("/pservercgi/getProbes.cgi?" + nodePath);

        //        NodeInformation goes here 

    }

    
public String getCurrentVersion()
    {
        return currentVersion;
    }

public String getLocalVersion()
    {
        return localVersion;
    }



public void downloadZippedTree(String fileName)
    {
        try{

            probeSocket = new Socket(host, port);
            fromServer = probeSocket.getInputStream();
            toServer = new PrintWriter(probeSocket.getOutputStream());
            FileOutputStream outstream = new FileOutputStream(fileName);
            byteBuffer = new byte[4096];
            toServer.print("GET /" + fileName + "\n\n");
            System.out.println("Request:\nGET /" + fileName + "\n\n");
            toServer.flush();
            int bytesRead;
            while((bytesRead = fromServer.read(byteBuffer)) != -1)
                {
                    outstream.write(byteBuffer, 0, bytesRead);
                }

            // maybe not necessary but seems safer
            fromServer.close();

            toServer.close();
            probeSocket.close();
            outstream.close();
            System.out.println("current version of tree file downloaded");
        }catch (Exception e)
            {
                System.out.println(e.getMessage());
                System.out.println("Program terminates now!");
                System.exit(1);
            }
        
        
    }
}
