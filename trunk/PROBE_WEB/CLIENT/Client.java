import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;


/**
 * central driver and control class for ARB probe library client
 */

class Client
{
    private ProbesGUI     display    = null;
    private TreeNode      root       = null;
    private String        treeString = null;
    private String        baseurl    = null;
    private HttpSubsystem webAccess  = null;
    private IOManager     iom        = null;    




//     private GroupCache groupCache    = new GroupCache(this);
    private Vector     memberList    = new Vector();
    private HashMap    shortNameHash = new HashMap();

    private TreeNode lastNode = null; // last node (for which probes were retrieved)
    private String   members  = null; // members hit by last selected probe

    public Client() {
        Probe.groupCache = new GroupCache(this);
    }

    public HttpSubsystem webAccess() { return webAccess; }

    private HashMap knownOptions()
    {
        HashMap hm = new HashMap();
        hm.put("server", "=URL        Specify server URL manually (note: needs leading /)");
        hm.put("tree",   "=reload       Force tree reload");
        return hm;
    }

    public String readTree(HttpSubsystem webAccess, boolean forceReload) throws Exception
    {
        String     localTreeVersion  = null;
        String     serverTreeVersion = webAccess.getNeededTreeVersion();
        String     error             = null;
        String     localTreeFile     = new String("probetree.gz");
        TreeReader tr                = null;

        if (forceReload) {
            localTreeVersion = "reload forced";
        }
        else {
            tr    = new TreeReader(localTreeFile);
            error = tr.getError();

            if (error != null) {
                System.out.println("Can't access the tree ("+error+")");
                localTreeVersion = "missing or corrupt";
            }
            else {
                localTreeVersion = tr.getVersionString();
            }
        }

        if (!serverTreeVersion.equals(localTreeVersion))
        {
            showMessage("You have tree version '" + localTreeVersion+"'");
            showMessage("Downloading updated tree '"+serverTreeVersion+"' ..");
            //             System.out.println("You have tree version '" + localTreeVersion+"'");
            //             System.out.println("Downloading updated tree '"+serverTreeVersion+"' ..");

            error = webAccess.downloadZippedTree(localTreeFile);
            if (error == null) {
                tr    = new TreeReader(localTreeFile);
                error = tr.getError();
            }
            if (error != null) {
                Toolkit.AbortWithError("Server problem: Can't get a valid tree ("+error+")");
            }
            else {
                if (!tr.getVersionString().equals(serverTreeVersion)) {
                    showMessage("Warning: downloaded tree has unexpected version '"+tr.getVersionString()+"'");
                }
            }
        }
        else {
            showMessage("Your tree is up to date.");
        }

        return tr.getTreeString();
    }


    public void saveConfig() // called at shutdown
    {
        Point     location = display.getLocationOnScreen();
        Dimension size     = display.getScrollPaneSize();

        System.out.println("Current location on screen: x="+location.x+" y="+location.y);
        System.out.println("Scroll pane dimension:      width="+size.width+" height="+size.height);

        System.out.println("Saving config to ... [not implemented yet]");
    }

    public ProbesGUI getDisplay()
    {
        return display;
    }

    public void matchProbes(Probe probe) throws Exception {
        boolean needUpdate = false;
        String  error      = null;

        needUpdate = root.unmarkSubtree(); // unmark all

        if (probe != null) {
            members                   = probe.members();
//             members                   = groupCache.getGroupMembers(probe.groupID(), probe.length());
            //             int komma1 = probeInfo.indexOf(',');
            //             int komma2 = probeInfo.indexOf(',', komma1+1);

//             if (komma1 == -1 || komma2 == -1) {
//                 error = "Kommas expected in '"+probeInfo+"'";
//             }
//             else {
//                 showMessage("Retrieving match information..");
//                 String groupId = probeInfo.substring(komma2+1);
//                 members        = groupCache.getGroupMembers(webAccess, groupId, komma1);
                 if (members == null) {
                     error = "during probe match: "+probe.getError();
                 }
                 else {
                     needUpdate = root.markSpecies(members, shortNameHash) || needUpdate;
                     updateDetails(probe, members);
                 }
//             }
        }

        if (error != null) {
            root.unmarkSubtree();
            showError(error);
            needUpdate = true;
        }

        if (needUpdate) {
            display.getTreeDisplay().repaint();
        }
        else {
            System.out.println("Marks did not change.");
        }

        //     throw new Exception("Test exception");
        //     Toolkit.AbortWithError("Test error");
    }

