// ProbesGUI.java

import java.util.*;
import java.awt.*;
import java.awt.event.*;

public class ProbesGUI extends Frame
{
    private ScrollPane              sc;
    private TextArea                details;
    private ProbeList               probe_list;
    private int                     probeListWidth = 200;
//     private int                     treeLevels;
    private TreeDisplay             td;
    private TreeNode                root;
    private ProbeMenu               pm;
//     private ProbesGUIActionListener al;
    private ProbeListActionListener ll;
    private Client                  client;
    private IOManager iom;      // wrapper to save functions


    public ProbesGUI(int levels, String title, Client cl) throws Exception
    {
        super(title);
        setVisible(true);

        Color backgroundColor = new Color(160, 180, 255);
        
        setBackground(Color.lightGray);
        setLayout(new BorderLayout());
        setLocation(200, 200);

        client = cl;

        td = new TreeDisplay( null, levels, backgroundColor);
        if (td == null) Toolkit.AbortWithError("constructor/ProbesGUI: no TreeDisplay received");
        td.setClient(client);

//         al = new ProbesGUIActionListener(this);
//         setMenuBar(pm = new ProbeMenu(al));
        setMenuBar(pm = new ProbeMenu(this));

        {
            Color panelColor = Color.white;
            Panel panel      = new Panel();
            panel.setLayout(new BorderLayout());

            probe_list = new ProbeList();
            ll = new ProbeListActionListener(this);
            probe_list.addItemListener(ll);
            probe_list.add("Right click a node");
            probe_list.add("to display probes.");
            probe_list.setBackground(panelColor);
            panel.add(probe_list, BorderLayout.CENTER);

            details = new TextArea("Display detail information", 20, 40, TextArea.SCROLLBARS_BOTH);
            details.setBackground(panelColor);
            panel.add(details, BorderLayout.SOUTH);

            add(panel, BorderLayout.EAST);
        }

        sc = new ScrollPane();

        sc.add(td);
        sc.getVAdjustable().setUnitIncrement(1);
        sc.getHAdjustable().setUnitIncrement(1);
        sc.setSize(600,600);
        // sc.setWheelScrollingEnabled(true); // later - needs java 1.4 :(

        add(sc, BorderLayout.CENTER);

        addWindowListener(new WindowClosingAdapter(true));

        pack();
    }

    public void initTreeDisplay(TreeNode root)  throws Exception {
        td.setRootNode(root);
    }

    public Dimension getScrollPaneSize() { return sc.getViewportSize(); }
    public TreeDisplay getTreeDisplay() { return td; }
    public ProbeList getProbeList() { return probe_list; }
    public Client getClient() { return client; }
    public TextArea getDetails() { return details; }


    public void setIOManager(IOManager io){
	iom = io;
	if (iom == null){System.out.println("Cannot set IOManager null"); System.exit(1);}
    }

    public IOManager getIOManager(){
	return iom;
    }


}
