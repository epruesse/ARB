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
    // private TreeNode subtree;

public static void main(String[] args)

    {
        // create central client object(s)
        // create communication structure
        // initialization of communication
        // control of user interaction



        Client cl = new Client();

        // tree to display
        cl.treeString = new TreeReader().getTreeString();

        // parsing


        cl.root = (new TreeParser(cl.treeString)).getRootNode();

        if (cl.root == null) 
            {
                System.out.println("in Client(): no valid node given to display");
                System.exit(1);
            }


        cl.display = new ProbesGUI(cl.root, 10);
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