    public void updateNodeInformation(TreeNode clickedNode) throws Exception
    {
        ProbeList list = display.getProbeList();
        list.removeAll();
        list.add("retrieving probes..");
//         showMessage("Retrieving probes ..");

        String emptyMessage = null;
        try {
            //             String     encodedPath  = clickedNode.getCodedPath();
            lastNode = clickedNode;
            boolean    exactMatches = clickedNode.getExactMatches() > 0;
            NodeProbes probes       = clickedNode.getNodeProbes(exactMatches);

            list.setContents(probes);

            System.out.println("list.length()="+list.length());
            if (list.length() == 0) {
                root.unmarkSubtree();
            }
            else {
                list.select(0);
                // list.requestFocus(); // doesn't work
                matchProbes(list.getProbe(0)); // so we do the action manually
            }
        }
        catch (ClientException e) {
            showError(e.getMessage());
            emptyMessage = new String("An error occurred!");
        }
        catch (Exception e) {
            showError("in updateNodeInformation: "+e.getMessage());
            e.printStackTrace();
            emptyMessage = new String("Oops! Uncaught exception!");
        }

        if (emptyMessage != null) {
            list.removeAll();
            list.add(emptyMessage);
            root.unmarkSubtree();
        }

        // ...

        //         String answer = webAccess.retrieveNodeInformation(encodedPath, exactMatches);

        //         if (answer == null) { // error during retrieval
        //             list.removeAll();
        //             root.unmarkSubtree();
        //             showError(webAccess.getLastRequestError());
        //         }
        //         else {
        //             ServerAnswer parsedAnswer = new ServerAnswer(answer, true, false);

        //             if (parsedAnswer.hasError()) {
        //                 Toolkit.AbortWithError("While retrieving probe information: "+parsedAnswer.getError());
        //             }
        //             else {
        //                 list.setContents(parsedAnswer);

        //                 if (parsedAnswer.hasKey("probe0")) { // if we found probes
        //                     // select the first one
        //                     list.select(0);
        //                     // list.requestFocus(); // doesn't work
        //                     matchProbes(list.getProbeInfo(0)); // so we do the action manually
        //                 }
        //                 else {
        //                     root.unmarkSubtree();
        //                     display.getTreeDisplay().repaint();
        //                 }
        //             }
        //         }
    }


    // rest should be grouped in separate class later on

    private int detailState = 0; // 0 = details displayed; 1 = messages/errors displayed; 2 = empty

    public void clearDetails()
    {
        if (detailState != 2) {
            TextArea ta = display.getDetails();
            ta.setText("");
            detailState  = 2;
        }
    }

    private static String spacer = "          ";

    private String indent(String text, int indent) {
        if (indent == 0) return text;
        if (spacer.length()<indent) spacer = spacer+spacer;
        return spacer.substring(0, indent)+text;
    }

    private String formatError(String error, int indent) {
        int colon = error.indexOf(':');

        if (colon == -1) return indent(error, indent);

        return
            indent(error.substring(0, colon+1), indent) + "\n" +
            formatError(error.substring(colon+1), indent+1);
    }

    public void showError(String error) {
        TextArea ta       = display.getDetails();
        if (detailState == 0)
            ta.setText("\n"+formatError(error, 0)+"\n");
        else
            ta.append("\n"+formatError(error, 0)+"\n");

        detailState = 1;
        ta.setCaretPosition(999999999); // ugly way to go to end of text
    }

