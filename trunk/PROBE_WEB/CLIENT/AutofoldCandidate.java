//  ==================================================================== // 
//                                                                       // 
//    File      : AutofoldCandidate.java                                 // 
//    Purpose   :                                                        // 
//    Time-stamp: <Thu Mar/11/2004 09:48 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

import java.util.*;

public class AutofoldCandidate {
    private TreeNode myNode;
    private int      height       = -1;
    private int      poi_handicap = -1;

    private static LinkedList pointsOfInterrest = new LinkedList();
    private static int        maxPOI            = 10; // max. size of pointsOfInterrest
    
    public static void annouce_POI(TreeNode poi) {
        // marks poi as "point of interest"
        // i.e. autofolding will prefer other TreeNodes

        int found = pointsOfInterrest.indexOf(poi);
        if (found == -1) {
            if (pointsOfInterrest.size() > (maxPOI-1)) pointsOfInterrest.removeLast();
            pointsOfInterrest.addFirst(poi);
        }
        else {
            if (found != 0) {
                pointsOfInterrest.remove(found);
                pointsOfInterrest.addFirst(poi);
            }
        }
        // System.out.println("annouce_POI at '"+poi.getBinaryPath()+"'");
    }

    public int POI_handicap() {
        if (poi_handicap == -1) {
            poi_handicap = 0;

            int size = pointsOfInterrest.size();
            for (int i = 0; i<size; ++i) {
                int malus = size-i+1;

                TreeNode poi = (TreeNode)pointsOfInterrest.get(i);

                if (myNode.equals(poi)) {
                    poi_handicap += 2*malus*malus;
                    // System.out.println("poi_handicap="+poi_handicap+" for "+myNode.getBinaryPath()+" (cause: "+poi.getBinaryPath()+")");
                }
                else if (myNode.contains(poi)) {
                    poi_handicap += malus*malus;
                }
                else if (poi.contains(myNode)) {
                    poi_handicap += malus;
                    // System.out.println("poi_handicap="+poi_handicap+" for "+myNode.getBinaryPath()+" (cause: "+poi.getBinaryPath()+")");
                }
            }
        }
        return poi_handicap;
    }

    private static int min_height = 10; // min. size for candidates
    public static void setMinHeight(int h) {
        min_height = h;
    }

    public AutofoldCandidate(TreeNode node) {
        myNode = node;
    }

    public TreeNode getNode() { return myNode; }

    public int getHeight() {
        if (height == -1) height = myNode.getHeight();
        return height;
    }

    public boolean betterThan(AutofoldCandidate other) {
        if (POI_handicap()<other.POI_handicap()) return true;

        boolean is_group       = getNode().isGroup();
        boolean other_is_group = other.getNode().isGroup();

        if (is_group != other_is_group) return is_group;

        boolean aboveHeight      = getHeight() >= min_height;
        boolean otherAboveHeight = other.getHeight() >= min_height;

        if (aboveHeight != otherAboveHeight) return aboveHeight;
        if (getHeight() < other.getHeight()) return aboveHeight;

        return false;
    }

}
