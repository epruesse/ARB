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
    
    boolean                 fakeScrollPaneSize = true;
    private Dimension       estimatedScrollPaneSize;
    private final Dimension wantedScrollPaneSize;


    // public ProbesGUI(int levels, String title, Client cl, Point origin, Dimension size, int textWidth) throws Exception
    public ProbesGUI(int levels, String title, Client cl, Rectangle frame_bounds, Dimension wantedScrollPaneSize_, int textWidth) throws Exception
    {
        super(title);

        wantedScrollPaneSize    = wantedScrollPaneSize_;
        estimatedScrollPaneSize = new Dimension(frame_bounds.width, frame_bounds.height);
        client                  = cl;

        Color backgroundColor = new Color(160, 180, 255);

        setVisible(false);
        setBackground(Color.lightGray);
        setLayout(new BorderLayout());

        // if (wantedScrollPaneSize == null) {
            setBounds(frame_bounds);
        // }
        // else {
        // setLocation(frame_bounds.x, frame_bounds.y);
        // }

        td = new TreeDisplay( null, levels, backgroundColor);
        if (td == null) Toolkit.AbortWithError("constructor/ProbesGUI: no TreeDisplay received");
        td.setClient(client);

        setMenuBar(pm = new ProbeMenu(this)); // menus


        {
            Color rightPanelColor = Color.white;
            Panel rightPanel      = new Panel();
            rightPanel.setLayout(new BorderLayout());

            probe_list = new ProbeList(this, rightPanelColor);
            showHelp();
            rightPanel.add(probe_list, BorderLayout.CENTER);

            rightPanel.add(new ProbeToolbar(this), BorderLayout.NORTH);
            details = new TextArea("Detail information", 20, textWidth, TextArea.SCROLLBARS_BOTH);
            details.setBackground(rightPanelColor);
            rightPanel.add(details, BorderLayout.SOUTH);

            add(rightPanel, BorderLayout.EAST);

            rightPanel.validate();
            Dimension rightPanelDim           = rightPanel.getPreferredSize();
            // Dimension rightPanelDim        = rightPanel.getSize();
            // estimatedScrollPaneSize.width -= rightPanelDim.width;
            // System.out.println("rightPanelDim="+rightPanelDim.width+"/"+rightPanelDim.height+" (wrong)");
            estimatedScrollPaneSize.width    -= 300;
        }

        {
            Panel treePanel = new Panel();
            treePanel.setLayout(new BorderLayout());

            TreeToolbar ttb = new TreeToolbar(this);
            treePanel.add(ttb, BorderLayout.NORTH);

            sc = new ScrollPane() {

                    public Dimension getPreferredSize() {
                        Dimension result = null;
                        if (fakeScrollPaneSize == false) {
                            System.out.println("Returning real size:");
                            result = getSize(); // ignores size of contained canvas!
                        }
                        else {
                            result = estimatedScrollPaneSize;
                            if (wantedScrollPaneSize != null) {
                                result = wantedScrollPaneSize;
                                System.out.println("I know the exact size:");
                            }
                        }
                        System.out.println("ScrollPane::getPreferredSize returns "+result.width+"/"+result.height);
                        return result;
                    }
                };

            sc.add(td);
            sc.getVAdjustable().setUnitIncrement(1);
            sc.getHAdjustable().setUnitIncrement(1);

            // sc.setSize(estimatedPaneSize); // need to set this here (allows to calc. correct size for 'ttb'

            // sc.setWheelScrollingEnabled(true); // later - needs java 1.4 :(
            treePanel.add(sc, BorderLayout.CENTER);
            add(treePanel);

            Dimension ttbdim                = ttb.getPreferredSize();
            estimatedScrollPaneSize.height -= ttbdim.height;
            System.out.println("ttbdim="+ttbdim.width+"/"+ttbdim.height+"");


            estimatedScrollPaneSize.height -= 40+50; // for menubar

            // sc.setSize(sc.getPreferredSize());
            // sc.setSize(estimatedPaneSize);
        }

        // pack();
        // setVisible(true);
        // setBounds(origin.x, origin.y, size.width, size.height);
        // System.out.println("setBounds= "+origin.x+"/"+origin.y+" w/h="+size.width+"/"+size.height);

        // System.out.println("call invalidate");
        // invalidate();

        System.out.println("call pack");
        validate();
        pack();
        System.out.println("call pack done");
        fakeScrollPaneSize = false;
        setVisible(true);
        // setBounds(frame_bounds);
        // Client.showRect(frame_bounds, "setBounds(frame_bounds) after pack");


        // setLocation(frame_bounds.x, frame_bounds.y);
        // fakeScrollPaneSize = false;
        // validate();
        // pack();

        // invalidate();
        // doLayout();
        // pack();

        addWindowListener(new WindowClosingAdapter(true));
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
        probe_list.removeAll();
        probe_list.add("Left click a branch to unfold or step.");
        probe_list.add("Right click a branch to display probes.");
    }

    public void initTreeDisplay(TreeNode root)  throws Exception {
        td.setRootNode(root);
    }

    public Dimension getPreferredScrollPaneSize() { return sc.getPreferredSize(); }
    public Dimension getScrollPaneViewportSize() { return sc.getViewportSize(); }
    public TreeDisplay getTreeDisplay() { return td; }
    public ProbeList getProbeList() { return probe_list; }
    public Client getClient() { return client; }
    public TextArea getDetails() { return details; }


}
