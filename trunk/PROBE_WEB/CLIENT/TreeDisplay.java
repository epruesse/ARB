//package de.arbhome.arb_probe_library;

// TreeDisplay.java

import java.lang.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
//import java.Thread.*;

public class TreeDisplay extends Canvas
{
    private long     displayCounter;
    private String   treeString;
    private TreeNode root;
    private TreeNode visibleSubtree;        // visible subtree
    // private TreeNode prevNode;
    private TreeNode lastFoldedNode; // last node folded/unfolded by hand 
    private TreeNode lastMatchedNode; // last node probe has been ran for

    // if lockedPositionNode is set to a node
    // -> next repaint tries to keep that node at the same position
    // if moveToPositionNode is set to a diff. node
    // -> next repaint tries to set that node at the position where lockedPositionNode was
    private TreeNode lockedPositionNode = null;
    private TreeNode moveToPositionNode = null; 

    private Stack   history;

    private int xSize     = 0;
    private int ySize     = 0;
    private int scaleBarY = -1;

    private int leafNumber;

    private HashMap branchLines; // vertical lines
    private HashMap rootLines;  // horizontal lines
    private Client  myClient;

    private int leafSpace   = 15;
    private int leafOffset  = 10;
    private int groupSpace  = 30;
    private int groupOffset = 20;
    private int leaf_text_size = 300; // additional space for leaf-text
    private int group_text_size = 300; // additional space for group-text

    private int xPointer = 0;   // serves as pointer to the x-space already used up (w/o leaf text)
    private int yPointer = 0;   // serves as pointer to the y-space already used up

    private boolean         newLayout = true;
    private LineAreaFactory laf;
    private Set             rootSet;
    private Set             branchSet;

    private static Color dc;    // normal display color
    private static Color mc   = Color.yellow; // marked display color
    private static Color pmc  = Color.white; // partly marked display color
    private static Color afc  = Color.cyan; // autofold color (text only)
    private static Color lfnc = Color.green; // lastFoldedNode color
    private static Color lmnc = Color.orange; // lastMatchedNode color
    private static Color nic1 = Color.blue; // node info color 1 (not used for species name etc.)
    private static Color nic2 = Color.magenta; // node info color 2 (not used for group name)

    private int group_box_size = 3; // size of group box (real size = group_box_size*2+1)
    private int clickTolerance = 5;
    private int xSpreading     = 1000;
    private int min_xSpreading = 100;

    public TreeDisplay(TreeNode root, int treeLevels, Color backgroundColor) throws Exception
    {
        setBackground(Color.lightGray);

        displayCounter = 0;

        rootLines    = new HashMap();
        branchLines  = new HashMap();
        rootSet      = new HashSet();
        branchSet    = new HashSet();
        history      = new Stack();
        laf          = new LineAreaFactory(6);

        if (root != null) {
            setRootNode(root);
        }

        setNewSize(100, 100); // just any initial size (resized by parent)
        setBackground(backgroundColor);
    }

    static private boolean first_validation_error = true;

    private void setNewSize(int minx, int miny) {
        Container parent = getParent();
        if (parent == null) {
            xSize = minx;
            ySize = miny;
            setSize(xSize, ySize);
        }
        else {
            Dimension dim = parent.getSize(); // size of scroll pane

            dim.width -= 5;
            dim.height -= 5;

            xSize = minx > dim.width ? minx : dim.width;
            ySize = miny > dim.height ? miny : dim.height;
            setSize(xSize, ySize);
        }

        // System.out.println("new Size="+xSize+"/"+ySize);

        // try to correct a sizing bug causing wrong scrolling
        try {
            invalidate();
            getParent().validate();
        }
        catch (Exception e) {
            if (!first_validation_error) System.out.println("(in)validate failed");
            first_validation_error = false;
        }
    }

    private void setNewDrawSize(int drawx, int drawy) {
        setNewSize(drawx+leaf_text_size, // space for leaf text
                   drawy+50); // a bit space below tree
    }

    private void recalcLayoutSize() {
        xPointer = (int)(visibleSubtree.calculateTotalDist(0)*xSpreading+0.5);
        yPointer = 0;
        calculateYValues(visibleSubtree);
        System.out.println("xPointer="+xPointer+" yPointer="+yPointer);
    }