    public void showMessage(String message)
    {
        TextArea ta = display.getDetails();
        if (detailState == 0)
            ta.setText(message+"\n");
        else
            ta.append(message+"\n");

        detailState = 1;
        ta.setCaretPosition(999999999); // ugly way to go to end of text
    }

    public void updateDetails(Probe probe, String shortNames) throws Exception
    {
        TextArea ta = display.getDetails();
        ta.setVisible(false);

        //         int komma1 = probeInfo.indexOf(',');
        //         int komma2 = probeInfo.indexOf(',', komma1+1);

        //         if (komma1 == -1 || komma2 == -1) {
        //             System.out.println("Kommas expected in '"+probeInfo+"'");
        //         }
        //         else {
        StringBuffer theText = new StringBuffer();

        //             String probeSequenz = probeInfo.substring(0,komma1);
        theText.append("Selected probe: " + probe.sequence() + "\n");

        //             int tm = 0;
        //             for (int i = 0; i < probeSequence.length(); i ++) {
        //                 tm = tm + ((probeSequence.charAt(i) == 'A' || probeSequenz.charAt(i) == 'U') ? 2 : 4); // 2 + 4 rule
        //             }
        theText.append("Melting temperature (2+4 rule):  " + probe.temperature() + "°C\n");

        // GC content
        //             int gc = 0;
        //             for (int i = 0; i < probeSequenz.length(); i ++) {
        //                 gc = gc + ((probeSequenz.charAt(i) == 'G' || probeSequenz.charAt(i) == 'C') ? 1: 0);
        //             }
        //             theText.append("%GC-Content:  " + (gc*100)/probeSequenz.length() + "%\n");
        theText.append("%GC-Content:  " + (int)(probe.gc_content()+0.5) + "%\n");

//         theText.append("");

        int snlen = shortNames.length();
        if (snlen != 0)
        {
            int komma       = 0;
            int begin       = 0;
            int hit_count = probe.no_of_hits();

//             while (begin < snlen) { // detect number of hits
//                 komma                  = shortNames.indexOf(',', begin);
//                 if (komma == -1) komma = snlen;
//                 hit_count++;
//                 begin                  = komma+1;
//             }

            int species_in_subtree = lastNode.getNoOfLeaves();

            theText.append("\nThe probe matches " + hit_count +
                           " of " + species_in_subtree + " species = " +
                           (int)((hit_count*100.0/species_in_subtree)+0.5)+"%\n");

            komma = 0;
            begin = 0;
            while (begin < snlen) {
                komma                  = shortNames.indexOf(',', begin);
                if (komma == -1) komma = snlen;

                String   sn  = shortNames.substring(begin,komma);
                TreeNode ref = (TreeNode)shortNameHash.get(sn);

                // String details = ref.getFullName() + " , " + ref.getAccessionNumber() + "\n";
                // toAppend.append(details);
                theText.append(ref.getFullName());
                theText.append(" , ");
                theText.append(ref.getAccessionNumber());
                theText.append("\n");

                begin = komma + 1;
            }
        }
        else {
            theText.append("Error: probe does not match any species (internal error)\n");
        }
        ta.setText(theText.toString());
        //         }
        ta.setCaretPosition(0);
        detailState = 0;
        ta.setVisible(true);
    }

    public void fillShortNameHash(TreeNode node) {
        if (!node.isLeaf()) {
            //             fillShortNameHash ( (TreeNode)node.getChilds().elementAt(0) ) ;
            //             fillShortNameHash ( (TreeNode)node.getChilds().elementAt(1) ) ;
            
            fillShortNameHash(node.upperSon());
            fillShortNameHash(node.lowerSon());
        }
        else {
            shortNameHash.put(node.getShortName(), node);
        }
    }

