import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;


/**
 * central driver and control class for ARB probe library client
 */

class Client
{
private ProbesGUI     display;
private TreeNode      root;
private String        treeString;
private String        baseurl;
private HttpSubsystem webAccess;
private GroupCache    groupCache;
private String        members;
private Vector        memberList;
private HashMap       shortNameHash;

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
                //             cl.baseurl = new String("http://probeserver.mikro.biologie.tu-muenchen.de/probe_library/"); // final server URL (DNS not propagated yet)
                cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probe_library/"); // final server URL
                //             cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/"); // URL for debugging
            }

            cl.display = new ProbesGUI(10, Toolkit.clientName+" v"+Toolkit.clientVersion, cl);
            
            try {
                cl.webAccess = new HttpSubsystem(cl.baseurl);

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

                // cl.display.getTreeDisplay().setBoss(cl); // obtain reference to Treedisplay first !
                // cl.display.setLocation(200, 200); // this seems to cause display problems with fvwm
                // cl.display.setVisible(true);
                // cl.display.setLocation(200, 200);
            }
            catch (Exception e) {
                cl.showError(e.getMessage());
                MessageDialog popup = new MessageDialog(cl.getDisplay(), "Error:", e.getMessage());
                while (!popup.okClicked()) ; // @@@ do non-busy wait here (how?)
                System.exit(1);
            }
        }
        catch (Exception e) {
            System.out.println(e.getMessage());
            System.exit(1);
        }
    }

public Client()
    {
        groupCache = new GroupCache();
        memberList = new Vector();
        shortNameHash = new HashMap();
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

public void matchProbes(String probeInfo) throws Exception {
    boolean needUpdate = false;
    String  error      = null;

    needUpdate = root.unmarkSubtree(); // unmark all

    if (probeInfo != null) {
        int komma1 = probeInfo.indexOf(',');
        int komma2 = probeInfo.indexOf(',', komma1+1);

        if (komma1 == -1 || komma2 == -1) {
            error = "Kommas expected in '"+probeInfo+"'";
        }
        else {
            showMessage("Retrieving match information..");
            String groupId = probeInfo.substring(komma2+1);
            members        = groupCache.getGroupMembers(webAccess, groupId, komma1);
            if (members == null) {
                error = "during probe match: "+groupCache.getError();
            }
            else {
                needUpdate = root.markSpecies(members, shortNameHash);
                updateDetails(probeInfo,members);
            }
        }
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

public void updateNodeInformation(String encodedPath, boolean exactMatches) throws Exception
    {
        ProbeList list = display.getProbeList();
        list.removeAll();
        list.add("wait..");
        showMessage("Retrieving probes ..");

        String answer = webAccess.retrieveNodeInformation(encodedPath, exactMatches);

        if (answer == null) { // error during retrieval
            list.removeAll();
            root.unmarkSubtree();
            showError(webAccess.getLastRequestError());
        }
        else {
            ServerAnswer parsedAnswer = new ServerAnswer(answer, true, false);

            if (parsedAnswer.hasError()) {
                String error = parsedAnswer.getError();
                System.out.println("Error while retrieving probe information: "+error);
            }
            else {
                list.setContents(parsedAnswer);

                if (parsedAnswer.hasKey("probe0")) { // if we found probes
                    // select the first one
                    list.select(0);
                    // list.requestFocus(); // doesn't work
                    matchProbes(list.getProbeInfo(0)); // so we do the action manually
                }
                else {
                    root.unmarkSubtree();
                    display.getTreeDisplay().repaint();
                }
            }
        }
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

    public void showError(String error)
    {
        TextArea ta = display.getDetails();
        if (detailState == 0)
            ta.setText("Error: "+error+"\n");
        else
            ta.append("Error: "+error+"\n");

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

    public void updateDetails (String probeInfo, String shortNames)
    {
        TextArea ta = display.getDetails();
        ta.setVisible(false);
        int komma1 = probeInfo.indexOf(',');
        int komma2 = probeInfo.indexOf(',', komma1+1);

        if (komma1 == -1 || komma2 == -1) {
            System.out.println("Kommas expected in '"+probeInfo+"'");
        }
        else {
            StringBuffer theText = new StringBuffer();

            String probeSequenz = probeInfo.substring(0,komma1);
            theText.append("Selected probe: " + probeSequenz + "\n");

            int tm = 0;
            for (int i = 0; i < probeSequenz.length(); i ++) {
                tm = tm + ((probeSequenz.charAt(i) == 'A' || probeSequenz.charAt(i) == 'U') ? 2 : 4); // 2 + 4 rule
            }
            theText.append("Melting temperature (2+4 rule):  " + tm+ "°C\n");

            // GC content
            int gc = 0;
            for (int i = 0; i < probeSequenz.length(); i ++) {
                gc = gc + ((probeSequenz.charAt(i) == 'G' || probeSequenz.charAt(i) == 'C') ? 1: 0);
            }
            theText.append("%GC-Content:  " + (gc*100)/probeSequenz.length() + "%\n");

            int snlen = shortNames.length();
            if (snlen != 0)
            {
                int komma       = 0;
                int begin       = 0;
                int hit_count = 0;

                while (begin < snlen) { // detect number of hits
                    komma                  = shortNames.indexOf(',', begin);
                    if (komma == -1) komma = snlen;
                    hit_count++;
                    begin                  = komma+1;
                }

                theText.append("The probe matches "+hit_count+" species:\n");

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
        }
        ta.setCaretPosition(0);
        detailState = 0;
        ta.setVisible(true);
    }

    public void fillShortNameHash(TreeNode node)
    {
        if (!node.testLeaf())
        {
            fillShortNameHash ( (TreeNode)node.getChilds().elementAt(0) ) ;
            fillShortNameHash ( (TreeNode)node.getChilds().elementAt(1) ) ;
        }
        else
        {
            shortNameHash.put(node.getShortName(), node);
        }
    }

    public HashMap getShortNameHash()
    {
        return shortNameHash;
    }

}