    private void recalcLayout() {
        rootLines.clear();
        branchLines.clear();
        rootSet.clear();
        branchSet.clear();

        recalcLayoutSize();


        // avoid tree displays >= 32768 by autofolding the tree

        // int max_display_ysize    = 32768-1; // doesn't work with Win98
        int max_display_ysize    = 28000;
        // int max_display_ysize = 3000; // for testing
        int max_loop             = 20;
        while (yPointer>max_display_ysize && max_loop>0) {
            max_loop--;
            visibleSubtree.autofold(yPointer-max_display_ysize);
            // System.out.println("autofolded!");
            recalcLayoutSize();
            // if (yPointer>max_display_ysize) {
                // System.out.println("autofolding failed (display_ysize = "+yPointer+")");
            // }
        }

        setNewDrawSize(xPointer, yPointer);

        Dimension scrollPaneSize = getParent().getSize();
        int       tolerance      = 30;
        int       old_xSpreading = xSpreading;

        if ((xPointer+leaf_text_size) < (scrollPaneSize.width-tolerance)) {
            xSpreading *= scrollPaneSize.width/(double)(xPointer+leaf_text_size);
        }
        else if ((xPointer+leaf_text_size) > (scrollPaneSize.width+tolerance)) {
            xSpreading *= scrollPaneSize.width/(double)(xPointer+leaf_text_size);
        }

        if (xSpreading<min_xSpreading) xSpreading = min_xSpreading;

        if (xSpreading != old_xSpreading) {
//             System.out.println("old xSpreading="+old_xSpreading);
//             System.out.println("new xSpreading="+xSpreading);
            recalcLayout();
        }
    }

    public void setVisibleSubtree(TreeNode vis) {
        lockedPositionNode = visibleSubtree;

        visibleSubtree     = vis;
        moveToPositionNode = visibleSubtree;
        newLayout          = true;

        if (visibleSubtree != null) {
            AutofoldCandidate.annouce_POI(visibleSubtree);
            if (lockedPositionNode != null) {
                //                 System.out.println("visibleSubtree="+visibleSubtree);
                //                 try {
                //                     Toolkit.AbortWithError("Test");
                //                 }
                //                 catch (Exception e) {
                //                     Toolkit.showError(e.getMessage());
                //                     e.printStackTrace();
                //                 }
                visibleSubtree.autounfold();
            }
        }
    }

    public void setRootNode(TreeNode root) throws Exception {
        if (this.root != null) Toolkit.AbortWithError("TreeDisplay: root set twice");
        if (root == null) Toolkit.AbortWithError("TreeDisplay: no root node obtained from caller");

        this.root = root;
        setVisibleSubtree(root);
        visibleSubtree.setDistance(0.01);
        leafNumber = visibleSubtree.getNoOfLeaves();
        Toolkit.showMessage("Tree has " + leafNumber + " leaves.");

        //         recalcLayout(); // done inside repaint

        addMouseListener(new DisplayMouseAdapter(this));
        repaint();
    }


    static Dimension last_scroll_pane_size = new Dimension(0, 0);

