import java.util.*;
import java.awt.*;
import java.awt.event.*;


/**
 * central driver and control class for ARB probe service client
 */

class Client
{

private  ProbesGUI display;
private TreeDisplay tree;
private TreeNode root;
private String treeString;
private String hostname;
private HttpSubsystem webAccess;


    // private TreeNode subtree;

    // private Communicationsubsystem


public static void main(String[] args)

    {
        // create central client object(s)
        // create communication structure
        // initialization of communication
        // control of user interaction

        // System.out.println(args.length);

        Client cl = new Client();

        if (args.length == 0) cl.hostname = new String("http://localhost");
        cl.webAccess = new HttpSubsystem(cl.hostname);
        
        // enables users to store tree local
        // update only when tree is changed

        // here goes the web access code

        // tree to display
        TreeReader tr = new TreeReader("probetree.gz");
   

        //        cl.webAccess.conductRequest("/demo.newick");
        // inlude version number in tree
        String currentVersion = cl.webAccess.conductRequest("/cgi/getTreeVersion.cgi");
        System.out.println("server version: " + ">>>" + currentVersion + "<<<");
        //        String localVersion = new String("[CURRENTVERSION_16S_27081969]");
        // access local jar file;


                //                System.out.println(cl.treeString.substring(0,40));
        System.out.println("local version: " + ">>>" + tr.getVersionString() + "<<<"); 
       if (!currentVersion.equals(tr.getVersionString()))
            {


                System.out.println("TreeVersion don't match the current version");
                System.out.println("Downloading actual version\nPlease start again");

                //                System.out.println("Downloading current tree version from server");
                //                cl.treeString = cl.webAccess.conductRequest("/demo.newick");

                cl.webAccess.downloadZippedTree("probetree.gz");
                System.exit(15);
            }
       else
           {
               cl.treeString = tr.getTreeString();
           };


        // parsing


        cl.root = (new TreeParser(cl.treeString)).getRootNode();

        if (cl.root == null) 
            {
                System.out.println("in Client(): no valid node given to display");
                System.exit(1);
            }

        cl.root.setPath("");
        cl.display = new ProbesGUI(cl.root, 10);
        //        ProbesGUIActionListener al = new ProbesGUIActionListener(cl.display);
        //        cl.display.setMenuBar(new ProbeMenu(al));
        // 
        // obtain reference to Treedisplay first !
        cl.display.getTreeDisplay().setBoss(cl);
        cl.display.setLocation(200, 200);
        cl.display.setVisible(true);


    }

public Client()
    {
        //        display = new ProbesGUI();


    }

 public ProbesGUI getDisplay()
 {
     return display;
 }

public String getNodeInformation(String nodePath)
    {
        return webAccess.retrieveNodeInformation(nodePath);

    }



}

