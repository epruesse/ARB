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

        if (args.length == 0) cl.hostname = new String("localhost");

        // inlude version number in tree
        // enables users to store tree local
        // update only when tree is changed

        // here goes the web access code

        // tree to display
        cl.treeString = new TreeReader().getTreeString();

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

}

