
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

    public TreeParser(String ts) { treeString = ts; }

    public TreeNode getRootNode() throws Exception {
        if (root == null) {
            try {
                root = generateTreeNodes();
                root.setFather(root);

                return root;
            }
            catch (Exception e) {
                Toolkit.AbortWithError("Cant create tree: "+e.getMessage());
                // System.out.println("Exception occurred: "+e.getMessage());
                // System.out.println("opening paren's : >>>" + numOpen + "<<<");
                // System.out.println("closing paren's : >>>" + numClose + "<<<");
                // return null;
            }
        }
        return root;
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

    private void handleInnerNodeInformation(TreeNode node, String nodeInfo) throws Exception {
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

    private void handleLeafNodeInformation(TreeNode node, String nodeInfo) throws Exception{
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
    private TreeNode generateTreeNodes() throws Exception {
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
            Toolkit.showDebugMessage("got an error at '"+treeString.substring(parse_position, parse_position+5)+"'");
            Toolkit.showDebugMessage(" at '"+treeString.substring(parse_position-40, parse_position+40)+"'");
            Toolkit.AbortWithError("generateTreeNodes: Error parsing tree ("+parse_error+")");
        }

        return anchor;
    }

}