    public void paint(Graphics g)
    {
        if (root == null) return;
        if (dc == null) dc = g.getColor(); // get draw color ?

        // check whether auto-layout should be done
        {
            ScrollPane scrollPane     = (ScrollPane)getParent();
            Dimension  scrollPaneSize = scrollPane.getSize();
            // System.out.println("scrollPaneSize="+scrollPaneSize.width+"/"+scrollPaneSize.height);
            if (!scrollPaneSize.equals(last_scroll_pane_size)) {
                newLayout = true;   // auto-layout if scrollPaneSize changes
                last_scroll_pane_size = scrollPaneSize;
                // System.out.println("auto-layout");
            }
        }

        try {
            if (lockedPositionNode == null) {
                if (newLayout == true) recalcLayout(); // tree changed, layout new tree
                displayTreeGraph(g, visibleSubtree);
            }
            else {
                // try to scroll to reasonable position

                if (moveToPositionNode == null) moveToPositionNode = lockedPositionNode;

                ScrollPane scrollPane = (ScrollPane)getParent();

                Point old_locked_position = new Point(lockedPositionNode.getCurrentXoffset(), lockedPositionNode.getCurrentYoffset());
                Point old_scroll_position = scrollPane.getScrollPosition();

                int locked_x_offset = old_locked_position.x - old_scroll_position.x;
                int locked_y_offset = old_locked_position.y - old_scroll_position.y;

                //                 System.out.println("old_locked_position="+old_locked_position.x+"/"+old_locked_position.y);
                //                 System.out.println("old_scroll_position="+old_scroll_position.x+"/"+old_scroll_position.y);
                //                 System.out.println("locked_?_offset="+locked_x_offset+"/"+locked_y_offset);

                if (newLayout == true) recalcLayout(); // tree changed, layout new tree
                displayTreeGraph(g, visibleSubtree);

                Point new_locked_position = new Point(moveToPositionNode.getCurrentXoffset(), moveToPositionNode.getCurrentYoffset());
                Point new_scroll_position = new Point(new_locked_position.x-locked_x_offset, new_locked_position.y-locked_y_offset);

                Dimension canvasSize          = getSize();
                Dimension scrollPaneSize      = scrollPane.getSize();
                Point     max_scroll_position = new Point(canvasSize.width-scrollPaneSize.width, canvasSize.height-scrollPaneSize.height);

                if (new_scroll_position.x > max_scroll_position.x) new_scroll_position.x = max_scroll_position.x;
                if (new_scroll_position.y > max_scroll_position.y) new_scroll_position.y = max_scroll_position.y;

                if (new_scroll_position.x < 0) new_scroll_position.x = 0;
                if (new_scroll_position.y < 0) new_scroll_position.y = 0;

                //                 System.out.println("new_locked_position="+new_locked_position.x+"/"+new_locked_position.y);
                //                 System.out.println("new_scroll_position="+new_scroll_position.x+"/"+new_scroll_position.y);

                int new_locked_x_offset = new_locked_position.x - new_scroll_position.x;
                int new_locked_y_offset = new_locked_position.y - new_scroll_position.y;

                //                 System.out.println("new_locked_?_offset="+new_locked_x_offset+"/"+new_locked_y_offset);
                //                 if (locked_x_offset == new_locked_x_offset && locked_y_offset == new_locked_y_offset) {
                //                     System.out.println("offsets are equal");
                //                 }
                //                 else {
                //                     System.out.println("offsets differ: movex="+(new_locked_x_offset-locked_x_offset)+" movey="+(new_locked_y_offset-locked_y_offset));
                //                 }

                scrollPane.setScrollPosition(new_scroll_position);

                Point curr_scroll_position = scrollPane.getScrollPosition();
                if (!curr_scroll_position.equals(new_scroll_position)) {
                    System.out.println("too bad - did not scroll to wanted position (java sucks)");
                }

                lockedPositionNode = null;
                moveToPositionNode = null;
            }

            //             {
            //                 // debug print
            //                 ScrollPane scrollPane           = (ScrollPane)getParent();
            //                 Point      curr_scroll_position = scrollPane.getScrollPosition();
            //                 System.out.println("curr_scroll_position="+curr_scroll_position.x+"/"+curr_scroll_position.y);
            //                 System.out.println("");
            //             }

            {
                // show scale bar

                int ysize  = 5; // "half" height of scale bar
//                 int yspace = 2*ysize; // "half" space used by scalebar

                if (newLayout == true) {
                    scaleBarY = yPointer + 6*ysize;
                    yPointer  = scaleBarY + 2*ysize;
                }

                int x1 = 10;
                int x2 = x1 + (int)(0.1*xSpreading + 0.5);
                int y  = scaleBarY;

                g.setColor(pmc);
                g.drawLine(x1, y, x2, y);
                g.drawLine(x1, y-ysize, x1, y+ysize);
                g.drawLine(x2, y-ysize, x2, y+ysize);
                g.setColor(dc);
                g.drawString("0.1", x1+(x2-x1)/2-10, y-2);
            }
        }
        catch (ClientException e) {
            Toolkit.showError(e.getMessage());
        }
        catch (Exception e) {
            Toolkit.showError("while painting tree: "+e.getMessage());
            e.printStackTrace();
        }
        // new lines are registered in displayTreeGraph()

        if (rootSet.isEmpty() || branchSet.isEmpty()) // equivalent to newLayout == true except first invocation
        {
            rootSet = rootLines.keySet();
            branchSet = branchLines.keySet();
        }
        newLayout = false;

        // System.out.println("method display tree was called "+ ++displayCounter+" times");
    }

    public int calculateYValues(TreeNode node) {
        if (node.isLeaf()) {
            node.setYOffset( yPointer + leafOffset);
            yPointer = yPointer + leafSpace;
        }
        else if (node.isFoldedGroup()) {
            node.setYOffset( yPointer + groupOffset);
            yPointer = yPointer + groupSpace;
        }
        else {
            int y1 = calculateYValues(node.upperSon());
            int y2 = calculateYValues(node.lowerSon());
            node.setYOffset((y1+y2)/2);
        }
        return node.getYOffset();
    }

