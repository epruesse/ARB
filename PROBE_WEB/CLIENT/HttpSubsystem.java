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
            port    = hostUrl.getPort();

            if ((port = hostUrl.getPort()) == -1) port = 80;
        }
        catch (Exception e) {
            Toolkit.AbortWithError("Incorrect URL '"+name+"' ("+e.getMessage()+")");
        }

        // check whether server is available
        try {
            System.out.println("Connecting "+host+" ...");
            probeSocket = new Socket(host, port);
            probeSocket.close();
        }
        catch (Exception e) {
            Toolkit.AbortWithServerProblem("Cannot connect to server '"+host+"'");
        }

        // init other members
        textInput = new char[4096];
        error     = "";
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

            int charsRead;
            while((charsRead = charReader.read(textInput)) != -1) {
                PrintWriter testOutput = new PrintWriter(System.out);
                testOutput.write(textInput, 0, charsRead);
                strb.append(textInput, 0, charsRead);

            }

            // maybe not necessary but seems safer
            fromServer.close();
            charReader.close();
            toServer.close();
            probeSocket.close();

            return strb.toString();
        }
        catch (Exception e)
        {
            lastRequestError = e.getMessage();
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
        ServerAnswer parsedVersionInfo = new ServerAnswer(versionInfo, true, true);

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

            fromServer.close(); // maybe not necessary but seems safer
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