    public HashMap getShortNameHash() { return shortNameHash; }

    
//     public IOManager getIOManager(){
// 	if (iom == null){
// 	    System.out.println("Cannot return IOManager");
// 	    System.exit(2);
// 	}

// 	return iom;
//    }



    public static void main(String[] args)
    {
        Client cl          = new Client();
        Toolkit.clientName = "ARB probe library";

        System.out.println(Toolkit.clientName+" v"+Toolkit.clientVersion+" -- (C) 2003/2004 Lothar Richter & Ralf Westram");
        try {
            CommandLine cmdline = new CommandLine(args, cl.knownOptions());

            if (cmdline.getOption("server")) {
                cl.baseurl = cmdline.getOptionValue("server");
            }
            else {
                cl.baseurl = new String("http://probeserver.mikro.biologie.tu-muenchen.de/probe_library/"); // final server URL (DNS not propagated yet)
                // cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probe_library/"); // final server URL
                //             cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/"); // URL for debugging
            }

            cl.display = new ProbesGUI(10, Toolkit.clientName+" v"+Toolkit.clientVersion, cl);

	    cl.iom = new IOManager(cl.display, ""); // no config file name given, using default
	    if (cl.iom == null){System.out.println("Cannot initialize IOManager");System.exit(2);}
	    cl.display.setIOManager(cl.iom);
            try {
                cl.webAccess         = new HttpSubsystem(cl.baseurl);
                NodeProbes.webAccess = cl.webAccess;
//                 GroupCache.webAccess = cl.webAccess;
//                 cl.groupCache        = new GroupCache(cl.webAccess);

                // ask server for version info
                cl.showMessage("Contacting probe server ..");
                cl.webAccess.retrieveVersionInformation(); // terminates on failure
                if (!Toolkit.clientVersion.equals(cl.webAccess.getNeededClientVersion())) {
                    Toolkit.AbortWithError("Your client is out-of-date!\nPlease get the newest version from\n  "+cl.baseurl+"arb_probe_library.jar");
                }

                // load and parse the most recent tree
                {
                    boolean reload_tree = cmdline.getOption("tree") && cmdline.getOptionValue("tree").equals("reload");
                    cl.treeString       = cl.readTree(cl.webAccess, reload_tree); // terminates on failure
                    cl.root             = (new TreeParser(cl.treeString)).getRootNode();
                    cl.fillShortNameHash(cl.root);
                    // System.out.println("Number of shortNameHash-entries: " + cl.shortNameHash.size());
                }

                if (cl.root == null) {
                    Toolkit.AbortWithError("no valid node given to display");
                    //                     System.out.println("in Client(): no valid node given to display");
                    //                     System.exit(1);
                }

                cl.root.setPath("");
                // cl.display = new ProbesGUI(cl.root, 10, Toolkit.clientName+" v"+Toolkit.clientVersion, cl);
                cl.display.initTreeDisplay(cl.root);

                // cl.display.getTreeDisplay().setClient(cl); // obtain reference to Treedisplay first !
                // cl.display.setLocation(200, 200); // this seems to cause display problems with fvwm
                // cl.display.setVisible(true);
                // cl.display.setLocation(200, 200);
            }
            catch (ClientException e) {
                cl.showError(e.getMessage());
                MessageDialog popup = new MessageDialog(cl.getDisplay(), e.get_kind(), e.get_plain_message());
                while (!popup.okClicked()) ; // @@@ do non-busy wait here (how?)
                System.exit(e.get_exitcode());
            }
            catch (Exception e) {
                cl.showError(e.getMessage());
                MessageDialog popup = new MessageDialog(cl.getDisplay(), "Uncaught exception", e.getMessage());
                while (!popup.okClicked()) ; // @@@ do non-busy wait here (how?)
                System.exit(3);
            }
        }
        catch (Exception e) {
            System.out.println(e.getMessage());
            System.exit(1);
        }
    }

}

