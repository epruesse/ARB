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
        System.out.println("  url.getPath()='"+url.getPath()+"'");
        System.out.println("  url.getQuery()='"+url.getQuery()+"'");
        System.out.println("  url.getFile()='"+url.getFile()+"'");
        System.out.println("  url.getProtocol()='"+url.getProtocol()+"'");
        System.out.println("  url.getRef()='"+url.getRef()+"'");
    }

    private static int streamAll(InputStream in, OutputStream out) throws Exception {
        final int bufsize       = 64*1024;
        byte[]    buffer        = new byte[bufsize];
        int       streamedBytes = 0;

        try {
            int readBytes = in.read(buffer);
            System.out.println("  readBytes="+readBytes+" (first)");
            while (readBytes != -1) { // got some bytes                
                out.write(buffer, 0, readBytes);
                streamedBytes += readBytes;
                readBytes      = in.read(buffer);
                System.out.println("  readBytes="+readBytes);
            }
        }
        catch (IOException e) {
            Toolkit.AbortWithError("while streaming data: "+e.getMessage());
        }

        System.out.println("streamedBytes="+streamedBytes);
        return streamedBytes;
    }

    private int conductRequest_internal(String relativePath, String expected_content_type, OutputStream out) {
        // returns number of bytes read

        lastRequestError = null;
        try {
            URL url = new URL(baseUrl, relativePath);

            try {
                HttpURLConnection request = (HttpURLConnection)url.openConnection();

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
                if (!content_type.startsWith(expected_content_type)) {
                    throw new Exception("answer has wrong content type: '"+content_type+"' (expected: "+expected_content_type+")");
                }

                int content_len = request.getContentLength();
                int bytes_read  = streamAll(request.getInputStream(), out);                
                if (bytes_read != content_len && content_len != -1) {
                    System.out.println("unexpected length: bytes_read="+bytes_read+" content_len="+content_len);
                }
                return bytes_read;
            }
            catch (Exception e) {
                lastRequestError = "requested: '"+relativePath+"' (from: '"+url.getHost()+"'): "+e.getMessage();
                showUrlInfo("failure", url);
                return 0;
            }
        }
        catch (Exception e) {
            lastRequestError = "requested: '"+relativePath+"' (from: '"+baseUrl.getHost()+"'): "+e.getMessage();
            return 0;
        }
    }

    public String conductRequest(String relativePath) {
        ByteArrayOutputStream str     = new ByteArrayOutputStream(10000);
        int                   written = conductRequest_internal(relativePath, "text/plain", str);

        if (written <= 0 && lastRequestError == null) {
            lastRequestError = "written="+written+" (illegal value)";
        }

        return lastRequestError == null ? str.toString() : null;
    }

    public String downloadZippedTree(String fileName) {
        try {
            Toolkit.showMessage("  to file "+fileName);
            FileOutputStream outstream     = new FileOutputStream(fileName);
            int              streamedBytes = conductRequest_internal("getTreeBinary.cgi", "application/octet-stream", outstream);

            outstream.close();
            Toolkit.showMessage("Tree has been saved ("+(streamedBytes/1024)+"k)");
        }
        catch (Exception e) {
            lastRequestError = "Can't save tree: "+e.getMessage();
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
