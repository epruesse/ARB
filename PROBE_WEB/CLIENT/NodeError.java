//package de.arbhome.arb_probe_library;

import java.util.*;
//import de.arbhome.arb_probe_library.*;
public class NodeError
{
public NodeError(TreeNode tn)
    {
        System.out.println("NodeError:node information    : '" + tn.getNodeInformation() + "'");
        System.out.println("NodeError:short name          : '" + tn.getShortName() + "'");
        System.out.println("NodeError:full name           : '" + tn.getFullName() + "'");
        System.out.println("NodeError:acc                 : '" + tn.getAccessionNumber() + "'");
        System.out.println("NodeError:exact matches       : " + tn.getExactMatches() );
        System.out.println("NodeError:min. non-group-hits : " + tn.getMinNonGroupHits() );
        System.out.println("NodeError:max. coverage       : " + tn.getMaxCoverage() );
        System.out.println("NodeError:node distance       : " + tn.getDistance() );
        System.out.println("NodeError:father node         : " + tn.getFather() );
        System.out.println("NodeError:child node container: " + tn.getChilds() );
        System.out.println("NodeError:leaf status         : " + tn.testLeaf() );
        System.out.println("NodeError:level               : " + tn.level );
        System.out.println("NodeError:node serial         : " + tn.nodeSerial );
    }
}
