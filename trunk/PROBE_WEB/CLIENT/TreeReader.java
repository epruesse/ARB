import java.io.*;
import java.util.*;
import java.util.zip.*;

public class TreeReader
{

private String tree;
private String version;


public TreeReader(String treefile)
    {

        StringBuffer inputTree = new StringBuffer();
        try {

            //            File f =  new File("demo.newick");
            File f =  new File("probetree.gz");

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
        catch (Exception e) {
            System.out.println("Couldn't open treefile");
            System.exit(1);
        }

        tree = inputTree.toString().substring(inputTree.toString().indexOf('('));
        version = inputTree.toString().substring(0, inputTree.toString().indexOf('\n'));
    }

public String getTreeString()
    {
        return tree;
    }


public String getVersionString()
    {
        return version;
    }

}
