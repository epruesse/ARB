//package de.arbhome.allProbes;

import java.util.*;
//import de.arbhome.allProbes.*;
public class NodeError
{
public NodeError(TreeNode tn)
    {
        System.out.println("NodeError:node name: >>>" + tn.getNodeName() + "<<<");
        System.out.println("NodeError:node distance: >>>" + tn.getDistance() + "<<<");
        System.out.println("NodeError:father node: >>>" + tn.getFather() + "<<<");
        System.out.println("NodeError:child node container: >>>" + tn.getChilds() + "<<<");
        System.out.println("NodeError:leaf status: >>>" + tn.testLeaf() + "<<<");
        System.out.println("NodeError:level: >>>" + tn.level + "<<<");
        System.out.println("NodeError:node serial: >>>" + tn.nodeSerial + "<<<");
        //        System.out.println("maxDepth: >>>" + maxDepth + "<<<");
//         System.out.println(": >>>" + tn.getFather() + "<<<");
//         System.out.println(": >>>" + tn.getFather() + "<<<");
//         System.out.println(": >>>" + tn.getFather() + "<<<");




    }
}
