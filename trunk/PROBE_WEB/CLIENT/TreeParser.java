
// test class to develop the tree parsing method

import java.io.*;
import java.util.*;

public class TreeParser
{
private String   treeString;
private int      parse_position;
private int      parse_max_position;
private String   parse_error;
private TreeNode root;
public long      numOpen  = 0;
public long      numClose = 0;
public boolean   verbose  = false;

public TreeParser(String ts)
    {
        treeString = ts;
    }


public TreeNode getRootNode()
    {
        if (root == null)
        {
            try{
                root = generateTreeNodes();
                root.setFather(root);

                return root;
            }
            catch (Exception e)
            {
                System.out.println("Exception occurred: "+e.getMessage());
                System.out.println("opening paren's : >>>" + numOpen + "<<<");
                System.out.println("closing paren's : >>>" + numClose + "<<<");
                return null;
            }


        }
        else
        {
            return root;
        }
    }

private char next_char() {
    if (parse_position >= parse_max_position) {
        return 0;
    }

    char c = treeString.charAt(parse_position);
    parse_position++;

    if (c == ' ' || c == '\n') {
        return next_char();
    }

    return c;
}

private String parseString() {
    char c = next_char();
    if (c != '\'') {
        parse_error = "Expected \"'\" got \""+c+"\"";
        return null;
    }

    int close = treeString.indexOf('\'', parse_position+1);
    if (close == -1) {
        parse_error = "Did not find \"'\" at end of string";
        return null;
    }

    String result  = treeString.substring(parse_position, close);
    parse_position = close+1;

    return result;
}

private double parseDouble() {
    char c = next_char();
    int  p = parse_position;

    while ((c >= '0' && c <= '9') || c == '.') {
        p++;
        c = treeString.charAt(p);
    }

    if (p == parse_position) {
        parse_error = "Expected floating point value";
        return -1.0;
    }

    String num_string = treeString.substring(parse_position, p);
    parse_position    = p;
    double d          = Double.parseDouble(num_string);
    // System.out.println("d="+d);
    return d;
}

private String decodedNodeInfo(String encoded) throws Exception {
    int uscore = encoded.indexOf('_'); // find esc char
    if (uscore != -1) {
        char enc = encoded.charAt(uscore+1);
        char dec = 0;
        if (enc == '_') {
            dec = '_';
        }
        else if (enc == '.') {
            dec = '\n';
        }
        else if (enc == 's') {
            dec = ';';
        }
        else if (enc == 'c') {
            dec = ':';
        }
        else if (enc == 'q') {
            dec = '\'';
        }
        else {
            throw new Exception("Illegal escape sequence '_"+enc+"'");
        }

        return encoded.substring(0, uscore)+dec+decodedNodeInfo(encoded.substring(uscore+2));
    }
    return encoded;
}

private void parseProbeInfo(TreeNode node, ServerAnswer parsed_info) throws Exception {
    int exactMatches       = 0;
    int min_non_group_hits = 0;
    int max_coverage       = 100;

    if (parsed_info.hasKey("em")) { // no of exact matches
        exactMatches = Integer.parseInt(parsed_info.getValue("em"));
    }
    if (parsed_info.hasKey("ng")) { // non-group-hits
        min_non_group_hits = Integer.parseInt(parsed_info.getValue("ng"));
    }
    if (parsed_info.hasKey("mc")) { // max coverage
        max_coverage = Integer.parseInt(parsed_info.getValue("mc"));
    }

    node.setProbeInfo(exactMatches, min_non_group_hits, max_coverage);
}

private void handleInnerNodeInformation(TreeNode node, String nodeInfo) {
    String error = null;
    try {
        String decodedNodeInfo = decodedNodeInfo(nodeInfo);
        // System.out.println("handleInnerNodeInformation '"+nodeInfo+"'");
        // System.out.println("handleInnerNodeInformation '"+decodedNodeInfo+"'");

        ServerAnswer parsed_info = new ServerAnswer(decodedNodeInfo, false, false);
        if (parsed_info.hasError()) {
            error = parsed_info.getError();
        }
        else {
            if (parsed_info.hasKey("g")) { // group name
                node.setGroupInfo(parsed_info.getValue("g"));
            }
            parseProbeInfo(node, parsed_info);
        }
    }
    catch (Exception e) {
        error = e.getMessage();
    }

    if (error != null) {
        Toolkit.AbortWithError("Cannot parse node info '"+nodeInfo+"' (reason: "+error+")");
    }
}

private void handleLeafNodeInformation(TreeNode node, String nodeInfo) {
    String error = null;

    try {
        String       decodedNodeInfo = decodedNodeInfo(nodeInfo);
        ServerAnswer parsed_info     = new ServerAnswer(decodedNodeInfo, false, false);

        if (parsed_info.hasError())        {
            error = parsed_info.getError();
        }
        else if (!parsed_info.hasKey("n")) error = "No short name provided";
        else if (!parsed_info.hasKey("f")) error = "No full name provided";
        else if (!parsed_info.hasKey("a")) error = "No accession number provided";
        else {
            node.setSpeciesInfo(parsed_info.getValue("n"), parsed_info.getValue("f"), parsed_info.getValue("a"));
            parseProbeInfo(node, parsed_info);
        }
    }
    catch (Exception e) {
        error = e.getMessage();
    }

    if (error != null) {
        Toolkit.AbortWithError("Cannot parse node info '"+nodeInfo+"' (reason: "+error+")");
    }
}

private TreeNode parseNode() throws Exception {
    char c             = next_char();
    char expected_char = 0;

    if (c == '(') {
        TreeNode first = parseNode();
        if (first == null) return null;

        c = next_char();
        if (c == ',') {
            TreeNode second = parseNode();
            if (second == null) return null;

            c = next_char();
            if (c == ')') {
                // both child nodes parsed
                TreeNode me = new TreeNode(false);
                me.addChild(first);
                me.addChild(second);

                c = next_char();
                if (c == '\'') { // optional node info
                    parse_position--;
                    String nodeInfo = parseString();
                    if (nodeInfo == null) return null;

                    handleInnerNodeInformation(me, nodeInfo);
                    if (parse_error != null) return null;

                    c = next_char();
                }

                if (c == ':') {
                    // System.out.println("parsing double");
                    double dist = parseDouble();
                    if (parse_error != null) return null;
                    me.setDistance(dist);
                }
                else {
                    parse_position--;
                    me.setDistance(1.0);
                }

                return me;
            }
            else {
                expected_char = ')';
            }
        }
    }
    else if (c == '\'') {
        parse_position--;
        String nodeInfo = parseString();
        if (nodeInfo == null) return null;

        TreeNode leaf = new TreeNode(true);
        handleLeafNodeInformation(leaf, nodeInfo);
        if (parse_error != null) return null;

        c = next_char();
        if (c == ':') {
            // System.out.println("parsing double");
            double dist = parseDouble();
            if (parse_error != null) return null;
            leaf.setDistance(dist);
        }
        else {
            parse_position--;
            leaf.setDistance(1.0);
        }

        return leaf;
    }

    if (expected_char != 0) {
        parse_error = "Expected '"+expected_char+"' got '"+c+"'";
    }
    else {
        parse_error = "Unexpected character '"+c+"'";
    }
    return null;
}

