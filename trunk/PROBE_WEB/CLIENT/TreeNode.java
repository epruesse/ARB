//package de.arbhome.arb_probe_library;


import java.lang.*;
import java.util.*;
//import de.arbhome.arb_probe_library.*;




public class TreeNode
{
// private float xOffset;
// private float yOffset;

// graphical display
private int xOffset;
private int yOffset;

    // content
private float distance = 0;
private StringBuffer nodeName;
private float totalDist = 0;
private float bootstrap;
private String emblAccession;
private String arbAccession;
private String binaryPath;
private String codedPath;
private  static  final char[] hexToken = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    // data structure
private boolean isLeaf;
private TreeNode father;
private Vector childNodes;
private int noOfChildren = 0;
private boolean grouped = false;
private boolean marked = false;


    // test and internals
public int level;
public static long nodeCounter = 0;
public long nodeSerial;
public float maxDepth = 0;
public boolean verbose = false;



public TreeNode()
{
    nodeName = new StringBuffer();
    childNodes = new Vector();
    nodeSerial = nodeCounter;
    if (verbose) System.out.println("TreeNode:node number " + nodeSerial + " generated");
    nodeCounter++;

}

    // copy constructor
// public TreeNode( TreeNode oldNode)
// {
//     nodeName = new StringBuffer(oldNode.getNodeName());
//     childNodes = new Vector(oldNode.getChilds());
//     nodeSerial = oldNode.nodeCounter;
//     //    System.out.println("TreeNode:node number " + nodeSerial + " generated");
//     //    nodeCounter++;

// }


// public int getNoOfChildren()

//     {
//         if (noOfChildren == 0)
//             {
//                 noOfChildren = getNoOfLeaves();
//             };
//         return noOfChildren;
//     }


public void setGrouped(boolean b)
    {
        grouped = b;
    }
public boolean testGrouped()
    {
        return grouped;
    }

public float getTotalDist()
    {
        return totalDist;
    }

public void calculateTotalDist(float td)
{
    totalDist = distance + td;
    if (isLeaf != true)
        {
            ((TreeNode)this.getChilds().elementAt(0)).calculateTotalDist(totalDist);
            ((TreeNode)this.getChilds().elementAt(1)).calculateTotalDist(totalDist);
        }

}

// public void calculateTotalDist(float td, TreeDisplay dis)
// {
//     totalDist = distance + td;
//     if(totalDist > dis.maxDist) {dis.maxDist = totalDist;};

//     if (isLeaf != true)
//         {
//             ((TreeNode)this.getChilds().elementAt(0)).calculateTotalDist(totalDist);
//             ((TreeNode)this.getChilds().elementAt(1)).calculateTotalDist(totalDist);
//         }

// }


public void setDistance(float d){
    distance = d;
}

public float getDistance(){
    return distance;
}

public void setNodeName(String n)
{
    nodeName = new StringBuffer(n);
}

public String getNodeName()
{
    return nodeName.toString();
}

public void appendNodeName(String app)
{
    nodeName.append(app);

}

public void setFather(TreeNode f)
    {
        father = f;
    }

public TreeNode getFather()
    {
      return father;
    }

public void addChild(TreeNode tn)
    {
        childNodes.addElement(tn);
    }


public int getNoOfLeaves()
    //throws Exception e
{
//      try {
//          return testLeaf() ? 1 : ((TreeNode)this.getChilds().elementAt(0)).getNoOfLeaves()
//              +  ((TreeNode)this.getChilds().elementAt(1)).getNoOfLeaves();
//      }catch (Exception e)
//          {
//              new NodeError(this);
//              System.exit(1);
//              return 0;
//          }

     if (testLeaf())
         {
             noOfChildren = 0;
             //             return 0;
             return 1;
         }
     else {
         //  return noOfChildren = 1 + ((TreeNode)this.getChilds().elementAt(0)).getNoOfLeaves()
  return noOfChildren = ((TreeNode)this.getChilds().elementAt(0)).getNoOfLeaves()
      +  ((TreeNode)this.getChilds().elementAt(1)).getNoOfLeaves();
     }

}


public Vector getChilds()
    {
        return childNodes;
    }

public boolean testLeaf()
{
    return isLeaf;
}
public void setLeaf(boolean l)
{
    isLeaf = l;
}


public void setXOffset(int x){
    xOffset = x;
}

public int setYOffset(int y){
    yOffset = y;
    return y;
}


public int getXOffset(){
    return xOffset;
}
public int getYOffset(){


    return yOffset;
}


// not used yet

public float getMaxDepth()
{
    return testLeaf() ? getTotalDist()
        : ((TreeNode)getChilds().elementAt(0)).getTotalDist() > ((TreeNode)getChilds().elementAt(1)).getTotalDist()
        ?  ((TreeNode)getChilds().elementAt(0)).getMaxDepth()
        : ((TreeNode)getChilds().elementAt(1)).getMaxDepth();
}

public void setMarked(boolean marker)
{
    marked = marker;
}

public boolean isMarked()
{
    return marked;
}

public void markSubtree(boolean flag)
{

    marked = flag;
    if (isLeaf != true)
        {
            ((TreeNode)childNodes.elementAt(0)).markSubtree(flag);
            ((TreeNode)childNodes.elementAt(1)).markSubtree(flag);
        }
}

public void setPath(String path)
{
    binaryPath = path;
    codedPath =  encodePath(binaryPath);
    if(!testLeaf())
        {
            ((TreeNode)childNodes.elementAt(0)).setPath(binaryPath + "0");
            ((TreeNode)childNodes.elementAt(1)).setPath(binaryPath + "1");
        }
}

public String getBinaryPath()
{
    return binaryPath;
}

public String getCodedPath()
{
    return codedPath;
}

// -------------------------------------------------------------------
// NOTE: if you change encodePath() please keep encodePath/decodePath()
//       in ./PROBE_SERVER/WORKER/psw_main.cxx up-to-date

private String encodePath(String path)
{
    StringBuffer coded = new StringBuffer();
    int pathLength = path.length();

    // transform length into two byte hex format
    for (int digit = 0; digit < 4; digit ++)
            {
                int remain = pathLength%16;
                pathLength = pathLength/16;
                coded.insert(0,hexToken[remain]);
            }


    int value = 0;
    int position;
    for (position = 0; position < path.length(); position++ )
            {
                //                System.out.println(path.charAt(position));
                 if (path.charAt(position) == '1')
                     {
                     // value = value + (3 - (position%4))*2;
                         switch (position%4)
                             {
                             case 0:
                                 value += 8;
                                 break;
                             case 1:
                                 value += 4;
                                 break;
                             case 2:
                                 value += 2;
                                 break;
                             case 3:
                                 value += 1;
                                 break;
                             default:
                                 System.out.println("Error in binary string conversion");
                             }

                     };
                 //              System.out.println(value);
                 if ((position%4) == 3)
                     {
                         //                         System.out.println(value);
                         //                         System.out.println(hexToken[value]);
                         coded.append(hexToken[value]);
                         value = 0;
                     }

            }

    //        System.out.println(hexToken[value]);
    if ((position%4) != 0) {
        coded.append(hexToken[value]);
    }
        //                System.out.println("am Ende: " + binary);
    return coded.toString();
}

}// end of class
