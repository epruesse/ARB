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
    private int         probeListWidth = 200;
    private ProbeMenu   pm;
    private Client      client;


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

        setMenuBar(pm = new ProbeMenu(this)); // menus

        {
            Color panelColor = Color.white;
            Panel panel      = new Panel();
            panel.setLayout(new BorderLayout());

            probe_list = new ProbeList(this, panelColor);
            showHelp();
            panel.add(probe_list, BorderLayout.CENTER);

            panel.add(new ProbeToolbar(this), BorderLayout.NORTH);

            details = new TextArea("Display detail information", 20, 40, TextArea.SCROLLBARS_BOTH);
            details.setBackground(panelColor);
            panel.add(details, BorderLayout.SOUTH);

            add(panel, BorderLayout.EAST);
        }

        {
            Panel treePanel = new Panel();
            treePanel.setLayout(new BorderLayout());

            treePanel.add(new TreeToolbar(this), BorderLayout.NORTH);

            sc = new ScrollPane();

            sc.add(td);
            sc.getVAdjustable().setUnitIncrement(1);
            sc.getHAdjustable().setUnitIncrement(1);
            sc.setSize(600,600);
            // sc.setWheelScrollingEnabled(true); // later - needs java 1.4 :(

            treePanel.add(sc, BorderLayout.CENTER);

            add(treePanel);
        }

        addWindowListener(new WindowClosingAdapter(true));

        pack();
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
