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
private String baseurl;
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

//         if (args.length == 0) cl.baseurl = new String("http://probeserver.mikro.biologie.tu-muenchen.de/");
        if (args.length == 0) cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/");
        cl.webAccess = new HttpSubsystem(cl.baseurl);

        // enables users to store tree local
        // update only when tree is changed

        // here goes the web access code

        // tree to display
        String     localTreeFile = new String("probetree.gz");
        TreeReader tr            = new TreeReader(localTreeFile);

        //        cl.webAccess.conductRequest("/demo.newick");
        // inlude version number in tree
        ServerAnswer getTreeVersionAnswer = new ServerAnswer(cl.webAccess.conductRequest("getTreeVersion.cgi"));
        if (getTreeVersionAnswer.hasError()) {
            System.out.println("Error: "+getTreeVersionAnswer.getError());
            System.exit(1);
        }

        String neededClientVersion = getTreeVersionAnswer.getValue("client_version");
        System.out.println("neededClientVersion='"+neededClientVersion+"'");

        if (!neededClientVersion.equals("1.0")) {
            System.out.println("Error: Your client is out-of-date - download the new version!");
            System.exit(1);
        }

        String currentVersion = getTreeVersionAnswer.getValue("tree_version");
        System.out.println("server version: " + ">>>" + currentVersion + "<<<");

        //        String localVersion = new String("[CURRENTVERSION_16S_27081969]");
        // access local jar file;

                //                System.out.println(cl.treeString.substring(0,40));
        String localVersion = tr.getVersionString();
        System.out.println("local version: " + ">>>" + localVersion + "<<<");
        if (!currentVersion.equals(localVersion))
        {


                System.out.println("TreeVersion don't match your local version");
                System.out.println("Downloading current version");

                //                System.out.println("Downloading current tree version from server");
                //                cl.treeString = cl.webAccess.conductRequest("/demo.newick");

                cl.webAccess.downloadZippedTree(localTreeFile);
                tr = new TreeReader(localTreeFile);
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

