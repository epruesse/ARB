// ProbesGUI.java

import java.util.*;
import java.awt.*;
import java.awt.event.*;

public class ProbesGUI extends Frame

{
    //private Canvas td;
private ScrollPane sc;
private TextArea details;
private TextArea info;
private int treeLevels;
private TreeDisplay td;
private TreeNode root;

// public static void main(String[] args)
//     {
//         ProbesGUI wnd = new ProbesGUI();
//         wnd.setLocation(200,200);
//         wnd.setVisible(true);
//     }


public ProbesGUI( TreeNode root, int levels)
  {
    super("ALL ARB PROBES");
    setBackground(Color.lightGray);

    setLayout(new BorderLayout());
    //    add(new Button("first"));


    details = new TextArea("Display detail information", 10, 40, TextArea.SCROLLBARS_BOTH);
    add(details, BorderLayout.SOUTH);

    info = new TextArea("short information", 20, 15, TextArea.SCROLLBARS_BOTH);
    add(info, BorderLayout.EAST);

//     td = new Canvas();
//     td.setSize(600,400);
//     td.setBackground(Color.red);    
//     add(td, BorderLayout.CENTER);

    sc = new ScrollPane();
        if (root == null) 
            {
                System.out.println("in ProbesGUI(): no valid node given to display");
                System.exit(1);
            }

    td = new TreeDisplay( root, levels);
    sc.add(td);
    sc.getVAdjustable().setUnitIncrement(1);
    sc.getHAdjustable().setUnitIncrement(1);
    sc.setSize(600,400);
    //    sc.setBackground(Color.red);    
    add(sc, BorderLayout.CENTER);

    //    add(new Button("second"));
      //    pack();
    //    add(new Button("third"));
      //Window-Listener
    addWindowListener(new WindowClosingAdapter(true));
    //Dialogelement anordnen
    pack();
  }


// public    void    paint (Graphics g){
//         details.paint(g);
//         info.paint(g);
//         td.paint(g);
//     }



 public TreeDisplay getTreeDisplay()
 {
     return td;
 }


}
