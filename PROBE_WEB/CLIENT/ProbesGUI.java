// ProbesGUI.java

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.text.*;

public class ProbesGUI extends Frame
{
    private Client       client; 
    private ProbeMenu    pm;
    private TreeToolbar  tree_toolbar;
    private ScrollPane   sc;
    private TreeDisplay  td;
    private ProbeToolbar probe_toolbar;
    private ProbeList    probe_list;
    private TextArea     details;

    private TreeNode root;

    private boolean         fakeScrollPaneSize = true;
    private Dimension       estimatedScrollPaneSize;
    private final Dimension wantedScrollPaneSize;

    public ProbesGUI(int levels, String title, Client cl, Rectangle frame_bounds, Dimension wantedScrollPaneSize_, int textWidth) throws Exception
    {
        super(title);

        wantedScrollPaneSize    = wantedScrollPaneSize_;
        estimatedScrollPaneSize = new Dimension(frame_bounds.width, frame_bounds.height);
        client                  = cl;

        Color backgroundColor    = new Color(160, 180, 255);
        Font  fixedFont          = findFixedFont();
        // if (fixedFont           == ;
            // Font fixedFont       = new Font("Courier", Font.PLAIN, 14); // used 4 probe list
            // Font  fixedFont   = new Font("Monospaced", Font.PLAIN, 14); // used 4 probe list
            // Font  normalFont  = new Font("SansSerif", Font.PLAIN, 18); // used everywhere else

        setVisible(false);
        setBackground(Color.lightGray);
        setLayout(new BorderLayout());
        // setFont(normalFont);

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

            // rightPanel.setFont(fixedFont); // valid for all childs
            rightPanel.setLayout(new BorderLayout());

            probe_list = new ProbeList(this, rightPanelColor, fixedFont);
            rightPanel.add(probe_list, BorderLayout.CENTER);

            rightPanel.add(probe_toolbar = new ProbeToolbar(this), BorderLayout.NORTH);

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

            tree_toolbar = new TreeToolbar(this);
            treePanel.add(tree_toolbar, BorderLayout.NORTH);

            sc = new ScrollPane() {

                    public Dimension getPreferredSize() {
                        Dimension result = null;
                        if (fakeScrollPaneSize == false) {
                            // System.out.println("Returning real size:");
                            result = getSize(); // ignores size of contained canvas!
                        }
                        else {
                            result = estimatedScrollPaneSize;
                            if (wantedScrollPaneSize != null) {
                                result = wantedScrollPaneSize;
                                // System.out.println("I know the exact size:");
                            }
                        }
                        // System.out.println("ScrollPane::getPreferredSize returns "+result.width+"/"+result.height);
                        return result;
                    }
                };

            sc.add(td);
            sc.getVAdjustable().setUnitIncrement(1);
            sc.getHAdjustable().setUnitIncrement(1);

            // sc.setSize(estimatedPaneSize); // need to set this here (allows to calc. correct size for 'tree_toolbar'

            // sc.setWheelScrollingEnabled(true); // later - needs java 1.4 :(
            treePanel.add(sc, BorderLayout.CENTER);
            add(treePanel);

            Dimension ttbdim                = tree_toolbar.getPreferredSize();
            estimatedScrollPaneSize.height -= ttbdim.height;
            // System.out.println("ttbdim="+ttbdim.width+"/"+ttbdim.height+"");


            estimatedScrollPaneSize.height -= 40+50; // for menubar

            sc.setSize(sc.getPreferredSize());
            // sc.setSize(estimatedPaneSize);
        }

        // pack();
        // setVisible(true);
        // setBounds(origin.x, origin.y, size.width, size.height);
        // System.out.println("setBounds= "+origin.x+"/"+origin.y+" w/h="+size.width+"/"+size.height);

        // System.out.println("call invalidate");
        // invalidate();

        // System.out.println("call pack");
        validate();
        pack();
        // System.out.println("call pack done");
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

    private static String probe_list_chars = "ACGTU=%0123456789° ";

    private static Font findFixedFont() {
        if (false) {
            // @@@ FIXME: currently disabled - unfinished
            
            GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
            Font allFonts[]        = ge.getAllFonts();

            for (int i = 0; i<allFonts.length; ++i) {
                Font f = allFonts[i];
                if (f.isPlain()) { // don't check bold and italic fonts
                    System.out.print("checking font '"+f.getName()+"'");

                    int can = f.canDisplayUpTo(probe_list_chars); // slow!

                    System.out.print(" family='"+f.getFamily()+"'");
                    // System.out.print(" size='"+f.getSize()+"'"); // always 1
                    System.out.print(" style='"+f.getStyle()+"'");
                    // System.out.print(" weight='"+f.getWeight()+"'");


                    // int can = f.canDisplayUpTo(probe_list_chars); // slow!
                    // if (can == -1 || can == probe_list_chars.length()) {
                    // System.out.print(" OK");
                    // }
                    // else {
                    // System.out.print(" cannot display '"+probe_list_chars.substring(can)+"' can="+can);
                    // }

                    System.out.println("");
                }
            }
        }
        return new Font("Monospaced", Font.PLAIN, 14);
    }

    public void toggleOverlap() throws Exception {
        // Note : intentionally placed here (because a 2nd probelist is planned)

        Probe overlapping = Probe.getOverlapProbe();
        Probe selected    = probe_list.getProbe(probe_list.getSelectedIndex());

        System.out.println("toggleOverlap!");
        if (overlapping != selected) {
            // show overlaps with current probe
            probe_list.showOverlap(selected);
        }
        else {
            probe_list.showOverlap(null); // hide overlaps
        }

        probe_list.resort();
    }

    public void showHelp() {
        Toolkit.showMessage("Quick help:\n");
        Toolkit.showMessage("Left click to unfold or step into.");
        Toolkit.showMessage("Right click to display probes.");
        Toolkit.showMessage("Click squares to collapse.\n");
        Toolkit.showMessage("more help is available online at\nhttp://www.arb-home.de/probeclient.html");
    }

    public void initTreeDisplay(TreeNode root)  throws Exception {
        td.setRootNode(root);
    }

    public Dimension getPreferredScrollPaneSize() { return sc.getPreferredSize(); }
    public Dimension getScrollPaneViewportSize() { return sc.getViewportSize(); }

    public TreeDisplay getTreeDisplay() { return td; }
    public TreeToolbar getTreeToolbar() { return tree_toolbar; }

    public ProbeList getProbeList() { return probe_list; }
    public ProbeToolbar getProbeToolbar() { return probe_toolbar; }
    public TextArea getDetails() { return details; }

    public Client getClient() { return client; }

}