    private static void setColor(int markedState, Graphics g) {
        switch (markedState) {
            case 0: g.setColor(dc); break;
            case 1: g.setColor(pmc); break;
            case 2: g.setColor(mc); break;
        }
    }

    public void displayTreeGraph(Graphics g, TreeNode node) throws Exception
    {
        if (node == null) {
            Toolkit.AbortWithError("no valid node given to display");
        }

        int[] line             = new int[4];
        boolean draw_node_info = true;

        //         if ((node.isLeaf() == true) || (depth == 0)) {
        if (node.isLeaf() || node.isFoldedGroup()) {

            // horizontal line to leaf/folded_subtree

            line[0]    = (int)((node.getTotalDist() - node.getDistance())* xSpreading);
            line[1]    = (int) node.getYOffset();
            line[2]    = (int)(node.getTotalDist()*xSpreading);
            line[3]    = (int) node.getYOffset();

            setColor(node.markedState(), g);
            g.drawLine( line[0], line[1], line[2], line[3]);
            if (newLayout == true) {
                rootLines.put(laf.getLAObject(line), node);
                node.setXOffset(line[2]);
            }

            if (node.isLeaf()) { // add the node name if node is leaf
                String info = node.getDisplayString();
                int    x    = (int)( node.getTotalDist() * xSpreading) + 5;
                int    y    = node.getYOffset() +3;

                g.drawString(info, x, y);

                if (info.charAt(0) == '[') { // has info about probes
                    int brclose = info.indexOf(']');
                    if (brclose != -1) {
                        info = info.substring(0, brclose+1); // cut rest after '[...]'
                        g.setColor(nic1); // and draw again in different color


                        g.drawString(info, x, y);
                    }
                }
                draw_node_info = false;
            }
            else // draws a box with number of childs and group name
            {
                g.drawRect((int)(node.getTotalDist() * xSpreading), (int)(node.getYOffset() - (groupOffset/2)),
                           group_text_size , groupOffset);

                String groupText = node.getGroupName()+" ["+node.getNoOfLeaves()+"]";
                if (node.isAutofolded()) g.setColor(afc);
                g.drawString(groupText, (int)( node.getTotalDist() * xSpreading) + 5, node.getYOffset() +4 );
                // g.drawString(Integer.toString(node.getNoOfLeaves()), (int)( node.getTotalDist() * xSpreading) + 5, node.getYOffset() +4 );
            }
        }
        else {
            line[0] = (int)((node.getTotalDist() - node.getDistance())* xSpreading);
            line[1] = node.getYOffset();
            line[2] = (int)(node.getTotalDist()*xSpreading);
            line[3] = node.getYOffset();

            setColor(node.markedState(), g);
            g.drawLine( line[0], line[1], line[2], line[3]); // draw horizontal line
            if (newLayout == true) {
                rootLines.put(laf.getLAObject(line), node);
                node.setXOffset(line[2]);
            }

            if (node.isGroup()) { // unfolded group -> draw small box
                g.fillRect(line[0]-group_box_size, line[1]-group_box_size, 2*group_box_size+1, 2*group_box_size+1);
            }

            // draw upper child

            TreeNode upChild = node.upperSon();
            int[] upline     = new int[4];

            upline[0] = (int)((node.getTotalDist())* xSpreading);
            upline[1] = upChild.getYOffset();
            upline[2] = line[2];
            upline[3] = line[3];

            setColor(upChild.markedState(), g);
            g.drawLine( upline[0], upline[1], upline[2], upline[3]);
            if (newLayout == true) branchLines.put(laf.getLAObject(upline), upChild);

            displayTreeGraph (g, upChild);

            // draw lower child

            TreeNode downChild = node.lowerSon();
            int[] downline     = new int[4];

            downline[0] = upline[0];
            downline[1] = downChild.getYOffset();
            downline[2] = line[2];
            downline[3] = line[3];

            setColor(downChild.markedState(), g);
            g.drawLine( downline[0], downline[1], downline[2], downline[3]);
            if (newLayout == true) branchLines.put(laf.getLAObject(downline), downChild);

            displayTreeGraph (g, downChild);
        }

        // lastFoldedNode indicator
        if (node == lastFoldedNode) {
            int box_size = group_box_size; // same size ok, because it always is a group 
            
            line[0] = (int)((node.getTotalDist() - node.getDistance())* xSpreading);
            line[1] = node.getYOffset();

            g.setColor(lfnc);
            g.fillRect(line[0]-box_size, line[1]-box_size, 2*box_size+1, 2*box_size+1);
        }
        
        // lastMatchedNode indicator
        if (node == lastMatchedNode) {
            int box_size = group_box_size-(node.isGroup() ? 1 : 0);

            line[0] = (int)((node.getTotalDist() - node.getDistance())* xSpreading);
            line[1] = node.getYOffset();
            g.setColor(lmnc);
            g.fillRect(line[0]-box_size, line[1]-box_size, 2*box_size+1, 2*box_size+1);
        }

        // draw inner node information
        // (this is done AFTER drawing childs to ensure it's on top)
        if (draw_node_info) {
            String info = node.getDisplayString();
            if (info.length() > 0) {
                int x = line[0]+3;
                int y = line[1]-1;

                setColor(node.markedState(), g);
                g.drawString(info, x, y);

                if (info.charAt(0) == '[') { // has info about probes
                    int brclose = info.indexOf(']');
                    if (brclose != -1) {
                        info = info.substring(0, brclose+1); // cut rest after '[...]'

                        if (info.indexOf('%') == -1) { // have no exact group
                            g.setColor(nic1);
                        }
                        else {
                            g.setColor(nic2);
                        }

                        g.drawString(info, x, y); // and draw again in different color
                    }
                }
            }
        }
    }

