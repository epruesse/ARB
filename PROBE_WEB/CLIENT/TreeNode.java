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
    private String        groupName;
    private boolean       folded; // whether group is folded
    private static String autofold = "<autofold>";

    // probe information
    private int exactMatches;   // number of probes matching exactly
    private int minNonGroupHits; // the probe(s) matching all members of the group, hit(s) minimal that many species outside the group
    private int maxCoverage;    // the maximum coverage reached for this group (range is  0..100)

    private NodeProbes retrievedProbes = null; // when Probes are retrieved, they are stored here

    private double totalDist = 0;
    private String binaryPath;
    private String codedPath;

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

        if (isLeaf()) {
            return markedState() == 0 ? null : this; // single marked species
        }

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
    public NodeProbes getNodeProbes() throws Exception {
        if (retrievedProbes == null) {
            retrievedProbes = new NodeProbes(this, getExactMatches() != 0);
        }
        return retrievedProbes;
    }

    public void cacheAllHits() throws Exception {
        getNodeProbes().cacheAllHits();
    }

    public void fold() { folded = true; }
    public void unfold() {
        folded = false;
        if (groupName.equals(autofold)) {
            groupName = null;
        }
    }

    // safe version :
    public boolean isFoldedGroup() { return isGroup() && folded; }
    public boolean isUnfoldedGroup() { return isGroup() && !folded; }
    // quick version :
    // public boolean isFoldedGroup() { return folded; }
    // public boolean isUnfoldedGroup() { return !folded; }

    // public boolean isFolded() { return folded; }

    public boolean isAutofolded() { return isGroup() && groupName.equals(autofold); }
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

        double d1 = upperSon().calculateTotalDist(totalDist);
        double d2 = lowerSon().calculateTotalDist(totalDist);

        if (isFoldedGroup()) return totalDist;
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
            // result = "["+getMinNonGroupHits()+"/"+getMaxCoverage()+"%]"; // final version
            result = "["+getMaxCoverage()+"%]";
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

    public void setFather(TreeNode f) throws Exception {
        if (father != null) {
            throw new Exception("setFather called twice");
        }
        father = f;
    }

    public TreeNode getFather() { return father; }

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
            noOfChildren = upperSon().getNoOfLeaves()+lowerSon().getNoOfLeaves();
        }

        return noOfChildren;
    }

    public boolean isLeaf() { return isLeaf; }
    public void setLeaf(boolean l) { isLeaf = l; }

    public int setXOffset(int x) { xOffset = x; return x; }
    public int setYOffset(int y) { yOffset = y; return y; }

    public int getXOffset(){ return xOffset; }
    public int getYOffset(){ return yOffset; }

    public TreeNode lowermostChild() {
        if (isLeaf()) return this;
        return lowerSon().lowermostChild();
    }
    public TreeNode uppermostChild() {
        if (isLeaf()) return this;
        return upperSon().uppermostChild();
    }
    public TreeNode getPreviousLeaf() {
        if (father == this) return null; // at root (called somewhere from uppermost branch)
        if (father.upperSon() == this) return father.getPreviousLeaf();
        return father.upperSon().lowermostChild();
    }

    public int getHeight() {    // returns display height of subtree
        if (isLeaf()) {
            TreeNode previousLeaf = getPreviousLeaf();
            if (previousLeaf == null) { // uppermost leaf
                return getYOffset();
            }
            return getYOffset()-previousLeaf.getYOffset();
        }

        TreeNode lowestChild  = lowermostChild();
        TreeNode uppestChild  = uppermostChild();
        TreeNode previousLeaf = uppestChild.getPreviousLeaf();

        if (previousLeaf == null) { // uppermost subtree
            return lowestChild.getYOffset();
        }

        int upperOffset = previousLeaf.getYOffset();
        int lowerOffset = lowestChild.getYOffset();

        return lowerOffset-upperOffset;
    }


    public boolean setMarked(int marker) {
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

    public boolean markSubtree(boolean unmarkRest) {
        // also used to mark single species

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
        }

        return marks_changed;
    }

    public int searchAndMark(String text, boolean ignoreCase) {
        // if ignoreCase == true, text has to be in lower case!
        if (isLeaf()) {
            boolean found = false;
            String  search;

            search = ignoreCase ? getFullName().toLowerCase() : getFullName();
            if (search.indexOf(text)!= -1) {
                found = true;
            }
            if (!found) {
                search = ignoreCase ? getAccessionNumber().toLowerCase() : getAccessionNumber();
                if (search.indexOf(text) != -1) {
                    found = true;
                }
            }

            if (found) {
                if (marked == 0) markSubtree(false);
                return 1;
            }
            if (marked == 2) unmarkSubtree();
            return 0;
        }
        return
            upperSon().searchAndMark(text, ignoreCase) +
            lowerSon().searchAndMark(text, ignoreCase);
    }

    public void setPath(String path) {
        binaryPath = path;
        codedPath  = Toolkit.encodePath(binaryPath);
        if (!isLeaf()) {
            upperSon().setPath(binaryPath + "0");
            lowerSon().setPath(binaryPath + "1");
        }
    }

    public String getBinaryPath() { return binaryPath; }
    public String getCodedPath() { return codedPath; }

    public boolean unfoldAll(boolean recursive) {
        if (!isLeaf()) {
            boolean changed = false;
            if (isFoldedGroup()) { changed = true; unfold(); }
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
            if ((recursive || !changed) && isUnfoldedGroup()) { changed = true; fold(); }
            return changed;
        }
        return false;
    }
    public boolean unfoldCompleteMarked(boolean recursive) {
        if (!isLeaf() && markedState() != 0) {
            boolean changed = false;
            if (isFoldedGroup() && markedState() == 2) { changed = true; unfold(); }
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
            if ((recursive || !changed) && isUnfoldedGroup() && markedState() == 2) { changed = true; fold(); }
            return changed;
        }
        return false;
    }
    public boolean unfoldUnmarked(boolean recursive) {
        if (!isLeaf()) {
            boolean changed = false;
            if (isFoldedGroup() && markedState() == 0) { changed = true; unfold(); }
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
            if ((recursive || !changed) && isUnfoldedGroup() && markedState() == 0) { changed = true; fold(); }
            return changed;
        }
        return false;
    }
    public boolean unfoldPartiallyMarked(boolean recursive) {
        if (!isLeaf() && markedState() == 1) {
            boolean changed = false;
            if (isFoldedGroup()) { changed = true; unfold(); }
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
            if ((recursive || !changed) && isUnfoldedGroup()) { changed = true; fold(); }
            return changed;
        }
        return false;
    }

    public boolean smartFold() {
        return foldUnmarked(false) || foldCompleteMarked(false) || foldPartiallyMarked(false); // step-by-step folding
    }
    public boolean smartUnfold() {
        return unfoldPartiallyMarked(true) || unfoldCompleteMarked(true) || unfoldUnmarked(true); // full-depth unfolding
    }


    public AutofoldCandidate findFoldCandidate(int wanted_reduce) {
        if (isLeaf()) return null;
        if (isFoldedGroup()) return null;

        AutofoldCandidate best = new AutofoldCandidate(this);
        // System.out.println("height="+getHeight()+" ("+best.getNode().getBinaryPath()+")");
        // if (best.getHeight() < wanted_reduce) return null; // subtree is too small to be candidate


        {
            AutofoldCandidate bestUpper = upperSon().findFoldCandidate(wanted_reduce);
            if (bestUpper != null && bestUpper.betterThan(best)) {
                // System.out.println("bestUpper("+bestUpper.getNode().getBinaryPath()+") betterThan best("+best.getNode().getBinaryPath()+")");
                best = bestUpper;
            }
        }
        {
            AutofoldCandidate bestLower = lowerSon().findFoldCandidate(wanted_reduce);
            if (bestLower != null && bestLower.betterThan(best)) {
                // System.out.println("bestLower("+bestLower.getNode().getBinaryPath()+") betterThan best("+best.getNode().getBinaryPath()+")");
                best = bestLower;
            }
        }

        return best;
    }

    public void autofold(int wanted_reduce) {
        System.out.println("autofolding.. wanted_reduce="+wanted_reduce);

        AutofoldCandidate.setMinHeight(wanted_reduce);
        AutofoldCandidate candidate = findFoldCandidate(wanted_reduce);

        if (candidate != null) {
            TreeNode node2fold = candidate.getNode();
            System.out.println("autofolding "+node2fold.getBinaryPath());

            if (node2fold.isGroup()) {
                Toolkit.showMessage("Autofolding group '"+node2fold.getGroupName()+"'");
                node2fold.fold();
            }
            else {
                Toolkit.showMessage("Autofolding non-group '"+node2fold.getBinaryPath()+"'");
                node2fold.groupName = autofold;
                node2fold.fold();
            }
        }
    }
    public void autounfold() {
        if (!isLeaf()) {
            if (isAutofolded()) groupName = null;
            upperSon().autounfold();
            lowerSon().autounfold();
        }
    }

    public boolean hasAnchestor(TreeNode anchestor) {
        if (father == this) return false;
        if (father == anchestor) return true;
        return father.hasAnchestor(anchestor);
    }

    public boolean contains(TreeNode child) {
        return child.hasAnchestor(this);
    }

    public void storeFolding(StringBuffer buf) {
        if (!isLeaf()) {
            if (isGroup() && !isAutofolded()) {
                buf.append(isFoldedGroup() ? '1' : '0');
            }
            upperSon().storeFolding(buf);
            lowerSon().storeFolding(buf);
        }
    }
    public int restoreFolding(String folding, int pos) {
        if (!isLeaf()) {
            if (isGroup() && !isAutofolded()) {
                char c = folding.charAt(pos);
                ++pos;
                if (c == '1') {
                    if (isUnfoldedGroup()) fold();
                }
                else {
                    if (isFoldedGroup()) unfold();
                }
            }
            pos = upperSon().restoreFolding(folding, pos);
            pos = lowerSon().restoreFolding(folding, pos);
        }
        return pos;
    }

    public TreeNode findNodeByBinaryPath(String binaryPath, int pos) {
        if (pos == binaryPath.length()) return this;
        if (isLeaf()) return null;

        if (binaryPath.charAt(pos) == '0') {
            return upperSon().findNodeByBinaryPath(binaryPath, pos+1);
        }
        else {
            return lowerSon().findNodeByBinaryPath(binaryPath, pos+1);
        }
    }

}// end of class
