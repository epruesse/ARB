import java.io.*;
import java.net.*;
import java.security.*;
import java.util.jar.*;


public class HttpSubsystem {
    
    private URL baseUrl;

    private String neededInterfaceVersion;
    private String availableClientVersion;
    private String neededTreeVersion;

    private String lastRequestError = null;

    public HttpSubsystem(String base_url, String proxy_def) throws Exception {
        // check URL
        try {
            baseUrl = new URL(base_url);
        }
        catch (Exception e) {
            Toolkit.AbortWithError("Incorrect URL '"+base_url+"' ("+e.getMessage()+")");
        }
    }

    public String getLastRequestError() {
        return lastRequestError;
    }

    private void showUrlInfo(String where, URL url) {        
        System.out.println("Checking URL ["+where+"]:");
        System.out.println("  url.getHost()='"+url.getHost()+"'");
        System.out.println("  url.getPort()='"+url.getPort()+"'");
        // System.out.println("  url.getDefaultPort()='"+url.getDefaultPort()+"'");
        System.out.println("  url.getPath()='"+url.getPath()+"'");
        System.out.println("  url.getQuery()='"+url.getQuery()+"'");
        System.out.println("  url.getFile()='"+url.getFile()+"'");
        System.out.println("  url.getProtocol()='"+url.getProtocol()+"'");
        System.out.println("  url.getRef()='"+url.getRef()+"'");
    }

    private byte[] conductRequest_internal(String relativePath, String expected_content_type) {
        lastRequestError = null;
        try {
            URL url = new URL(baseUrl, relativePath);

            // showUrlInfo("creation", url);

            try {
                System.out.println("Generating request to '"+relativePath+"'");
                HttpURLConnection request = (HttpURLConnection)url.openConnection();

                {
                    Permission perm = request.getPermission();
                    if (perm != null) {
                        System.out.println("Found permission[1]:");
                        System.out.println(" perm.toString()='"+perm.toString()+"'");
                        System.out.println(" perm.getName()='"+perm.getName()+"'");
                    }
                }

                System.out.println("connecting request");
                try {
                    request.connect();
                }
                catch (IOException e) {
                    throw new Exception("can't connect "+e.getMessage());
                    // Note : e.getMessage only contains the servername
                }

                showUrlInfo("after connect", url);                

                if (request.usingProxy()) {
                    System.out.println("* proxy is used");
                }

                {
                    Permission perm = request.getPermission();
                    if (perm != null) {
                        System.out.println("Found permission[2]:");
                        System.out.println(" perm.toString()='"+perm.toString()+"'");
                        System.out.println(" perm.getName()='"+perm.getName()+"'");
                    }
                }

                int response_code = request.getResponseCode();
                if (response_code != 200) {
                    String response = request.getResponseMessage();
                    throw new Exception("response: "+response_code+" '"+response+"'");
                }

                String content_type = request.getContentType();
                if (!content_type.startsWith(expected_content_type)) {
                    throw new Exception("answer has wrong content type: '"+content_type+"' (expected: "+expected_content_type+")");
                }

                int content_len = request.getContentLength();
                System.out.println("content_len='"+content_len+"'");

                byte buffer[] = new byte[content_len];
                DataInputStream dis = new DataInputStream(request.getInputStream());

                dis.readFully(buffer);

                showUrlInfo("final", url);
                return buffer;
            }
            catch (Exception e) {
                lastRequestError = "requested: '"+relativePath+"' (from: '"+url.getFile()+"'): "+e.getMessage();
                showUrlInfo("failure", url);
                return null;
            }
        }
        catch (Exception e) {
            lastRequestError = "requested: '"+relativePath+"' (from: '"+baseUrl.getHost()+"'): "+e.getMessage();
            return null;
        }
    }

    public String conductRequest(String relativePath) {
        byte[] response  = conductRequest_internal(relativePath, "text/plain");
        return response == null ? null : new String(response);
    }

    public String downloadZippedTree(String fileName) {
        byte[] response = conductRequest_internal("getTree.cgi", "text/gzipped");
        if (response != null) {
            try {
                Toolkit.showMessage("Saving tree as "+fileName);
                FileOutputStream outstream = new FileOutputStream(fileName);
                outstream.write(response, 0, response.length);
                outstream.close();
            }
            catch (Exception e) {
                lastRequestError = "Can't save tree: "+e.getMessage();
            }
        }
        return lastRequestError;
    }

    public String retrieveNodeInformation(String nodePath, boolean onlyExact)
    {
        String probes = conductRequest("getProbes.cgi?path=" + nodePath + "&plength=all&exact="+(onlyExact ? "1" : "0"));
        if (probes == null) {
            System.out.println("Warning: no answer from Server: "+getLastRequestError());
        }
        return probes;
    }

    public String retrieveGroupMembers(String groupId, int plength)
    {
        String members = conductRequest("getMembers.cgi?id=" + groupId + "&plength=" + plength);
        if (members == null) {
            System.out.println("Warning: no answer from Server: "+getLastRequestError());
        }
        return members;
    }

    public void retrieveVersionInformation() throws Exception
    {
        String versionInfo = conductRequest("getVersion.cgi");
        if (versionInfo == null) {
            Toolkit.AbortWithConnectionProblem("cannot get version info: "+getLastRequestError());
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
            Toolkit.AbortWithServerProblem("no "+missing+" version info: "+parsedVersionInfo.getError());
        }
    }

    public String getAvailableClientVersion() { return availableClientVersion; }
    public String getNeededInterfaceVersion() { return neededInterfaceVersion; }
    public String getNeededTreeVersion() { return neededTreeVersion; }

}
