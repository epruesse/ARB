
// test class to develop the tree parsing method

import java.io.*;
import java.util.*;

public class TreeParser
{
private String treeString;
private TreeNode root;
public long numOpen = 0;
public long numClose = 0;

public TreeParser(String ts)
    {
        treeString = ts;
    }


public TreeNode getRootNode()
    {
        if (root == null)
            {
                try{ 
                    root = generateTreeNodes(treeString);
               return root;
                } 
                catch (Exception e)
                    {
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


    // make abstract later on to support different tree formats in derived classes
private TreeNode generateTreeNodes(String ts)
{

    if (( ts.length() == 0) || (ts.charAt(0)!= '(') )
        {
            System.out.println("Error in function TreeNode.generateTreeNodes()");
            System.out.println("Empty tree string or wrong format");
            System.exit(1);
        }
    TreeNode anchor = new TreeNode();
    //   r.setFather(r);
    TreeNode currentNode = anchor;
    String lastToken = new String("(");
    anchor.setNodeName("anchor");
    anchor.level = 0;

    StringTokenizer strTok = 
        new StringTokenizer(ts,
                            "'():, \n\t", 
                            true); // returns also the separators

    int indent = 0 ;
    //    System.out.println("indent: " + indent);

    boolean insideString = false;

    while(strTok.hasMoreTokens())
        {
            String token = strTok.nextToken();

//             if (token.equals("'"))
//                 {
//                     insideString = insideString ? false : true;
//                 };
//             if (insideString) {
//                 currentNode.appendNodeName(token);
//                 continue;
//             };
            
            


            while ( ( (token.equals(" ") || token.equals("\n") || token.equals("\t") ) )
                    && strTok.hasMoreTokens()  )
                {
                    token = strTok.nextToken();
                }
            System.out.println("actual token is :>>>" + token + "<<<");

            if (insideString == true && !token.equals("'"))
                    {
                        currentNode.appendNodeName(token);
                        lastToken = token;
                        continue;
                    }

            if (insideString == true && token.equals("'"))
                    {
                        insideString = false;
                        lastToken = token;
                        continue;
                    }

            if (lastToken.equals("'")  && token.equals("'"))
                    {
                        insideString = true;
                        lastToken = token;
                        continue;
                    }


//             if (lastToken.equals("'")){
//                 while(!token.equals("'"))
//                 {
//                     currentNode.appendNodeName(token);
//                     lastToken = token;
//                     token = strTok.nextToken();
//                 }
//             }

            
            if(token.equals("(")) {
//                     System.out.println("recognized token '(': >>>" + token + "<<<");
                     ++ numOpen; 
                     lastToken = token;
                     TreeNode tmp = new TreeNode();
                     if (currentNode.getNodeName().equals("anchor")){
                         tmp.setNodeName("root");
                     };
                     tmp.setFather(currentNode);
                     currentNode.addChild(tmp);
                     currentNode = tmp;
                     ++indent;
                     currentNode.level = indent;
//                     System.out.println("indent: " + indent);
//                     //                    continue;
                };

                // token is identifier terminal node
                if ( (lastToken.equals("(") || lastToken.equals(",")  ) 
                     && ( !(token.equals("(") 
                            //|| token.equals(")")
                            ) ) )
                    {
                        //                 if(isIdentifier(token))//token is a identifier -> new named node
                        //                     {
                        //                     System.out.println("recognized identifier: >>>" + token + "<<<");
                        TreeNode tmp = new TreeNode();

                        //                     // this is maybe not necessary inside the tree
                        if (currentNode.getNodeName().equals("anchor")){
                            tmp.setNodeName("root");
                        };
                        if (token.equals("'"))
                        {
                            insideString = true;
                        };

                        tmp.setFather(currentNode);
                        currentNode.addChild(tmp);


//                         while ( ( (token.equals(" ") || token.equals("\n") || token.equals("\t") ) )
//                                 && strTok.hasMoreTokens()  )
//                             {
//                                 token = strTok.nextToken();
//                             }

//                         if (token.equals("'"))
//                             {
//                                     token = strTok.nextToken();
//                                     while (!token.equals("'"))
//                                         {
//                                             tmp.appendNodeName(token);
//                                             token = strTok.nextToken();
//                                         }

//                             }
//                         else
//                             {
                                 tmp.setNodeName(token);
//                             };


//                         while (!token.equals(":"))
//                         {
//                             tmp.appendNodeName(token);
//                             token = strTok.nextToken();
//                         }                        

                        currentNode = tmp; // ???
//                        lastToken = token;
                        lastToken = token;
                            //                     ++indent;
                            //                     System.out.println("indent: " + indent);

                            //                     currentNode.level = indent;
                            //                     //                    continue;
                    };

//                 if (lastToken.equals("'") && token.equals("'"))
//                     {
//                                     token = strTok.nextToken();
                                    
//                                     while (!token.equals("'"))
//                                         {
//                                             currentNode.appendNodeName(token);
//                                             token = strTok.nextToken();
//                                         }


//                     };


                // token is identifier for internal node;; strange

                if ( (lastToken.equals(")") 
                      // || lastToken.equals(",") 
                      ) // || lastToken.equals("'")  ) 
                     && ( 
                         ! (
                            //token.equals("(") ||
                            token.equals(":") ) ) )
                    {

                        System.out.println("found internal node name : " + token);
                        //                 if(isIdentifier(token))//token is a identifier -> new named node
                        //                     {
                        //                     System.out.println("recognized identifier: >>>" + token + "<<<");


//                         while ( ( (token.equals(" ") || token.equals("\n") || token.equals("\t") ) )
//                                 && strTok.hasMoreTokens()  )
//                             {
//                                 token = strTok.nextToken();
//                             }
                        

//                         if (token.equals("'"))
//                             {

//                                     token = strTok.nextToken();
//                                     while (!token.equals("'"))
//                                         {
//                                             currentNode.appendNodeName(token);
//                                             token = strTok.nextToken();
//                                         }

// //                                         System.out.println("NodeName: " + currentNode.getNodeName());
// //                                         System.exit(1);
//                             }
//                         else
//                             {
                                 currentNode.setNodeName(token);
//                             };
                                 if (token.equals("'"))
                                     {
                                         insideString = true;
                                     };



                        //                        lastToken = token;

//                         while (!token.equals(":"))

//                         {

//                             currentNode.appendNodeName(token);
//                             token = strTok.nextToken();
                            
//                         }
                        lastToken = token; //token;
                        ++indent;
                            //                     System.out.println("indent: " + indent);

                        currentNode.level = indent;
                            //                     //                    continue;
                    };


                 if(token.equals(":")) {

                     token = strTok.nextToken();
                     currentNode.setDistance((float)Float.parseFloat(token));
                     lastToken = new String("dist");

//                     System.out.println("recognized token ':' >>>" + token + "<<<");
//                     System.out.println("do nothing");
//                     //                    continue;
                 };


//                  if(lastToken.equals(":")) {

//                      //                     token = strTok.nextToken();
//                      currentNode.setDistance((float)Float.parseFloat(token));
//                      lastToken = new String("dist");

// //                     System.out.println("recognized token ':' >>>" + token + "<<<");
// //                     System.out.println("do nothing");
// //                     //                    continue;
//                  };



                 if(token.equals(")") ){
                     if (!lastToken.equals("dist"))
                         {
                             System.out.println("missing distance");
                             System.exit(1);
                         };
//                     System.out.println("recognized tokens: ')' >>>" + token + "<<<");
                     ++ numClose;
                     if (currentNode.getChilds().size() == 0)
                         {
                             currentNode.setLeaf(true);

//                             System.out.println("nodeName:>>>"+currentNode.getNodeName()+"<<<");
                         }
                     else{currentNode.setLeaf(false);}
                     currentNode = currentNode.getFather();
                     lastToken = token;
//                     System.out.println("went down to nodeName:>>>"+currentNode.getNodeName()+"<<<");

                     --indent;
//                     System.out.println("indent: " + indent);
//                     //                    continue;
                 }

                 if(token.equals(",") ){

                     if (!lastToken.equals("dist"))
                         {
                             System.out.println("missing distance, outside NodeName");
                             System.exit(1);
                         };

//                     System.out.println("recognized token: ',' >>>" + token + "<<<");
                     if (currentNode.getChilds().size() == 0)
                         {
                             currentNode.setLeaf(true);
//                             System.out.println("nodeName:>>>"+currentNode.getNodeName()+"<<<");
                         }
                     else{currentNode.setLeaf(false);}
                     currentNode = currentNode.getFather();
//                     System.out.println("went down to nodeName:>>>"+currentNode.getNodeName()+"<<<");

                     --indent;
//                     System.out.println("indent: " + indent);
//                     //                    continue;

                     lastToken = token;
                 };






//                 // formating for test and debugging
//                 if(indent < 0)
//                     { System.out.println("ParsingError"); System.exit(1);}

//                 String indentString = new String();
//                 for (int i = indent; i > 0; --i)
//                     {
//                         if (i==0){}
//                         else{indentString = indentString.concat("-");}
//                     }
//                 //                System.out.println(indentString + token);
            }

        // stops at last node before root because root node has no distance  
        //      System.out.println("anchor hat " + anchor.getChilds().size() + "Kinder");


        return (TreeNode) anchor.getChilds().elementAt(0); 
}



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
