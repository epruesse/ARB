import java.io.*;
import java.net.*;
import java.util.jar.*;


public class HttpSubsystem
{
//     // old version variables

//     private String host;
//     private String path;
//     private int    port;

//     private Socket probeSocket;
//     private String treeName;
//     private String cgiName;
    
//     private byte[] byteBuffer;
    
//     private PrintWriter       toServer;
//     private InputStream       fromServer;
//     private InputStreamReader charReader;
    
//     private char[] textInput;

    // new version variables
    private URL    baseUrl;
    private String neededInterfaceVersion;
    private String availableClientVersion;
    private String neededTreeVersion;

    private String error;

    public HttpSubsystem(String name) throws Exception
    {
        // check URL
        try {
            baseUrl = new URL(name);
            // host    = hostUrl.getHost();
            // path    = hostUrl.getPath();
            // port    = hostUrl.getPort();

            // if ((port = hostUrl.getPort()) == -1) port = 80;
        }
        catch (Exception e) {
            Toolkit.AbortWithError("Incorrect URL '"+name+"' ("+e.getMessage()+")");
        }

        // check whether server is available
        try {
            // System.out.println("Connecting "+host+" ...");
            // probeSocket = new Socket(host, port);
            // probeSocket.close();
        }
        catch (Exception e) {
            Toolkit.AbortWithConnectionProblem("Cannot connect to server '"+
                                               baseUrl.getHost()+"' ("+e.getMessage()+")");
        }

        // init other members
        // textInput = new char[4096];
        error     = "";
    }

    private String lastRequestError = null;

    public String getLastRequestError() {
        return lastRequestError;
    }

    public String conductRequest(String relativePath) {
        lastRequestError = null;
        try {
            URL url = new URL(baseUrl, relativePath);

            System.out.println("Generating request to '"+relativePath+"'");
            HttpURLConnection request = (HttpURLConnection)url.openConnection();

            if (request.usingProxy()) {
                System.out.println("detected proxy");
            }
            System.out.println("connecting request");
            try {
                request.connect();
            }
            catch (IOException e) {
                throw new Exception("can't connect "+e.getMessage());
                // Note : e.getMessage only contains the servername
            }

            int response_code = request.getResponseCode();
            if (response_code != 200) {
                String response = request.getResponseMessage();
                throw new Exception("response: "+response_code+" '"+response+"'");
            }

            String content_type = request.getContentType();
            if (!content_type.startsWith("text/plain")) {
                throw new Exception("answer has wrong content type: '"+content_type+"'");
            }

            int content_len = request.getContentLength();
            System.out.println("content_len='"+content_len+"'");

            byte buffer[] = new byte[content_len];
             // StringBuffer strb = new StringBuffer(content_len);

            DataInputStream dis = new DataInputStream(request.getInputStream());
            // InputStream is  = request.getInputStream();

            dis.readFully(buffer);

            return new String(buffer);
        }
        catch (Exception e) {
            lastRequestError = "requested: '"+relativePath+"': "+e.getMessage();
            return null;

        }
    }
//     public String conductRequest(String url_rest)
//     {
//         try{
//             lastRequestError = null;
//             probeSocket      = new Socket(host, port);
//             fromServer       = probeSocket.getInputStream();
//             charReader       = new InputStreamReader(fromServer);
//             toServer         = new PrintWriter(probeSocket.getOutputStream());

//             String request = "GET " + path + url_rest;

//             toServer.print(request + "\n");
//             System.out.println("Request: '" + request + "'");
//             toServer.flush();

//             StringBuffer strb = new StringBuffer();

//             int charsRead;
//             while((charsRead = charReader.read(textInput)) != -1) {
//                 PrintWriter testOutput = new PrintWriter(System.out);
//                 testOutput.write(textInput, 0, charsRead);
//                 strb.append(textInput, 0, charsRead);

//             }

//             // maybe not necessary but seems safer
//             fromServer.close();
//             charReader.close();
//             toServer.close();
//             probeSocket.close();

//             return strb.toString();
//         }
//         catch (Exception e)
//         {
//             lastRequestError = e.getMessage();
//             return null;
//         }
//     }

    public String retrieveNodeInformation(String nodePath, boolean onlyExact)
    {
        // String probes = conductRequest("getProbes.cgi?path=" + nodePath + "&plength=all&exact="+(onlyExact ? "1" : "0"));
        String probes = conductRequest("getProbes.cgi?path=" + nodePath + "&plength=all&exact="+(onlyExact ? "1" : "0"));
        if (probes == null) {
            System.out.println("Warning: no answer from Server ("+getLastRequestError()+")");
        }
        return probes;
    }

