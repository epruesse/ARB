// ProbesGUI.java

import java.util.*;
import java.awt.*;
import java.awt.event.*;

public class ProbesGUI extends Frame
{
    private ScrollPane  sc;
    private TreeDisplay td;
    private TreeNode    root;
    private TextArea    details;
    private ProbeList   probe_list;
    // private int         probeListWidth = 200;
    private ProbeMenu   pm;
    private Client      client;


    public ProbesGUI(int levels, String title, Client cl, Point origin, Dimension size, int textWidth) throws Exception
    {
        super(title);
        setVisible(true);

        Color backgroundColor = new Color(160, 180, 255);

        setBackground(Color.lightGray);
        setLayout(new BorderLayout());
        // setLocation(10, 10);
        setLocation(origin);

        client = cl;

        td = new TreeDisplay( null, levels, backgroundColor);
        if (td == null) Toolkit.AbortWithError("constructor/ProbesGUI: no TreeDisplay received");
        td.setClient(client);

        setMenuBar(pm = new ProbeMenu(this)); // menus

        // sum up dimension outside of ScrollPane 
        int xtraWidth  = 0;
        int xtraHeight = 0;

        ProbeToolbar tb = new ProbeToolbar(this);
        
        {
            Color panelColor = Color.white;
            Panel panel      = new Panel();
            panel.setLayout(new BorderLayout());

            probe_list = new ProbeList(this, panelColor);
            showHelp();
            panel.add(probe_list, BorderLayout.CENTER);

            panel.add(tb, BorderLayout.NORTH);
            details = new TextArea("Detail information", 20, textWidth, TextArea.SCROLLBARS_BOTH);
            details.setBackground(panelColor);
            panel.add(details, BorderLayout.SOUTH);

            add(panel, BorderLayout.EAST);

            Dimension pdim = panel.getPreferredSize();
            System.out.println("pdim = "+pdim.width+"/"+pdim.height);
            xtraWidth += pdim.width;
        }

        {
            Panel treePanel = new Panel();
            treePanel.setLayout(new BorderLayout());

            treePanel.add(new TreeToolbar(this), BorderLayout.NORTH);

            sc = new ScrollPane() {
                    public Dimension getPreferredSize() {
                        System.out.println("ScrollPane::getPreferredSize called");
                        return getSize(); // ignores size of contained canvas!
                    }
                    public Dimension getMaximumSize() {
                        System.out.println("ScrollPane::getMaximumSize called");
                        return getSize(); // ignores size of contained canvas!
                    }
                    public Dimension getMinimumSize() {
                        System.out.println("ScrollPane::getMinimumSize called");
                        return getSize(); // ignores size of contained canvas!
                    }
                };

            sc.add(td);
            sc.getVAdjustable().setUnitIncrement(1);
            sc.getHAdjustable().setUnitIncrement(1);

            System.out.println("xtraWidth="+xtraWidth+" xtraHeight="+xtraHeight);
            System.out.println("wanted frame size = "+size.width+"/"+size.height);
            size.width  -= xtraWidth;
            System.out.println("calc. sc size = "+size.width+"/"+size.height);

            sc.setSize(size);

            Dimension scdim = sc.getSize();
            System.out.println("resulting sc size = "+scdim.width+"/"+scdim.height);
            //sc.setSize(640-20,480-20);
            // sc.setWheelScrollingEnabled(true); // later - needs java 1.4 :(

            treePanel.add(sc, BorderLayout.CENTER);

            add(treePanel);
        }

        addWindowListener(new WindowClosingAdapter(true));

        Dimension    tbdim  = tb.getPreferredSize();
        System.out.println("tbdim = "+tbdim.width+"/"+tbdim.height);
        xtraHeight         += tbdim.height;

        // Dimension    pmdim  = pm.getSize();
        // System.out.println("pmdim = "+pmdim.width+"/"+pmdim.height);
        xtraHeight         += 40; // for menubar
        
        size.height -= xtraHeight;
        System.out.println("calc. sc size = "+size.width+"/"+size.height+" (2nd correction)");

        sc.setSize(size);


        // setSize(size);
        // invalidate();
        Dimension scdim = sc.getSize();
        System.out.println("scdim = "+scdim.width+"/"+scdim.height);
        System.out.println("packing ProbesGUI Frame");
        pack();


        // setSize(scdim);
        scdim = sc.getSize();
        System.out.println("final scdim = "+scdim.width+"/"+scdim.height);

        Dimension pgdim = getSize();
        System.out.println("final pgdim = "+pgdim.width+"/"+pgdim.height);

    }

    public void toggleOverlap() throws Exception {
        // Note : intentionally placed here (because a 2nd probelist is planned)

        Probe overlapping = Probe.getOverlapProbe();
        Probe selected    = probe_list.getProbe(probe_list.getSelectedIndex());

        System.out.println("toggleOverlap!");
        if (overlapping != selected) {
            // show overlaps with current probe
            System.out.println("overlapping != selected => set selected");
            probe_list.showOverlap(selected);
        }
        else {
            System.out.println("clear overlap");
            probe_list.showOverlap(null); // hide overlaps
        }

        probe_list.resort();
    }

    public void showHelp() {
        probe_list.clear();
        probe_list.add("Left click a branch to unfold or step.");
        probe_list.add("Right click a branch to display probes.");
    }

    public void initTreeDisplay(TreeNode root)  throws Exception {
        td.setRootNode(root);
    }

    public Dimension getScrollPaneSize() { return sc.getViewportSize(); }
    public TreeDisplay getTreeDisplay() { return td; }
    public ProbeList getProbeList() { return probe_list; }
    public Client getClient() { return client; }
    public TextArea getDetails() { return details; }


}
