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
    private String  groupName;
    private boolean folded; // whether group is folded

    // probe information
    private int exactMatches;   // number of probes matching exactly
    private int minNonGroupHits; // the probe(s) matching all members of the group, hit(s) minimal that many species outside the group
    private int maxCoverage;    // the maximum coverage reached for this group (range is  0..100)

    private NodeProbes retrievedProbes = null; // when Probes are retrieved, they are stored here

    private double totalDist = 0;
    private String binaryPath;
    private String codedPath;

    private static final char[] hexToken = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    // data structure
    private boolean  isLeaf;
    private TreeNode father;
    //     private Vector   childNodes;
    private TreeNode upperSon;
    private TreeNode lowerSon;
    private int      noOfChildren   = -1; // means: not initialized yet
    //     private boolean  grouped = false;

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
//         childNodes  = new Vector();
        nodeSerial  = nodeCounter;
        father      = null;
        maxCoverage = 100;

        if (verbose) System.out.println("TreeNode:node number " + nodeSerial + " generated");

        nodeCounter++;
    }

    public TreeNode getRoot() {
        TreeNode root = this;
        while (father != root) {
            root = father;
        }
        return root;
    }

    public TreeNode getRootOfMarked() {
        if (markedState() == 0)
            return null;
        
        if (upperSon().markedState() != 0) {
            if (lowerSon().markedState() != 0) {
                return this;
            }
            return upperSon().getRootOfMarked();
        }
        return lowerSon().getRootOfMarked();
    }

    public TreeNode upperSon() { return upperSon; }
    public TreeNode lowerSon() { return lowerSon; }

    public TreeNode getBrother() {
        if (father == this) return null; // root has no brother
        return father.upperSon() == this ? father.lowerSon() : father.upperSon();
        //         int myIdx                 = father.upperSon() == this ? 0 : 1;
        //         return (TreeNode)(father.childNodes.elementAt(1-myIdx));
    }
    public NodeProbes getNodeProbes(boolean exactMatches) throws Exception {
        if (retrievedProbes == null) {
            retrievedProbes = new NodeProbes(this, exactMatches);
        }
        return retrievedProbes;
    }

    public void fold() { folded = true; }
    public void unfold() { folded = false; }
    public boolean isFolded() { return folded; }
    public boolean isGroup() { return groupName != null; }

    public double getTotalDist() { return totalDist; }
    public void setDistance(double d) { distance = d; }
    public double getDistance() { return distance; }

    public int getCurrentXoffset() { return xOffset; }
    public int getCurrentYoffset() { return yOffset; }

    public double calculateTotalDist(double td)
    {
        totalDist = distance + td;
        if (isLeaf == true) return totalDist;

        //         double d1 = ((TreeNode)this.getChilds().elementAt(0)).calculateTotalDist(totalDist);
        //         double d2 = ((TreeNode)this.getChilds().elementAt(1)).calculateTotalDist(totalDist);
        double d1 = upperSon().calculateTotalDist(totalDist);
        double d2 = lowerSon().calculateTotalDist(totalDist);

        if (isFolded()) return totalDist;
        return d1>d2 ? d1 : d2;
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
        folded    = true; // initially all groups are folded
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
            if (upperSon == null) {
                upperSon = tn;
            }
            else {
                if (lowerSon == null) {
                    lowerSon = tn;
                }
                else {
                    throw new Exception("Tried to add 3rd son");
                }
            }

//             childNodes.addElement(tn);
            tn.setFather(this);
        }
    }

    public int getNoOfLeaves()
    {
        if (isLeaf()) {
            noOfChildren = 0;
            return 1;
        }

        if (noOfChildren == -1) {   // not calculated yet
            //             noOfChildren =
            //                 ((TreeNode)this.getChilds().elementAt(0)).getNoOfLeaves() +
            //                 ((TreeNode)this.getChilds().elementAt(1)).getNoOfLeaves();

            noOfChildren = upperSon().getNoOfLeaves()+lowerSon().getNoOfLeaves();
        }

        return noOfChildren;
    }


//     public Vector getChilds() { return childNodes; }

    public boolean isLeaf() { return isLeaf; }
    public void setLeaf(boolean l) { isLeaf = l; }

    public int setXOffset(int x) { xOffset = x; return x; }
    public int setYOffset(int y) { yOffset = y; return y; }

    public int getXOffset(){ return xOffset; }
    public int getYOffset(){ return yOffset; }


    // not used yet

