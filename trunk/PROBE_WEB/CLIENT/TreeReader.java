import java.io.*;
import java.util.*;

public class TreeReader
{

private String tree;

public TreeReader()
    {

        StringBuffer inputTree = new StringBuffer();
        try {
            //        File f =  new File("tree2.ntree");
            File f =  new File("demo.newick");
            FileReader in = new FileReader(f);
            char[] buffer = new char[4096];
            int len;
            while((len = in.read(buffer)) != -1)
                {
                    String s = new String(buffer,0,len);
                    inputTree.append(s);
                }

        }
        catch (Exception e) {
            System.out.println("Couldn't open treefile");
            System.exit(1);
        }

        tree = inputTree.toString().substring(inputTree.toString().indexOf('('));
 
    }

public String getTreeString()
    {
        return tree;
    }

}