    public String retrieveGroupMembers(String groupId, int plength)
    {
        String members = conductRequest("getMembers.cgi?id=" + groupId + "&plength=" + plength);
        if (members == null) {
            System.out.println("Warning: no answer from Server ("+getLastRequestError()+")");
        }
        return members;
    }

    public void retrieveVersionInformation() throws Exception
    {
        String versionInfo = conductRequest("getVersion.cgi");
        if (versionInfo == null) {
            Toolkit.AbortWithConnectionProblem("cannot get version info ("+getLastRequestError()+")");
        }
        ServerAnswer parsedVersionInfo = new ServerAnswer(versionInfo, true, false);
        if (parsedVersionInfo.hasError()) {
            Toolkit.AbortWithServerProblem(parsedVersionInfo.getError());
        }

        availableClientVersion = parsedVersionInfo.getValue("client_version");
        neededInterfaceVersion = parsedVersionInfo.getValue("interface_version");
        neededTreeVersion      = parsedVersionInfo.getValue("tree_version");

        String missing                                   = null;
        if (availableClientVersion == null) missing      = "client";
        else if (neededInterfaceVersion == null) missing = "interface";
        else if (neededTreeVersion == null) missing      = "tree";

        if (missing != null) {
            Toolkit.AbortWithServerProblem("no "+missing+" version info ("+parsedVersionInfo.getError()+")");
        }
    }

    public String getAvailableClientVersion() { return availableClientVersion; }
    public String getNeededInterfaceVersion() { return neededInterfaceVersion; }
    public String getNeededTreeVersion() { return neededTreeVersion; }

    public String downloadZippedTree(String fileName) {
        String error        = null;
        String relativePath = "getTree.cgi";

        try {
            URL url = new URL(baseUrl, relativePath);
            System.out.println("Generating request to '"+relativePath+"'");
            HttpURLConnection request = (HttpURLConnection)url.openConnection();

            if (request.usingProxy()) {
                System.out.println("detected proxy");
            }
            
            int response_code = request.getResponseCode();
            if (response_code != 200) {
                String response = request.getResponseMessage();
                throw new Exception("response: "+response_code+" '"+response+"'");
            }

            String content_type = request.getContentType();
            if (!content_type.startsWith("text/gzipped")) {
                throw new Exception("answer has wrong content type: '"+content_type+"'");
            }

            int content_len = request.getContentLength();
            System.out.println("content_len='"+content_len+"'");

            System.out.println("connecting request");
            try {
                request.connect();
            }
            catch (IOException e) {
                throw new Exception("can't connect "+e.getMessage());
                // Note : e.getMessage only contains the servername
            }

            byte buffer[]        = new byte[content_len];
            // StringBuffer strb = new StringBuffer(content_len);

            DataInputStream dis = new DataInputStream(request.getInputStream());
            // InputStream is  = request.getInputStream();

            dis.readFully(buffer);

            Toolkit.showMessage("Saving tree as "+fileName);
            FileOutputStream outstream = new FileOutputStream(fileName);
            outstream.write(buffer, 0, content_len);
            outstream.close();
        }
        catch (Exception e) {
            error = "requested: '"+relativePath+"': "+e.getMessage();
        }

        return error;

    }
//     public String downloadZippedTree(String fileName)
//     {
//         try {
//             probeSocket = new Socket(host, port);
//             fromServer = probeSocket.getInputStream();
//             toServer = new PrintWriter(probeSocket.getOutputStream());
//             FileOutputStream outstream = new FileOutputStream(fileName);
//             byteBuffer = new byte[4096];

//             String request = "GET " + path + "getTree.cgi";

//             toServer.print(request + "\n");
//             System.out.println("Request: '" + request + "'");
//             toServer.flush();

//             int bytesRead;
//             while((bytesRead = fromServer.read(byteBuffer)) != -1) {
//                 outstream.write(byteBuffer, 0, bytesRead);
//             }

//             fromServer.close(); // maybe not necessary but seems safer
//             toServer.close();
//             probeSocket.close();
//             outstream.close();
//             System.out.println("Saving tree as "+fileName);
//         }
//         catch (Exception e)
//         {
//             return e.getMessage();
//         }

//         return null;
//     }
}
