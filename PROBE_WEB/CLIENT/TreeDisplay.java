//package de.arbhome.arb_probe_library;

// TreeDisplay.java

import java.lang.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
//import java.Thread.*;

public class TreeDisplay
extends Canvas
{
private long displayCounter;
private String treeString;
private TreeNode root;
private TreeNode vs; // visible subtree
private TreeNode prevNode;
private Stack history;
private TreeSet listOfMarked;

// private int xSize = 800;
// private int ySize = 1000;
private int xSize = 0;
private int ySize = 0;

private int leafNumber;

private HashMap         branchLines; // vertical lines
private HashMap         rootLines; // horizontal lines
private Client          myBoss;
private int             yPosition      = 10;
private int             leafSpace      = 15 ; // (ySpan / leafNumber)
private int             yPointer       = 0; // serves as pointer to the y-space already used up
private int             xSpreading     = 1000;
private int             treeLevels;
private boolean         newLayout      = true;
private LineAreaFactory laf;
private Set             rootSet;
private Set             branchSet;
private Color           dc;
private Color           mc             = Color.yellow;
public float            maxDist        = 0;
private int             clickTolerance = 5;


public TreeDisplay(TreeNode root, int treeLevels)
    {

        setBackground(Color.lightGray);
        // postpone this later until tree is parsed to determined the needed space

        //        setVisible(true);

        displayCounter = 0;

        rootLines = new HashMap();
        branchLines = new HashMap();
        rootSet = new HashSet();
        branchSet = new HashSet();
        history = new Stack();
        listOfMarked = new TreeSet();
        laf = new LineAreaFactory(6);

        this.treeLevels = treeLevels;
        this.root = root;
        if (root == null) {
            Toolkit.AbortWithError("TreeDisplay: no root node obtained from caller");
        }
//         if (root == null)
//             {
//                 System.out.println("tree parsing failed, no root node returned");
//                 System.exit(1);
//             }

//        vs =  clipTree(root, treeLevels);
        vs = root;
            //        root.setDistance((float)0.01);
        vs.setDistance((float)0.01);

        System.out.println("root name: " + vs.getNodeName());
        System.out.println(">>>"+vs.getNodeName()+"<<<");

        System.out.println("tree has " + vs.getNoOfLeaves() + " leaves");
        leafNumber = vs.getNoOfLeaves();
        //        root.calculateTotalDist(0);
        vs.calculateTotalDist(0);
        calculateYValues4Leaves(vs, treeLevels);
        calculateYValues4internal(vs, treeLevels);

        //        setSize(xSize,ySize);
        //        xSize = (int)(root.getMaxDepth()*xSpreading +100);
        xSize = 1000;
        ySize = yPointer + 50;
        setSize(xSize,ySize);
        System.out.println("tree area: x: " + xSize);
        System.out.println("maxDist  : x: " + maxDist);
        System.out.println("tree area: y: " + ySize);
        addMouseListener(
                         new DisplayMouseAdapter(this)

                         );





        //        setVisible(true);
    }


public void paint(Graphics g)
    {

        // rooot changed, layout new tree

        if (dc == null)
            {
                dc = g.getColor();
            }
        // do some cleanup or reset of datastructures before a new layout is generated
        if (newLayout == true)
            {

                yPointer = 0;
                rootLines.clear();
                branchLines.clear();
                rootSet.clear();
                branchSet.clear();
                vs.calculateTotalDist(0);
                calculateYValues4Leaves(vs, treeLevels);
                calculateYValues4internal(vs, treeLevels);
                setSize(xSize,yPointer + 50);

            }

        if (root == null) {
            Toolkit.AbortWithError("in paint: no valid node given to display");
        }


        displayTreeGraph(g, vs, treeLevels);
        // new lines are registered in displayTreeGraph()

        if (rootSet.isEmpty() || branchSet.isEmpty()) // equivalent to newLayout == true except first invocation
            {
                rootSet = rootLines.keySet();
                branchSet = branchLines.keySet();
            }
       newLayout = false;

        System.out.println("method display tree was called "+ ++displayCounter+" times");
    }



public void calculateYValues4Leaves(TreeNode node, int depth)
    {

        if ((node.testLeaf() == true) || (depth == 0))
            {

                 node.setYOffset( yPointer + yPosition);
                 yPointer = yPointer + leafSpace;
                 if (node.testLeaf() == false && depth == 0 )
                     {
                         node.setGrouped(true);
                     }


            }
        else
            {
                node.setGrouped(false);
                calculateYValues4Leaves((TreeNode)(node.getChilds().elementAt(0)), depth -1 );
                calculateYValues4Leaves((TreeNode)(node.getChilds().elementAt(1)), depth -1 );
            }

    }



public int calculateYValues4internal(TreeNode node, int depth)
{
  return  (node.testLeaf() || depth == 0 )?  node.getYOffset()
        :  node.setYOffset((calculateYValues4internal((TreeNode)(node.getChilds().elementAt(0) ), depth -1 )
                            + calculateYValues4internal((TreeNode)(node.getChilds().elementAt(1) ), depth -1 )) /2 );

}




public void displayTreeGraph(Graphics g, TreeNode node, int depth)
    {

        if (node == null) {
            Toolkit.AbortWithError("no valid node given to display");
        }

        if (node.isMarked() == true )
            {
                g.setColor(mc);
            }
        else
            {
                g.setColor(dc);
            }
        if ((node.testLeaf() == true) || (depth == 0))
             {
                 int[] line = new int[4];
                 line[0] = (int)((node.getTotalDist() - node.getDistance())* xSpreading);
                 line[1] = (int) node.getYOffset();
                 line[2] = (int)(node.getTotalDist()*xSpreading);
                 line[3] = (int) node.getYOffset();

                 if (node.isMarked() == true)
                     {
                         g.setColor(Color.yellow);
                     }
                 g.drawLine( line[0],
                             line[1],
                             line[2],
                             line[3]
                             );
                 if (newLayout == true)
                     {
//                          rootLines.put(new Integer((int) node.getYOffset()), new Pair(
//                                                                                 (int)((node.getTotalDist() - node.getDistance())* xSpreading),
//                                                                                 (int)(node.getTotalDist()*xSpreading)
//                                                                                 )
//                                          );

                         rootLines.put(laf.getLAObject(line), node);

                         node.setXOffset(
                                         line[2]
                                         //  (int)(node.getTotalDist()*xSpreading)
                                         );
                     };



                 if (node.testGrouped() == false) // add the node name if node is leaf
                     {
                         g.drawString(node.getNodeName(), (int)( node.getTotalDist() * xSpreading) + 5, node.getYOffset() +3 );
                     }
                 else // draws a box with number of child if node is internal
                     {
                         g.drawRect((int)( node.getTotalDist() * xSpreading),
                                    node.getYOffset() - ( leafSpace/2 - 1 ),
                                    leafSpace * 3 ,
                                    (int)(leafSpace * 0.8) );
                         g.drawString(Integer.toString(node.getNoOfLeaves()), (int)( node.getTotalDist() * xSpreading) + 5, node.getYOffset() +4 );
                     }


             } // end of scope of old line object
         else
             {
                 if(depth > 0){ // rather obsolete


                 int[] line = new int[4];
                 line[0] = (int)((node.getTotalDist() - node.getDistance())* xSpreading);
                 line[1] = node.getYOffset();
                 line[2] = (int)(node.getTotalDist()*xSpreading);
                 line[3] = node.getYOffset();
                 g.drawLine( line[0],
                             line[1],
                             line[2],
                             line[3]
                             );


                 if (newLayout == true)
                     {
//                          branchLines.put(new Integer((int)(node.getYOffset())), new Pair(
//                                                                                 (int)((node.getTotalDist() - node.getDistance())* xSpreading),
//                                                                                 (int)(node.getTotalDist()*xSpreading)
//                                                                                 )
//                                          );


                         rootLines.put(laf.getLAObject(line), node);

                         // node.setXOffset((int)(node.getTotalDist()*xSpreading));
                         node.setXOffset(line[2]);
                     };

                 // vertical branching lines

                 //                 int[]
                 line = new int[4];
                 line[0] = (int)((node.getTotalDist())* xSpreading);
                 line[1] = ((TreeNode)(node.getChilds().elementAt(0))).getYOffset();
                 line[2] = (int)((node.getTotalDist())* xSpreading);
                 line[3] = ((TreeNode)(node.getChilds().elementAt(1))).getYOffset();

                 g.drawLine( line[0],
                             line[1],
                             line[2],
                             line[3]
                             );

                 if (newLayout == true)
                     {
                         branchLines.put(laf.getLAObject(line), node);
                     };

                 displayTreeGraph (g, (TreeNode)(node.getChilds().elementAt(0)), depth -1);
                 displayTreeGraph (g, (TreeNode)(node.getChilds().elementAt(1)), depth -1);
                 }
                 else
                     {
//                          g.drawLine( (int)((node.getTotalDist() - node.getDistance())* xSpreading),
//                                      node.getYOffset(),
//                                      (int)(node.getTotalDist()*xSpreading),
//                                      node.getYOffset()
//                                      );

                         Toolkit.InternalError("displayTreeGraph: unexpected node reached");
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
        System.out.println("handleLeftMouseCLick() was called");
        System.out.println("coordinates " + x + "\t" + y );
        TreeNode clickedNode = getClickedNode(x, y);
        if ( clickedNode != null)
            {
                //                prevNode = vs;
                history.push(vs);
                if(clickedNode.equals(vs))
                    {
                        vs = clickedNode.getFather();
                    }
                else
                    {
                        vs = clickedNode;
                    }
                newLayout = true;
                repaint();
            }
    }


public void handleRightMouseClick(int x, int y)
    {
        System.out.println("right mouse button clicked");
        TreeNode clickedNode = getClickedNode(x, y);
        if(clickedNode != null)
            {
                System.out.println("path to clicked node: " + clickedNode.getBinaryPath());
                System.out.println("path to clicked node: " + clickedNode.getCodedPath());
                System.out.println("returned node information: " + myBoss.getNodeInformation(clickedNode.getCodedPath()));

                boolean state = clickedNode.isMarked();
                clickedNode.markSubtree(!state);
                updateListOfMarked();
                repaint();
            }
    }


    // methods offer by pull down menus

public void unmarkNodes()
    {
        root.markSubtree(false);
        repaint();
    }

public void goDown()
    {
        //        prevNode = vs;
        history.push(vs);

//         System.out.println("TreeDisplay/goDown() was called");
//         System.out.println("current node: " + vs);
//         System.out.println("father node: " + vs.getFather());

        vs = vs.getFather();
        newLayout = true;
        repaint();
    }

public void enterUBr()
    {
        if (!vs.testLeaf())
            {
                //           prevNode = vs;
                history.push(vs);
                vs = (TreeNode)vs.getChilds().elementAt(0);
                newLayout = true;
                repaint();
            }

    }


public void enterLBr()
    {
        if (!vs.testLeaf())
            {
                //                prevNode = vs;
                history.push(vs);
                vs = (TreeNode)vs.getChilds().elementAt(1);
                newLayout = true;
                repaint();
            }

    }

public void resetRoot()
    {
        //        prevNode = vs;
        history.push(vs);
        vs = root;
        newLayout = true;
        repaint();
    }


public void previousRoot()
    {
        if(!history.empty())
            {
                vs = (TreeNode)history.pop();
//         System.out.println("TreeDisplay/goDown() was called");
//         System.out.println("current node: " + vs);
//         System.out.println("father node: " + vs.getFather());

                newLayout = true;
                repaint();
            }
    }


public void countMarkedSpecies ()
    {
        int numberOfMarked = countMarkedRecursive(root);
        System.out.println("Currently number of marked species: " + numberOfMarked);
    }

public int countMarkedRecursive(TreeNode node)
    {

        return  !node.testLeaf() ?
             countMarkedRecursive( (TreeNode)node.getChilds().elementAt(0)) +  countMarkedRecursive ((TreeNode)node.getChilds().elementAt(1))
            : node.isMarked() ? 1 : 0
            ;

    }

public void updateListOfMarked()
    {
        listOfMarked.clear();
        getLeafNames(root);

    }

public void getLeafNames(TreeNode node)
    {
       if(node.testLeaf())
           {
               listOfMarked.add(node.getNodeName());
           }else{
               getLeafNames((TreeNode)node.getChilds().elementAt(0));
               getLeafNames((TreeNode)node.getChilds().elementAt(1));
           }


    }

    // transform coordinates in TreeNode
private TreeNode getClickedNode(int x, int y)
    {

        for (Iterator it = rootSet.iterator(); it.hasNext();)
            {
                LineArea la = (LineArea) it.next();
                if (la.isInside(x,y, clickTolerance))
                    {
                        return (TreeNode) rootLines.get(la);
                    }
            }

        for (Iterator it = branchSet.iterator(); it.hasNext();)
            {
                LineArea la = (LineArea) it.next();
                if (la.isInside(x,y, clickTolerance))
                    {
                        return (TreeNode) branchLines.get(la);
                    }

            }

        if (x > 2*clickTolerance) { // search leftwards for node
            return getClickedNode(x-2*clickTolerance, y);
        }

        return null;
    }
public void setBoss(Client boss)
    {
        myBoss = boss;

    }


}