    public void handleLeftMouseClick(int x, int y) {
        TreeNode clickedNode = getClickedNode(x, y);
        if ( clickedNode != null) {
            boolean goto_node = true;

            AutofoldCandidate.annouce_POI(clickedNode);

            if (clickedNode.isGroup()) {
                if (clickedNode.isFoldedGroup()) {
                    clickedNode.unfold();
                    clickedNode.autounfold();

                    goto_node          = false;
                    lockedPositionNode = clickedNode;
                    lastFoldedNode    = clickedNode;
                }
                else {
                    TreeNode foldGroupNode = getBoxClickedNode(x, y);
                    if (foldGroupNode == clickedNode) { // if small box has been clicked -> fold group
                        clickedNode.fold();
                        goto_node          = false;
                        lockedPositionNode = clickedNode;
                        lastFoldedNode    = clickedNode;
                    }
                }
            }

            if (goto_node) {
                history.push(visibleSubtree);
                setVisibleSubtree(clickedNode.equals(visibleSubtree) ? clickedNode.getFather() : clickedNode);
            }

            newLayout = true;
            repaint();
        }
    }


    public void handleRightMouseClick(int x, int y) {
        TreeNode clickedNode = getClickedNode(x, y);
        if(clickedNode != null) {
            try {
                AutofoldCandidate.annouce_POI(clickedNode);
                myClient.updateNodeInformation(clickedNode);
                lastMatchedNode = clickedNode;
            }
            catch (ClientException e) {
                Toolkit.showError(e.getMessage());
            }
            catch (Exception e) {
                Toolkit.showError("in handleRightMouseClick: "+e.getMessage());
                e.printStackTrace();
            }
        }
    }


    // methods offer by pull down menus

    // 1. tree movement

    private void goAndUnfold(TreeNode dest) {
        if (visibleSubtree != dest && dest != null) {
            if (visibleSubtree != null) history.push(visibleSubtree);
            setVisibleSubtree(dest);
            if (dest.isFoldedGroup()) dest.unfold();
            repaint();
        }
    }

    public void goUp() {
        if (visibleSubtree != null)
            goAndUnfold(visibleSubtree.getFather());
    }

    public void enterUBr() {
        if (visibleSubtree != null && !visibleSubtree.isLeaf())
            goAndUnfold(visibleSubtree.upperSon());
    }

    public void enterLBr() {
        if (visibleSubtree != null && !visibleSubtree.isLeaf())
            goAndUnfold(visibleSubtree.lowerSon());
    }

    public void resetRoot() {
        if (visibleSubtree != root) {
            history.push(visibleSubtree);
            setVisibleSubtree(root);
            repaint();
        }
    }

    public void previousRoot() {
        if (!history.empty()) {
            setVisibleSubtree((TreeNode)history.pop());
            repaint();
        }
        else {
            Toolkit.showMessage("History is empty");
        }
    }

