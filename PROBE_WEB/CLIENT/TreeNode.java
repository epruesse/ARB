//package de.arbhome.allProbes;


import java.lang.*;
import java.util.*;
//import de.arbhome.allProbes.*;




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

    // data structure 
private boolean isLeaf;
private TreeNode father;
private Vector childNodes;
private int noOfChildren = 0;
private boolean grouped = false;

    // test and internals
public int level;
public static long nodeCounter = 0;
public long nodeSerial;
public float maxDepth = 0;



public TreeNode()
{
    nodeName = new StringBuffer();
    childNodes = new Vector();
    nodeSerial = nodeCounter;
    System.out.println("TreeNode:node number " + nodeSerial + " generated"); 
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
             return 0;
         }
     else {
  return noOfChildren = 1 + ((TreeNode)this.getChilds().elementAt(0)).getNoOfLeaves() 
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


}
