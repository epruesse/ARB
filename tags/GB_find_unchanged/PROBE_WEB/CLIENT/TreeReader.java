import java.io.*;
import java.util.*;
import java.util.zip.*;

public class TreeReader
{

private String tree;
private String version;
private String error;

public TreeReader(String treefile) throws Exception 
    {
        error                  = null;
        StringBuffer inputTree = new StringBuffer();
        try {

            //            File f = new File("demo.newick");

            File f = new File(treefile);
            if (f == null)  {
                error = "File not found ("+treefile+")";
            }
            else {

                //  String archiveName = new String("allProbes.jar");
                //            FileReader in = new FileReader(f);
                InputStreamReader in = new InputStreamReader(new GZIPInputStream( new FileInputStream(f)));
                //JarFile archive = new JarFile(archiveName, false);
                //InputStreamReader in = new InputStreamReader(archive.getInputStream(archive.getEntry("demo.newick")));



                char[] buffer = new char[4096];
                int len;
                while((len = in.read(buffer)) != -1)
                {
                    String s = new String(buffer,0,len);
                    inputTree.append(s);
                }
                in.close();
            }
        }
        catch (Exception e) {
            error = "Could not read treefile: "+e.getMessage();
        }

        if (error == null) {
            String treeString = inputTree.toString();

            if (treeString == "") {
                error = "empty file";
            }
            else {
                int bracket_open  = treeString.indexOf('[');
                int bracket_close = treeString.indexOf(']');

                if (bracket_open == -1 || bracket_close == -1) {
                    error = "no header";
                }
                else {
                    String header = treeString.substring(bracket_open+1, bracket_close);
                    // System.out.println("header='"+header+"'");

                    ServerAnswer parsed_header = new ServerAnswer(header, false, false);

                    if (parsed_header.hasError()) {
                        error = parsed_header.getError();
                    }
                    else if (!parsed_header.hasKey("version")) {
                        error = "'version' expected";
                    }
                    else {
                        version = parsed_header.getValue("version");
                        // System.out.println("version='"+version+"'");

                        int paren = treeString.indexOf('(', bracket_close+1);
                        if (paren == -1) {
                            error = "has no tree";
                        }
                        else {
                            tree = treeString.substring(paren);
                        }
                    }
                }
            }

            if (error != null) {
                error = "Treefile '"+treefile+"' is corrupt ("+error+")";
            }
        }
    }

public String getError()
    {
        return error;
    }

public String getTreeString() throws Exception 
    {
        Toolkit.ExpectNoError(error);
        return tree;
    }


public String getVersionString() throws Exception 
    {
        Toolkit.ExpectNoError(error);
        return version;
    }

}