//     public double getMaxDepth()
//     {
//         return isLeaf()
//             ? getTotalDist()
//             : (((TreeNode)getChilds().elementAt(0)).getTotalDist() > ((TreeNode)getChilds().elementAt(1)).getTotalDist()
//                ? ((TreeNode)getChilds().elementAt(0)).getMaxDepth()
//                : ((TreeNode)getChilds().elementAt(1)).getMaxDepth());
//     }

    public boolean setMarked(int marker)
    {
        if (marker == marked) return false;

        marked = marker;
        return true;
    }

    public int markedState() { return marked; }

    private boolean markSubtree_rek(boolean flag) {
        boolean marks_changed = false;

        int new_marked = flag ? 2 : 0;
        if (new_marked != marked) {
            marked        = new_marked;
            marks_changed = true;
        }

        if (!isLeaf) {
            //             marks_changed = ((TreeNode)childNodes.elementAt(0)).markSubtree_rek(flag) || marks_changed;
            //             marks_changed = ((TreeNode)childNodes.elementAt(1)).markSubtree_rek(flag) || marks_changed;

            marks_changed = upperSon().markSubtree_rek(flag) || marks_changed;
            marks_changed = lowerSon().markSubtree_rek(flag) || marks_changed;
        }

        return marks_changed;
    }

    private int recalcMarkedState() {
        // Note: does NOT do a complete recalculation for the whole tree!
        if (isLeaf) {
            return marked;
        }

        //         int upState = ((TreeNode)childNodes.elementAt(0)).recalcMarkedState();
        int upState = upperSon().recalcMarkedState();
        if (upState == 1) { // marked partially -> no need to check other subtree
            return upState;
        }

        int downState = lowerSon().recalcMarkedState();
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

    public int countMarked() {
        switch (markedState()) {
            case 0: return 0;
            case 2: return noOfChildren;
            case 1: break;
        }

        return upperSon().countMarked()+lowerSon().countMarked();
    }

    public boolean unmarkRestOfTree()
    {
        if (father == this) return false; // at root -> no rest

        boolean marks_changed = getBrother().unmarkSubtree();
        marks_changed         = father.unmarkRestOfTree() || marks_changed;

        return marks_changed;
    }

    public boolean markSubtree(boolean unmarkRest)
    {
        boolean marks_changed = markSubtree_rek(true);
        marks_changed         = propagateMarkUpwards() || marks_changed;
        if (unmarkRest) {
            marks_changed = unmarkRestOfTree() || marks_changed;
        }

        return marks_changed;
    }

    public boolean unmarkSubtree()
    {
        boolean marks_changed = markSubtree_rek(false);
        marks_changed         = propagateMarkUpwards() || marks_changed;
        return marks_changed;
    }

    public boolean markSpecies(String speciesList, HashMap hm)
    {
        int     begin         = 0;
        int     komma         = 0;
        int     len           = speciesList.length();
        boolean marks_changed = false;

        while (begin<len) {
            komma                  = speciesList.indexOf(',', begin);
            if (komma == -1) komma = len;
            String speciesName     = speciesList.substring(begin, komma);
            begin                  = komma+1;

            TreeNode ref  = (TreeNode)hm.get(speciesName);
            marks_changed = ref.markSubtree(false) || marks_changed;
            // System.out.println("markSpecies (marked '"+speciesName+"')");
        }

        return marks_changed;
    }


    public void setPath(String path)
    {
        binaryPath = path;
        codedPath  = encodePath(binaryPath);
        if (!isLeaf()) {
            //             ((TreeNode)childNodes.elementAt(0)).setPath(binaryPath + "0");
            //             ((TreeNode)childNodes.elementAt(1)).setPath(binaryPath + "1");

            upperSon().setPath(binaryPath + "0");
            lowerSon().setPath(binaryPath + "1");
        }
    }

    public String getBinaryPath() { return binaryPath; }
    public String getCodedPath() { return codedPath; }

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

    public boolean unfoldAll(boolean recursive) {
        if (!isLeaf()) {
            boolean changed = false;
            if (isGroup() && isFolded()) { changed = true; unfold(); }
            if (recursive || !changed) {
                changed = upperSon.unfoldAll(recursive) || changed;
                changed = lowerSon.unfoldAll(recursive) || changed;
            }
            return changed;
        }
        return false;
    }
    public boolean foldAll(boolean recursive) {
        if (!isLeaf()) {
            boolean changed = false;
            changed = upperSon.foldAll(recursive) || changed;
            changed = lowerSon.foldAll(recursive) || changed;
            if ((recursive || !changed) && isGroup() && !isFolded()) { changed = true; fold(); }
            return changed;
        }
        return false;
    }
    public boolean unfoldCompleteMarked(boolean recursive) {
        if (!isLeaf() && markedState() != 0) {
            boolean changed = false;
            if (isGroup() && isFolded() && markedState() == 2) { changed = true; unfold(); }
            if (recursive || !changed) {
                changed = upperSon.unfoldCompleteMarked(recursive) || changed;
                changed = lowerSon.unfoldCompleteMarked(recursive) || changed;
            }
            return changed;
        }
        return false;
    }
    public boolean foldCompleteMarked(boolean recursive) {
        if (!isLeaf() && markedState() != 0) {
            boolean changed = false;
            changed = upperSon.foldCompleteMarked(recursive) || changed;
            changed = lowerSon.foldCompleteMarked(recursive) || changed;
            if ((recursive || !changed) && isGroup() && !isFolded() && markedState() == 2) { changed = true; fold(); }
            return changed;
        }
        return false;
    }
    public boolean unfoldUnmarked(boolean recursive) {
        if (!isLeaf()) {
            boolean changed = false;
            if (isGroup() && isFolded() && markedState() == 0) { changed = true; unfold(); }
            if (recursive || !changed) {
                changed = upperSon.unfoldUnmarked(recursive) || changed;
                changed = lowerSon.unfoldUnmarked(recursive) || changed;
            }
            return changed;
        }
        return false;
    }
    public boolean foldUnmarked(boolean recursive) {
        if (!isLeaf()) {
            boolean changed = false;
            changed = upperSon.foldUnmarked(recursive) || changed;
            changed = lowerSon.foldUnmarked(recursive) || changed;
            if ((recursive || !changed) && isGroup() && !isFolded() && markedState() == 0) { changed = true; fold(); }
            return changed;
        }
        return false;
    }
    public boolean unfoldPartiallyMarked(boolean recursive) {
        if (!isLeaf() && markedState() == 1) {
            boolean changed = false;
            if (isGroup() && isFolded()) { changed = true; unfold(); }
            if (recursive || !changed) {
                changed = upperSon.unfoldPartiallyMarked(recursive) || changed;
                changed = lowerSon.unfoldPartiallyMarked(recursive) || changed;
            }
            return changed;
        }
        return false;
    }
    public boolean foldPartiallyMarked(boolean recursive) {
        if (!isLeaf() && markedState() == 1) {
            boolean changed = false;
            changed = upperSon.foldPartiallyMarked(recursive) || changed;
            changed = lowerSon.foldPartiallyMarked(recursive) || changed;
            if ((recursive || !changed) && isGroup() && !isFolded()) { changed = true; fold(); }
            return changed;
        }
        return false;
    }

//     public void smartFold() {
//         if (foldUnmarked(false)) {
//             System.out.println("folded unmarked");
//         }
//         else if (foldCompleteMarked(false)) {
//             System.out.println("folded fully marked");
//         }
//         else if (foldPartiallyMarked(false)) {
//             System.out.println("folded partially marked");
//         }
//         else {
//             System.out.println("folded NOTHING");
//         }
//     }
//     public void smartUnfold() {
//         if (unfoldPartiallyMarked(true)) {
//             System.out.println("unfolded partially marked");
//         }
//         else if (unfoldCompleteMarked(true)) {
//             System.out.println("unfolded fully marked");
//         }
//         else if (unfoldUnmarked(true)) {
//             System.out.println("unfolded unmarked");
//         }
//         else {
//             System.out.println("unfolded NOTHING");
//         }
//     }
    public boolean smartFold() {
        return foldUnmarked(false) || foldCompleteMarked(false) || foldPartiallyMarked(false); // step-by-step folding
    }
    public boolean smartUnfold() {
        return unfoldPartiallyMarked(true) || unfoldCompleteMarked(true) || unfoldUnmarked(true); // full-depth unfolding
    }


}// end of class
