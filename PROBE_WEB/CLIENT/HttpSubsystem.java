import java.io.*;
import java.net.*;
import java.util.jar.*;


public class HttpSubsystem
{
    // old version variables

private String host;
private String path;
private int    port;

private Socket            probeSocket;
private String            treeName;
private String            cgiName;
private byte[] byteBuffer;
private PrintWriter       toServer;
private InputStream       fromServer;
private InputStreamReader charReader;
private char[] textInput;

    // new version variables
private URL    hostUrl;
private String neededClientVersion;
private String neededTreeVersion;

private String error;

public HttpSubsystem(String name)
    {
        // check URL
        try {
            hostUrl = new URL(name);
            host    = hostUrl.getHost();
            path    = hostUrl.getPath();
            // System.out.println("path: " + path);

            port = hostUrl.getPort();
            // System.out.println("port: " + port);

            if ((port = hostUrl.getPort()) == -1) port = 80;

            error = "";

        }
        catch (Exception e)
        {
            Toolkit.AbortWithError("Incorrect URL '"+name+"' ("+e.getMessage()+")");
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
            Toolkit.AbortWithServerProblem("Cannot connect to server '" + host + "'\n(reason: "+e.getMessage()+")");
        }

//         try
//         {

            // adopt to local zipped version of the tree
            //         String archiveName = new String("allProbes.jar");
//             String treeName = new String("probetree.gz");
//             File treeFile = new File(treeName);
//             if (!treeFile.exists())
//             {
//                 System.out.println("No tree stored local -> downloading from server ..");
//                 String error = downloadZippedTree(treeName);
//                 if (error != null) {
//                     Toolkit.AbortWithError(error);
//                 }
//             }
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

//         }
//         catch (Exception e) {
//             System.out.println("Couldn't find treefile or was not able to download");
//             System.exit(1);
//         }


        //                System.out.println(cl.treeString.substring(0,40));


    }

private String lastRequestError = null;

public String getLastRequestError()
    {
        return lastRequestError;
    }

public String conductRequest(String url_rest)
    {
        try{
            lastRequestError = null;
            probeSocket      = new Socket(host, port);
            fromServer       = probeSocket.getInputStream();
            charReader       = new InputStreamReader(fromServer);
            toServer         = new PrintWriter(probeSocket.getOutputStream());

            String request = "GET " + path + url_rest;

            toServer.print(request + "\n\n");
            System.out.println("Request: " + request + "\n");
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
        }
        catch (Exception e)
        {
            lastRequestError = e.getMessage();
            // System.err.println(e.getMessage());
            return null;
        }


    }

public String retrieveNodeInformation(String nodePath)
    {
        String probes = conductRequest("getProbes.cgi?path=" + nodePath + "&plength=all");
        if (probes == null) {
            System.out.println("Warning: no answer from Server ("+getLastRequestError()+")");
        }
        return probes;
    }

public void retrieveVersionInformation()
    {
        String versionInfo = conductRequest("getVersion.cgi");
        if (versionInfo == null) {
            Toolkit.AbortWithServerProblem("cannot get version info ("+getLastRequestError()+")");
        }
        ServerAnswer parsedVersionInfo = new ServerAnswer(versionInfo);

        neededClientVersion = parsedVersionInfo.getValue("client_version");
        neededTreeVersion   = parsedVersionInfo.getValue("tree_version");

        if (neededClientVersion == null) Toolkit.AbortWithServerProblem("no client version info ("+parsedVersionInfo.getError()+")");
        if (neededTreeVersion == null) Toolkit.AbortWithServerProblem("no tree version info  ("+parsedVersionInfo.getError()+")");
    }

public String getNeededClientVersion()
    {
        return neededClientVersion;
    }

public String getNeededTreeVersion()
    {
        return neededTreeVersion;
    }



public String downloadZippedTree(String fileName)
    {
        try {
            probeSocket = new Socket(host, port);
            fromServer = probeSocket.getInputStream();
            toServer = new PrintWriter(probeSocket.getOutputStream());
            FileOutputStream outstream = new FileOutputStream(fileName);
            byteBuffer = new byte[4096];
            toServer.print("GET " + path + "getTree.cgi\n\n");
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
            System.out.println("Saving tree as "+fileName);
        }
        catch (Exception e)
        {
            return e.getMessage();
        }

        return null;
    }
}
