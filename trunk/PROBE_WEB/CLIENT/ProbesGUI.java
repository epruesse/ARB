// ProbesGUI.java

import java.util.*;
import java.awt.*;
import java.awt.event.*;

public class ProbesGUI extends Frame

{
    //private Canvas td;
private ScrollPane              sc;
private TextArea                details;
private ProbeList               probe_list;
private int                     probeListWidth = 200;
private int                     treeLevels;
private TreeDisplay             td;
private TreeNode                root;
private ProbeMenu               pm;
private ProbesGUIActionListener al;
private ProbeListActionListener ll;
private Client                  boss;

public ProbesGUI( TreeNode root, int levels, String title, Client b)
  {
    super(title);
    setBackground(Color.lightGray);

    setLayout(new BorderLayout());
    // System.out.println("reference to ProbesGUI: " + this);
    boss = b;
    td = new TreeDisplay( root, levels);

    // debug code
    if (td == null) {
        System.out.println("constructor/ProbesGUI: no TreeDisplay received");
    }

    int tree_height = 400;

    al = new ProbesGUIActionListener(this);
    setMenuBar(pm = new ProbeMenu(al));

    details = new TextArea("Display detail information", 10, 40, TextArea.SCROLLBARS_BOTH);
    add(details, BorderLayout.SOUTH);

    probe_list = new ProbeList(probeListWidth, tree_height);
    ll = new ProbeListActionListener(this);
    probe_list.addItemListener(ll);
    probe_list.add("Right click a node");
    probe_list.add("to display probes.");
    add(probe_list, BorderLayout.EAST);

    sc = new ScrollPane();
    if (root == null) {
        Toolkit.InternalError("in ProbesGUI: can't display invalid tree node");
    }

    sc.add(td);
    sc.getVAdjustable().setUnitIncrement(1);
    sc.getHAdjustable().setUnitIncrement(1);
    sc.setSize(600,tree_height);
    sc.setBackground(Color.red);
    add(sc, BorderLayout.CENTER);

    //Window-Listener
    addWindowListener(new WindowClosingAdapter(true));

    //Dialogelement anordnen
    pack();
  }

public TreeDisplay getTreeDisplay()
    {
        if (td == null) {
            System.out.println("ProbesGUI/getTreeDisplay: TreeDisplay not defined");
        }

        return td;
    }

public ProbeList getProbeList()
    {
        return probe_list;
    }

public Client getClient()
    {
        return boss;
    }

}
