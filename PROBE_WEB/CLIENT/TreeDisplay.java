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
    private TreeNode prevNode;

    // if lockedPositionNode is set to a node
    // -> next repaint tries to keep that node at the same position
    // if moveToPositionNode is set to a diff. node
    // -> next repaint tries to set that node at the position where lockedPositionNode was
    private TreeNode lockedPositionNode = null;
    private TreeNode moveToPositionNode = null; 

    private Stack   history;
    private TreeSet listOfMarked;

    // private int xSize = 800;
    // private int ySize = 1000;
    private int xSize = 0;
    private int ySize = 0;

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

    //     private int             treeLevels;

    private boolean         newLayout = true;
    private LineAreaFactory laf;
    private Set             rootSet;
    private Set             branchSet;

    private static Color dc;    // normal display color
    private static Color mc   = Color.yellow; // marked display color
    private static Color pmc  = Color.white; // partly marked display color
    private Color nic1 = Color.blue; // node info color 1 (not used for species name etc.)
    private Color nic2 = Color.magenta; // node info color 2 (not used for group name)

    private int group_box_size = 3; // size of group box (real size = group_box_size*2+1)
    private int clickTolerance = 5;
//     private int yOffset        = 10; // space used for leaf/group
    private int xSpreading     = 1000;

    //     public float maxDist        = 0;

    public TreeDisplay(TreeNode root, int treeLevels, Color backgroundColor) throws Exception
    {
        setBackground(Color.lightGray);
        // postpone this later until tree is parsed to determined the needed space

        //        setVisible(true);

        displayCounter = 0;

        rootLines    = new HashMap();
        branchLines  = new HashMap();
        rootSet      = new HashSet();
        branchSet    = new HashSet();
        history      = new Stack();
        listOfMarked = new TreeSet();
        laf          = new LineAreaFactory(6);

//         this.treeLevels = treeLevels;
        if (root != null) {
            setRootNode(root);
        }

        //         xSize = 2000;
        //         ySize = 2000;
        //         setSize(xSize,ySize);
        setNewSize(400, 600);
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

    private void recalcLayout() {
        rootLines.clear();
        branchLines.clear();
        rootSet.clear();
        branchSet.clear();

        xPointer = (int)(visibleSubtree.calculateTotalDist(0)*xSpreading+0.5);
        yPointer = 0;
//         calculateYValues4Leaves(vs);
//         calculateYValues4internal(vs);
        calculateYValues(visibleSubtree);

        // System.out.println("xPointer="+xPointer+" yPointer="+yPointer);
        setNewDrawSize(xPointer, yPointer);

        Dimension scrollPaneSize = getParent().getSize();
        int       tolerance      = 30;

        if ((xPointer+leaf_text_size) < (scrollPaneSize.width-tolerance)) {
            // System.out.println("old xSpreading="+xSpreading);
            xSpreading *= scrollPaneSize.width/(double)(xPointer+leaf_text_size);
            // System.out.println("new xSpreading="+xSpreading);
            recalcLayout();
        }
        else if ((xPointer+leaf_text_size) > (scrollPaneSize.width+tolerance)) {
            // System.out.println("old xSpreading="+xSpreading);
            xSpreading *= scrollPaneSize.width/(double)(xPointer+leaf_text_size);
            // System.out.println("new xSpreading="+xSpreading);
            recalcLayout();
        }
    }

//     public Dimension getSize() {
//         System.out.println("TreeDisplay::getSize called");
//         // newLayout = true;
//         // return new Dimension(xSize, ySize);
//         return super.getSize();
//     }

    public void setVisibleSubtree(TreeNode vis) {
        lockedPositionNode = visibleSubtree;
        moveToPositionNode = vis;
        visibleSubtree     = vis;
        newLayout          = true;
    }

    public void setRootNode(TreeNode root) throws Exception {
        if (this.root != null) Toolkit.AbortWithError("TreeDisplay: root set twice");
        if (root == null) Toolkit.AbortWithError("TreeDisplay: no root node obtained from caller");

        this.root = root;
        setVisibleSubtree(root);
        visibleSubtree.setDistance(0.01);
        leafNumber = visibleSubtree.getNoOfLeaves();
        myClient.showMessage("Tree has " + leafNumber + " leaves.");

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

            // show scale bar


        }
        catch (ClientException e) {
            myClient.showError(e.getMessage());
        }
        catch (Exception e) {
            myClient.showError("while painting tree: "+e.getMessage());
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



// //     public void calculateYValues4Leaves(TreeNode node, int depth)
// //     {
// //         if ((node.isLeaf() == true) || (depth == 0))
// //         {

// //             node.setYOffset( yPointer + yOffset);
// //             yPointer = yPointer + leafSpace;
// //             if (node.isLeaf() == false && depth == 0 )
// //             {
// //                 node.setGrouped(true);
// //             }
// //         }
// //         else
// //         {
// //             node.setGrouped(false);
// //             calculateYValues4Leaves((TreeNode)(node.getChilds().elementAt(0)), depth -1 );
// //             calculateYValues4Leaves((TreeNode)(node.getChilds().elementAt(1)), depth -1 );
// //         }
// //     }
//     public void calculateYValues4Leaves(TreeNode node)
//     {
//         if (node.isLeaf() || node.isFolded()) {
//             node.setYOffset( yPointer + yOffset);
//             yPointer = yPointer + leafSpace;
//         }
//         else
//         {
//             calculateYValues4Leaves((TreeNode)(node.getChilds().elementAt(0)));
//             calculateYValues4Leaves((TreeNode)(node.getChilds().elementAt(1)));
//         }
//     }



// //     public int calculateYValues4internal(TreeNode node, int depth)
// //     {
// //         return  (node.isLeaf() || depth == 0 )?  node.getYOffset()
// //             :  node.setYOffset((calculateYValues4internal((TreeNode)(node.getChilds().elementAt(0) ), depth -1 )
// //                                 + calculateYValues4internal((TreeNode)(node.getChilds().elementAt(1) ), depth -1 )) /2 );

// //     }
//     public int calculateYValues4internal(TreeNode node)
//     {
//         return  node.isLeaf() || node.isFolded()
//                  ?  node.getYOffset()
//                  :  node.setYOffset((calculateYValues4internal((TreeNode)(node.getChilds().elementAt(0) )) +
//                                      calculateYValues4internal((TreeNode)(node.getChilds().elementAt(1) ))) / 2 );

//     }

    public int calculateYValues(TreeNode node) {
        if (node.isLeaf()) {
            node.setYOffset( yPointer + leafOffset);
            yPointer = yPointer + leafSpace;
        }
        else if (node.isFolded()) {
            node.setYOffset( yPointer + groupOffset);
            yPointer = yPointer + groupSpace;
        }
        else {
            //             int y1 = calculateYValues((TreeNode)(node.getChilds().elementAt(0)));
            //             int y2 = calculateYValues((TreeNode)(node.getChilds().elementAt(1)));

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

    public void displayTreeGraph(Graphics g, TreeNode node /*, int depth*/) throws Exception
    {
        if (node == null) {
            Toolkit.AbortWithError("no valid node given to display");
        }

        int[] line             = new int[4];
        boolean draw_node_info = true;

        //         if ((node.isLeaf() == true) || (depth == 0)) {
        if (node.isLeaf() || node.isFolded()) {

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
            else // draws a box with number of child if node is internal
            {
                g.drawRect((int)(node.getTotalDist() * xSpreading), (int)(node.getYOffset() - (groupOffset/2)),
                           group_text_size , groupOffset);

                String groupText = node.getGroupName()+" ["+node.getNoOfLeaves()+"]";
                g.drawString(groupText, (int)( node.getTotalDist() * xSpreading) + 5, node.getYOffset() +4 );
                // g.drawString(Integer.toString(node.getNoOfLeaves()), (int)( node.getTotalDist() * xSpreading) + 5, node.getYOffset() +4 );
            }


        }
        else {
//             if (depth <= 0) { // rather obsolete
//                 Toolkit.InternalError("displayTreeGraph: unexpected node reached");
//             }

//             int[] line = new int[4];

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

            //             TreeNode upChild = (TreeNode)(node.getChilds().elementAt(0));
            TreeNode upChild = node.upperSon();
            int[] upline     = new int[4];

            upline[0] = (int)((node.getTotalDist())* xSpreading);
            upline[1] = upChild.getYOffset();
            upline[2] = line[2];
            upline[3] = line[3];

            setColor(upChild.markedState(), g);
            g.drawLine( upline[0], upline[1], upline[2], upline[3]);
            if (newLayout == true) branchLines.put(laf.getLAObject(upline), upChild);

//             displayTreeGraph (g, upChild, depth -1);
            displayTreeGraph (g, upChild);

            // draw lower child

            // TreeNode downChild = (TreeNode)(node.getChilds().elementAt(1));
            TreeNode downChild = node.lowerSon();
            int[] downline     = new int[4];

            downline[0] = upline[0];
            downline[1] = downChild.getYOffset();
            downline[2] = line[2];
            downline[3] = line[3];

            setColor(downChild.markedState(), g);
            g.drawLine( downline[0], downline[1], downline[2], downline[3]);
            if (newLayout == true) branchLines.put(laf.getLAObject(downline), downChild);

//             displayTreeGraph (g, downChild, depth -1);
            displayTreeGraph (g, downChild);

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


    // public void handleMouseClick(int x, int y)
    //     {
    //         System.out.println("handleMouseCLick() was called");
    //         System.out.println("coordinates " + x + "\t" + y );
    //         for (Iterator it = rootSet.iterator(); it.hasNext();)
    //             {
    //                 LineArea la = (LineArea) it.next();
    //                 if (la.isInside(x,y))
    //                     {
    //                         if(((TreeNode) rootLines.get(la)).equals(vs))
    //                             {
    //                                 vs = ((TreeNode) rootLines.get(la)).getFather();
    //                             }
    //                         else
    //                             {
    //                                 vs = (TreeNode) rootLines.get(la);
    //                             }

    //                         newLayout = true;
    //                         //      la.getBorder();
    //                         repaint();
    //                     }

    //             }

    //         for (Iterator it = branchSet.iterator(); it.hasNext();)
    //             {
    //                 LineArea la = (LineArea) it.next();
    //                 if (la.isInside(x,y))
    //                     {
    //                         vs = (TreeNode) branchLines.get(la);
    //                         newLayout = true;
    //                         // la.getBorder();
    //                         repaint();
    //                     }

    //             }

    //     }


    public void handleLeftMouseClick(int x, int y)
    {
        //         System.out.println("handleLeftMouseCLick() was called");
        //         System.out.println("coordinates " + x + "\t" + y );

        TreeNode clickedNode = getClickedNode(x, y);
        if ( clickedNode != null) {
            boolean goto_node = true;

            if (clickedNode.isGroup()) {
                if (clickedNode.isFolded()) {
                    clickedNode.unfold();
//                     System.out.println("unfold");
                    goto_node          = false;
                    lockedPositionNode = clickedNode;
                }
                else {
                    TreeNode foldGroupNode = getBoxClickedNode(x, y);
                    if (foldGroupNode == clickedNode) { // if small box has been clicked -> fold group
                        clickedNode.fold();
//                         System.out.println("fold");
                        goto_node = false;
                        lockedPositionNode = clickedNode;
                    }
                }
            }

            if (goto_node) {
                //                 System.out.println("goto");
                history.push(visibleSubtree);
                setVisibleSubtree(clickedNode.equals(visibleSubtree) ? clickedNode.getFather() : clickedNode);
            }

            newLayout = true;
            repaint();
        }
    }


    public void handleRightMouseClick(int x, int y)
    {
        // System.out.println("right mouse button clicked");
        TreeNode clickedNode = getClickedNode(x, y);
        if(clickedNode != null) {
            // System.out.println("path to clicked node: " + clickedNode.getBinaryPath());
            try {
//                 String codedPath = clickedNode.getCodedPath();
                // System.out.println("path to clicked node: " + codedPath);
//                 myClient.updateNodeInformation(codedPath, clickedNode.getExactMatches()>0);
                myClient.updateNodeInformation(clickedNode);
            }
            catch (ClientException e) {
                myClient.showError(e.getMessage());
            }
            catch (Exception e) {
                myClient.showError("in handleRightMouseClick: "+e.getMessage());
                e.printStackTrace();
            }
            //             System.out.println("returned node information: " + myClient.getNodeInformation(clickedNode.getCodedPath()));
            //             boolean state = clickedNode.markedState() != 0;
            //             clickedNode.markSubtree(!state);
            //             updateListOfMarked();
            //             repaint();
        }
    }


    // methods offer by pull down menus

    public void unmarkNodes()
    {
        if (root != null) {
            root.unmarkSubtree();
            repaint();
        }
    }

    public void gotoRootOfMarked() {
        TreeNode root_of_marked = root.getRootOfMarked();
        if (visibleSubtree != root_of_marked) {
            history.push(visibleSubtree);
            setVisibleSubtree(root_of_marked);
            repaint();            
        }
    }

    public void goUp() {
        if (visibleSubtree != null && visibleSubtree != root) {
            history.push(visibleSubtree);
            setVisibleSubtree(visibleSubtree.getFather());
            repaint();
        }
    }

    public void enterUBr() {
        if (visibleSubtree != null && !visibleSubtree.isLeaf()) {
            history.push(visibleSubtree);
            // setVisibleSubtree((TreeNode)visibleSubtree.getChilds().elementAt(0));
            setVisibleSubtree(visibleSubtree.upperSon());
            repaint();
        }
    }

    public void enterLBr() {
        if (visibleSubtree != null && !visibleSubtree.isLeaf()) {
            history.push(visibleSubtree);
            // setVisibleSubtree((TreeNode)visibleSubtree.getChilds().elementAt(1));
            setVisibleSubtree(visibleSubtree.lowerSon());
            repaint();
        }
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
            myClient.showMessage("History is empty");
        }
    }

    public void countMarkedSpecies () {
        if (root != null) {
            myClient.showMessage("Number of marked species: " + root.countMarked());
            if (visibleSubtree != root) {
                myClient.showMessage("Number of marked species in shown subtree: " + visibleSubtree.countMarked());
            }
        }
        else {
            myClient.showMessage("You have no tree");
        }
    }

//     public int countMarkedRecursive(TreeNode node) {
//         // @@@ FIXME: should be member of class TreeNode
//         return !node.isLeaf()
//             ? countMarkedRecursive( (TreeNode)node.getChilds().elementAt(0)) +  countMarkedRecursive ((TreeNode)node.getChilds().elementAt(1))
//             : (node.markedState() == 2 ? 1 : 0);

//     }

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


    public void updateListOfMarked()
    {
        listOfMarked.clear();
        getLeafNames(root);
    }

    public void getLeafNames(TreeNode node)
    {
        if(node.isLeaf()) {
            listOfMarked.add(node.getShortName());
        }
        else {
            //             getLeafNames((TreeNode)node.getChilds().elementAt(0));
            //             getLeafNames((TreeNode)node.getChilds().elementAt(1));
            getLeafNames(node.upperSon());
            getLeafNames(node.lowerSon());
        }
    }

    // transform coordinates in TreeNode
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
}