    // 2. marking species

    public void unmarkNodes() {
        if (root != null) {
            root.unmarkSubtree();
            repaint();
        }
    }

    public void gotoRootOfMarked() {
        TreeNode root_of_marked = root.getRootOfMarked();
        if (visibleSubtree != root_of_marked) {
            if (root_of_marked != null) {
                history.push(visibleSubtree);
                setVisibleSubtree(root_of_marked);
                repaint();
            }
            else {
                Toolkit.showError("Nothing marked.");
            }
        }
    }

    public void searchAndMark(String text) {
        if (root != null) {
            if (text == null || text.length() == 0) {
                root.unmarkSubtree();
            }
            else {
                String lowerText = text.toLowerCase();
                if (lowerText.equals(text)) {
                    root.searchAndMark(text, true); // case-insensitive
                }
                else {
                    root.searchAndMark(text, false); // case-sensitive
                }
            }
            repaint();
        }
    }

    public void countMarkedSpecies () {
        if (root != null) {
            Toolkit.showMessage("Number of marked species: " + root.countMarked());
            if (visibleSubtree == root) {
                Toolkit.showMessage("Number of species in tree: " + root.getNoOfLeaves());
            }
            else {
                Toolkit.showMessage("Number of marked species in shown subtree: " + visibleSubtree.countMarked());
                Toolkit.showMessage("Number of species in shown subtree: " + visibleSubtree.getNoOfLeaves());
            }
        }
        else {
            Toolkit.showMessage("You have no tree");
        }
    }

    // 3. folding

    public void unfoldAll            (){ visibleSubtree.unfoldAll            (true); newLayout = true; repaint(); }
    public void unfoldUnmarked       (){ visibleSubtree.unfoldUnmarked       (true); newLayout = true; repaint(); }
    public void unfoldCompleteMarked (){ visibleSubtree.unfoldCompleteMarked (true); newLayout = true; repaint(); }
    public void unfoldPartiallyMarked(){ visibleSubtree.unfoldPartiallyMarked(true); newLayout = true; repaint(); }

    public void foldAll              (){ visibleSubtree.foldAll              (true); newLayout = true; repaint(); }
    public void foldUnmarked         (){ visibleSubtree.foldUnmarked         (true); newLayout = true; repaint(); }
    public void foldCompleteMarked   (){ visibleSubtree.foldCompleteMarked   (true); newLayout = true; repaint(); }
    public void foldPartiallyMarked  (){ visibleSubtree.foldPartiallyMarked  (true); newLayout = true; repaint(); }

    public void smartUnfold(){ visibleSubtree.smartUnfold(); newLayout = true; repaint(); }
    public void smartFold  (){ visibleSubtree.smartFold  (); newLayout = true; repaint(); }

    public boolean unfoldMarkedFoldRest() {
        boolean unfolded = visibleSubtree.unfoldPartiallyMarked(true) || visibleSubtree.unfoldCompleteMarked(true);
        boolean folded   = visibleSubtree.foldUnmarked(true);
        if (unfolded || folded) {
            newLayout = true;
            repaint();
            return true;
        }
        return false;
    }

    // transform coordinates -> TreeNode

    private TreeNode getClickedNode(int x, int y)
    {
        for (Iterator it = rootSet.iterator(); it.hasNext();) {
            LineArea la = (LineArea) it.next();
            if (la.isInside(x,y, clickTolerance)) {
                return (TreeNode) rootLines.get(la);
            }
        }

        for (Iterator it = branchSet.iterator(); it.hasNext();) {
            LineArea la = (LineArea) it.next();
            if (la.isInside(x,y, clickTolerance)) {
                return (TreeNode) branchLines.get(la);
            }
        }

        if (x > 2*clickTolerance) { // search leftwards for node
            return getClickedNode(x-2*clickTolerance, y);
        }

        return null;
    }

    private TreeNode getBoxClickedNode(int x, int y) {
        for (Iterator it = rootSet.iterator(); it.hasNext();) {
            LineArea la = (LineArea) it.next();
            if (la.isAtLeft(x,y, group_box_size+2)) {
                return (TreeNode) rootLines.get(la);
            }
        }
        return null;
    }

    public void setClient(Client client) { myClient = client; }

    public void clearMatches() {
        lastMatchedNode = null;
        unmarkNodes();
    }

}
