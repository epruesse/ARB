// ProbesGUI.java

import java.util.*;
import java.awt.*;
import java.awt.event.*;

public class ProbesGUI extends Frame

{
    //private Canvas td;
private ScrollPane sc;
private TextArea details;
private java.awt.List info;
private int treeLevels;
private TreeDisplay td;
private TreeNode root;
private ProbeMenu pm;
private ProbesGUIActionListener al;

// public static void main(String[] args)
//     {
//         ProbesGUI wnd = new ProbesGUI();
//         wnd.setLocation(200,200);
//         wnd.setVisible(true);
//     }


public ProbesGUI( TreeNode root, int levels, String title)
  {
    super(title);
    setBackground(Color.lightGray);

    setLayout(new BorderLayout());
    //    add(new Button("first"));
    System.out.println("reference to ProbesGUI: " + this);

    td = new TreeDisplay( root, levels);

    // debug code
    if (td == null)
        {
            System.out.println("constructor/ProbesGUI: no TreeDisplay received");

        }


    al = new ProbesGUIActionListener(this);
    setMenuBar(pm = new ProbeMenu(al));
    // setMenuBar(pm = new ProbeMenu());

    details = new TextArea("Display detail information", 10, 40, TextArea.SCROLLBARS_BOTH);
    add(details, BorderLayout.SOUTH);

    //    info = new TextArea("short information", 20, 15, TextArea.SCROLLBARS_BOTH);
    info = new java.awt.List();
    add(info, BorderLayout.EAST);

//     td = new Canvas();
//     td.setSize(600,400);
//     td.setBackground(Color.red);
//     add(td, BorderLayout.CENTER);

    sc = new ScrollPane();
        if (root == null) {
            Toolkit.InternalError("in ProbesGUI: can't display invalid tree node");
        }


    sc.add(td);
    sc.getVAdjustable().setUnitIncrement(1);
    sc.getHAdjustable().setUnitIncrement(1);
    sc.setSize(600,400);
    sc.setBackground(Color.red);
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
     if (td == null)
         {
             System.out.println("ProbesGUI/getTreeDisplay: TreeDisplay not defined");
         }

     return td;
 }


}