    // make abstract later on to support different tree formats in derived classes
private TreeNode generateTreeNodes() {
    parse_position     = 0;
    parse_max_position = treeString.length();
    parse_error        = null;

    if (( parse_max_position == 0) || (treeString.charAt(0)!= '(') ) {
        Toolkit.AbortWithError("generateTreeNodes: Empty tree string or wrong format");
    }

    TreeNode anchor = null;
    try {
        anchor = parseNode();
    }
    catch (Exception e) {
        parse_error = e.getMessage();
    }

    if (anchor == null) {
        System.out.println("got an error at '"+treeString.substring(parse_position, parse_position+5)+"'");
        System.out.println("at '"+treeString.substring(parse_position-40, parse_position+40)+"'");
        Toolkit.AbortWithError("generateTreeNodes: Error parsing tree ("+parse_error+")");
    }

    return anchor;
}

// private TreeNode generateTreeNodes_old(String ts)
// {

//     if (( ts.length() == 0) || (ts.charAt(0)!= '(') ) {
//         Toolkit.AbortWithError("generateTreeNodes: Empty tree string or wrong format");
//     }

//     TreeNode anchor      = new TreeNode();
//     //   r.setFather(r);
//     TreeNode currentNode = anchor;
//     String   lastToken   = new String("(");
// //     anchor.setNodeName("anchor");
//     anchor.level         = 0;

//     StringTokenizer strTok =
//         new StringTokenizer(ts,
//                             "'():, \n\t",
//                             true); // returns also the separators

//     int indent = 0 ;
//     //    System.out.println("indent: " + indent);

//     boolean insideString = false;

//     while(strTok.hasMoreTokens())
//         {
//             String token = strTok.nextToken();

// //             if (token.equals("'"))
// //                 {
// //                     insideString = insideString ? false : true;
// //                 };
// //             if (insideString) {
// //                 currentNode.appendNodeName(token);
// //                 continue;
// //             };




//             while ( ( (token.equals(" ") || token.equals("\n") || token.equals("\t") ) )
//                     && strTok.hasMoreTokens()  )
//                 {
//                     token = strTok.nextToken();
//                 }

// //             if (verbose)
//                 System.out.println("current token is :>>>" + token + "<<<");

//             if (insideString == true && !token.equals("'"))
//                     {
//                         System.out.println("handle token 1 :>>>" + token + "<<<");

//                         // @@@ FIXME: parse the node info..
// //                         currentNode.appendNodeName(token);
//                         lastToken = token;
//                         continue;
//                     }

//             if (insideString == true && token.equals("'"))
//                     {
//                         insideString = false;
//                         lastToken = token;
//                         continue;
//                     }

//             if (lastToken.equals("'")  && token.equals("'"))
//                     {
//                         insideString = true;
//                         lastToken = token;
//                         continue;
//                     }


// //             if (lastToken.equals("'")){
// //                 while(!token.equals("'"))
// //                 {
// //                     currentNode.appendNodeName(token);
// //                     lastToken = token;
// //                     token = strTok.nextToken();
// //                 }
// //             }


//             if(token.equals("(")) {
// //                     System.out.println("recognized token '(': >>>" + token + "<<<");
//                      ++ numOpen;
//                      lastToken = token;
//                      TreeNode tmp = new TreeNode();
// //                      if (currentNode.getNodeName().equals("anchor")){
// //                          tmp.setNodeName("root");
// //                      };
//                      tmp.setFather(currentNode);
//                      currentNode.addChild(tmp);
//                      currentNode = tmp;
//                      ++indent;
//                      currentNode.level = indent;
// //                     System.out.println("indent: " + indent);
// //                     //                    continue;
//                 };

//                 // token is identifier terminal node
//                 if ( (lastToken.equals("(") || lastToken.equals(",")  )
//                      && ( !(token.equals("(")
//                             //|| token.equals(")")
//                             ) ) )
//                     {
//                         //                 if(isIdentifier(token))//token is a identifier -> new named node
//                         //                     {
//                         //                     System.out.println("recognized identifier: >>>" + token + "<<<");
//                         TreeNode tmp = new TreeNode();

//                         //                     // this is maybe not necessary inside the tree
// //                         if (currentNode.getNodeName().equals("anchor")){
// //                             tmp.setNodeName("root");
// //                         };
//                         if (token.equals("'"))
//                         {
//                             insideString = true;
//                         };

//                         tmp.setFather(currentNode);
//                         currentNode.addChild(tmp);


// //                         while ( ( (token.equals(" ") || token.equals("\n") || token.equals("\t") ) )
// //                                 && strTok.hasMoreTokens()  )
// //                             {
// //                                 token = strTok.nextToken();
// //                             }

// //                         if (token.equals("'"))
// //                             {
// //                                     token = strTok.nextToken();
// //                                     while (!token.equals("'"))
// //                                         {
// //                                             tmp.appendNodeName(token);
// //                                             token = strTok.nextToken();
// //                                         }

// //                             }
// //                         else
// //                             {
//                         System.out.println("handle token 2 :>>>" + token + "<<<");
// //                                  tmp.setNodeName(token);
// //                             };


// //                         while (!token.equals(":"))
// //                         {
// //                             tmp.appendNodeName(token);
// //                             token = strTok.nextToken();
// //                         }

//                         currentNode = tmp; // ???
// //                        lastToken = token;
//                         lastToken = token;
//                             //                     ++indent;
//                             //                     System.out.println("indent: " + indent);

//                             //                     currentNode.level = indent;
//                             //                     //                    continue;
//                     };

// //                 if (lastToken.equals("'") && token.equals("'"))
// //                     {
// //                                     token = strTok.nextToken();

// //                                     while (!token.equals("'"))
// //                                         {
// //                                             currentNode.appendNodeName(token);
// //                                             token = strTok.nextToken();
// //                                         }


// //                     };


//                 // token is identifier for internal node;; strange

//                 if ( (lastToken.equals(")")
//                       // || lastToken.equals(",")
//                       ) // || lastToken.equals("'")  )
//                      && (
//                          ! (
//                             //token.equals("(") ||
//                             token.equals(":") ) ) )
//                     {

//                         if (verbose) System.out.println("found internal node name : " + token);
//                         //                 if(isIdentifier(token))//token is a identifier -> new named node
//                         //                     {
//                         //                     System.out.println("recognized identifier: >>>" + token + "<<<");


// //                         while ( ( (token.equals(" ") || token.equals("\n") || token.equals("\t") ) )
// //                                 && strTok.hasMoreTokens()  )
// //                             {
// //                                 token = strTok.nextToken();
// //                             }


// //                         if (token.equals("'"))
// //                             {

// //                                     token = strTok.nextToken();
// //                                     while (!token.equals("'"))
// //                                         {
// //                                             currentNode.appendNodeName(token);
// //                                             token = strTok.nextToken();
// //                                         }

// // //                                         System.out.println("NodeName: " + currentNode.getNodeName());
// // //                                         System.exit(1);
// //                             }
// //                         else
// //                             {
//                         System.out.println("handle token 3 :>>>" + token + "<<<");
// //                                  currentNode.setNodeName(token);
// //                             };
//                                  if (token.equals("'"))
//                                      {
//                                          insideString = true;
//                                      };



//                         //                        lastToken = token;

// //                         while (!token.equals(":"))

// //                         {

// //                             currentNode.appendNodeName(token);
// //                             token = strTok.nextToken();

// //                         }
//                         lastToken = token; //token;
//                         ++indent;
//                             //                     System.out.println("indent: " + indent);

//                         currentNode.level = indent;
//                             //                     //                    continue;
//                     };


//                  if(token.equals(":")) {

//                      token = strTok.nextToken();
//                      currentNode.setDistance((float)Float.parseFloat(token));
//                      lastToken = new String("dist");

// //                     System.out.println("recognized token ':' >>>" + token + "<<<");
// //                     System.out.println("do nothing");
// //                     //                    continue;
//                  };


// //                  if(lastToken.equals(":")) {

// //                      //                     token = strTok.nextToken();
// //                      currentNode.setDistance((float)Float.parseFloat(token));
// //                      lastToken = new String("dist");

// // //                     System.out.println("recognized token ':' >>>" + token + "<<<");
// // //                     System.out.println("do nothing");
// // //                     //                    continue;
// //                  };



//                  if(token.equals(")") ){
//                      if (!lastToken.equals("dist"))
//                          {
//                              Toolkit.AbortWithError("generateTreeNodes: missing distance");
//                          };
// //                     System.out.println("recognized tokens: ')' >>>" + token + "<<<");
//                      ++ numClose;
//                      if (currentNode.getChilds().size() == 0)
//                          {
//                              currentNode.setLeaf(true);

// //                             System.out.println("nodeName:>>>"+currentNode.getNodeName()+"<<<");
//                          }
//                      else{currentNode.setLeaf(false);}
//                      currentNode = currentNode.getFather();
//                      lastToken = token;
// //                     System.out.println("went down to nodeName:>>>"+currentNode.getNodeName()+"<<<");

//                      --indent;
// //                     System.out.println("indent: " + indent);
// //                     //                    continue;
//                  }

//                  if(token.equals(",") ){

//                      if (!lastToken.equals("dist"))
//                          {
//                              Toolkit.AbortWithError("generateTreeNodes: missing distance, outside NodeName");
//                          };

// //                     System.out.println("recognized token: ',' >>>" + token + "<<<");
//                      if (currentNode.getChilds().size() == 0)
//                          {
//                              currentNode.setLeaf(true);
// //                             System.out.println("nodeName:>>>"+currentNode.getNodeName()+"<<<");
//                          }
//                      else{currentNode.setLeaf(false);}
//                      currentNode = currentNode.getFather();
// //                     System.out.println("went down to nodeName:>>>"+currentNode.getNodeName()+"<<<");

//                      --indent;
// //                     System.out.println("indent: " + indent);
// //                     //                    continue;

//                      lastToken = token;
//                  };






// //                 // formating for test and debugging
// //                 if(indent < 0)
// //                     { System.out.println("ParsingError"); System.exit(1);}

// //                 String indentString = new String();
// //                 for (int i = indent; i > 0; --i)
// //                     {
// //                         if (i==0){}
// //                         else{indentString = indentString.concat("-");}
// //                     }
// //                 //                System.out.println(indentString + token);
//             }

//         // stops at last node before root because root node has no distance
//         //      System.out.println("anchor hat " + anchor.getChilds().size() + "Kinder");


//         return (TreeNode) anchor.getChilds().elementAt(0);
// }



// private boolean isIdentifier(String s)
//     // returns true if s is an identifier else returns false
// {
//     int l = s.length();
//     boolean status = true;
//     if ((s.charAt(0) >= '0') && (s.charAt(0) <= '9'))
//         {
//             status = false;
//             //            System.out.println("not an identifier: " + s);
//         }
//     for (int i = 0; (i < l) && (status == true); ++i)
//         {
//             char c = s.charAt(i);
//             if (! ( ( (c >= 'A') && (c <= 'Z') ) || ((c >= 'a') && (c <= 'z'))
//                     || ( c == '_') || ((c >= '0') && (c <= '9')) || (c == '#') // ) )
//                                     || (c == '.') || (c == '\'') ) )
//                 {
//                     status = false;
//                     //                    System.out.println("not an identifier: " + s);
//                 }
//         }
//     return status;

// }

// private boolean isDistance(String s)
//     // checks whether a string represents a float between zero and one
// {
//     boolean status = false;

//     float distance = 0;
//     try{
//         distance = Float.parseFloat(s);
//     } catch (NumberFormatException e)
//         {
//             distance = -1;
//             //            System.out.println("token was not a distance value");
//         }
//     if ( (distance > 0) && (distance < 1) )
//         {
//             status = true;
//         }
//     return status;

// }



}
