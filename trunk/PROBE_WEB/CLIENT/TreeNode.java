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
private double distance = 0;     // branchlength ?
// private StringBuffer nodeName;
// private float        bootstrap;
// private String       emblAccession;

    // ARB Data (valid for leafs only)
private String shortName;
private String fullName;
private String accession_number;

    // ARB Data (valid for internal nodes only)
private String groupName;

    // probe information
private int exactMatches;       // number of probes matching exactly
private int minNonGroupHits;    // the probe(s) matching all members of the group, hit(s) minimal that many species outside the group
private int maxCoverage;        // the maximum coverage reached for this group (range is  0..100)

private double  totalDist = 0;
private String binaryPath;
private String codedPath;
private  static  final char[] hexToken = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    // data structure
private boolean  isLeaf;
private TreeNode father;
private Vector   childNodes;
private int      noOfChildren = 0;
private boolean  grouped      = false;

    // marks
private int  marked = 0;        // 0 = none marked, 1 = has marked, 2 = all marked (1 and 2 only differ for internal nodes)

    // test and internals
public int         level;
public static long nodeCounter = 0;
public long        nodeSerial;
public double      maxDepth    = 0;
public boolean     verbose     = false;



public TreeNode(boolean is_leaf)
{
    isLeaf      = is_leaf;
    childNodes  = new Vector();
    nodeSerial  = nodeCounter;
    father      = null;
    maxCoverage = 100;

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


public TreeNode getRoot() {
    TreeNode root = this;
    while (father != root) {
        root = father;
    }
    return root;
}

public TreeNode getBrother() {
    if (father == this) return null; // root has no brother
    int myIdx = (TreeNode)(father.childNodes.elementAt(0)) == this ? 0 : 1;
    return (TreeNode)(father.childNodes.elementAt(1-myIdx));

}

public void setGrouped(boolean b)
    {
        grouped = b;
    }
public boolean testGrouped()
    {
        return grouped;
    }

public double getTotalDist()
    {
        return totalDist;
    }

public void calculateTotalDist(double td)
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


public void setDistance(double d){
    distance = d;
}

public double getDistance(){
    return distance;
}

public void setSpeciesInfo(String short_name, String full_name, String acc) throws Exception {
    if (!isLeaf) {
        throw new Exception("setSpeciesInfo called for internal node");
    }
    shortName        = short_name;
    fullName         = full_name;
    accession_number = acc;
}
public void setGroupInfo(String group_name) throws Exception {
    if (isLeaf) {
        throw new Exception("setGroupInfo called for leaf");
    }
    groupName = group_name;
}

public void setProbeInfo(int no_of_exact_matches, int min_non_group_hits, int max_coverage) throws Exception {
    if (isLeaf) {
        if (max_coverage != 100) {
            throw new Exception("setProbeInfo with max_coverage!=0 called for leaf");
        }
    }
    exactMatches    = no_of_exact_matches;
    minNonGroupHits = min_non_group_hits;
    maxCoverage     = max_coverage;
}

public String getShortName() { return shortName; }
public String getFullName() { return fullName; }
public String getAccessionNumber() { return accession_number; }
public String getGroupName() { return groupName; }
public int getExactMatches() { return exactMatches; }
public int getMinNonGroupHits() { return minNonGroupHits; }
public int getMaxCoverage() { return maxCoverage; }

public String getDisplayString() { // the string displayed in window
    String result;

    if (getExactMatches() != 0) {
        result = "["+getExactMatches()+"]";
    }
    else if ((getMinNonGroupHits() != 0) || (getMaxCoverage() != 100)) {
        result = "["+getMinNonGroupHits()+"/"+getMaxCoverage()+"%]";
    }
    else {
        result = "";
    }

    if (isLeaf) {
        if (result.length() != 0) {
            result += " ";
        }
        result += getFullName()+", "+getAccessionNumber();
    }
    else {
//         if (groupName != null && groupName.length() != 0) {
//             if (result.length() != 0) {
//                 result += " ";
//             }
//             result += groupName;
//        }
    }

    return result;
}

public void setFather(TreeNode f) throws Exception
    {
        if (father != null) {
            throw new Exception("setFather called twice");
        }
        father = f;
    }

public TreeNode getFather()
    {
      return father;
    }

public void addChild(TreeNode tn) throws Exception
{
    if (isLeaf) {
        throw new Exception("Tried to add a child to a leaf node");
    }
    else {
        childNodes.addElement(tn);
        tn.setFather(this);
    }
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

public double getMaxDepth()
{
    return testLeaf() ? getTotalDist()
        : ((TreeNode)getChilds().elementAt(0)).getTotalDist() > ((TreeNode)getChilds().elementAt(1)).getTotalDist()
        ?  ((TreeNode)getChilds().elementAt(0)).getMaxDepth()
        : ((TreeNode)getChilds().elementAt(1)).getMaxDepth();
}

public boolean setMarked(int marker)
{
    if (marker == marked) return false;

    marked = marker;
    return true;
}

public int isMarked()
{
    return marked;
}

private boolean markSubtree_rek(boolean flag) {
    boolean marks_changed = false;

    int new_marked = flag ? 2 : 0;
    if (new_marked != marked) {
        marked        = new_marked;
        marks_changed = true;
    }

    if (!isLeaf) {
        marks_changed = ((TreeNode)childNodes.elementAt(0)).markSubtree_rek(flag) || marks_changed;
        marks_changed = ((TreeNode)childNodes.elementAt(1)).markSubtree_rek(flag) || marks_changed;
    }

    return marks_changed;
}

private int recalcMarkedState() {
    // Note: does NOT do a complete recalculation for the whole tree!
    if (isLeaf) {
        return marked;
    }

    int upState = ((TreeNode)childNodes.elementAt(0)).recalcMarkedState();
    if (upState == 1) { // marked partially -> no need to check other subtree
        return upState;
    }

    int downState = ((TreeNode)childNodes.elementAt(1)).recalcMarkedState();
    if (downState == 1 || downState == upState) {
        return downState;
    }

    // now one state is 2(=all marked) and the other is 0(=none marked)
    return 1; // partially marked
}

private boolean propagateMarkUpwards() {
    boolean marks_changed = false;
    if (father != this) {       // not at root
        if (marked == 1) {          // partially marked
            marks_changed = father.setMarked(1) || marks_changed;
        }
        else {
            TreeNode brother = getBrother();
            int      bmarked = brother.recalcMarkedState();

            if (bmarked == marked) {
                marks_changed = father.setMarked(marked) || marks_changed;
            }
            else {
                marks_changed = father.setMarked(1) || marks_changed; // partially marked
            }
        }
        marks_changed = father.propagateMarkUpwards() || marks_changed;
    }
    return marks_changed;
}

public boolean unmarkRestOfTree()
{
    if (father == this) return false; // at root -> no rest

    boolean marks_changed = getBrother().unmarkSubtree();
    marks_changed         = father.unmarkRestOfTree() || marks_changed;

    return marks_changed;
}

public boolean markSubtree()
{
    boolean marks_changed = markSubtree_rek(true);
    marks_changed         = propagateMarkUpwards() || marks_changed;
    marks_changed         = unmarkRestOfTree() || marks_changed;

    return marks_changed;
}

public boolean unmarkSubtree()
{
    boolean marks_changed = markSubtree_rek(false);
    marks_changed         = propagateMarkUpwards() || marks_changed;
    return marks_changed;
}

public boolean markSpecies_rek(String speciesList)
{
    boolean marks_changed = false;
    if (isLeaf) {
        if (speciesList.indexOf(","+shortName+",") != -1) {
            marks_changed = setMarked(2);
        }
        else {
            marks_changed = setMarked(0);
        }
        marks_changed = propagateMarkUpwards() || marks_changed;
    }
    else {
        marks_changed = ((TreeNode)childNodes.elementAt(0)).markSpecies_rek(speciesList);
        marks_changed = ((TreeNode)childNodes.elementAt(1)).markSpecies_rek(speciesList) || marks_changed;
    }
    return marks_changed;
}

public boolean markSpecies(String speciesList)
{
    // speciesList contains sth like ",name1,name2,name3,name4,"

    if (father != this) {
        System.out.println("markSpecies has to be called for root!");
        return getRoot().markSpecies(speciesList);
    }

    return markSpecies_rek(speciesList);
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
