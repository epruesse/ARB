import java.util.*;
import java.awt.*;
import java.awt.event.*;


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

public static String readTree(HttpSubsystem webAccess, boolean forceReload)
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
            System.out.println("You have tree version '" + localTreeVersion+"'");
            System.out.println("Downloading updated tree '"+serverTreeVersion+"' ..");

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
                    System.out.println("Warning: downloaded tree has unexpected version '"+tr.getVersionString()+"'");
                }
            }
        }

        return tr.getTreeString();
    }

public static void main(String[] args)
    {
        Client cl          = new Client();
        Toolkit.clientName = "arb_probe_library";

        System.out.println(Toolkit.clientName+" v"+Toolkit.clientVersion+" -- (C) 2003 Lothar Richter & Ralf Westram");
        CommandLine cmdline = new CommandLine(args, cl.knownOptions());

        if (cmdline.getOption("server")) {
            cl.baseurl = cmdline.getOptionValue("server");
        }
        else {
            // cl.baseurl = new String("http://probeserver.mikro.biologie.tu-muenchen.de/"); // final server URL (not working yet)
            cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/"); // URL for debugging
        }

        cl.webAccess = new HttpSubsystem(cl.baseurl);

        // ask server for version info
        cl.webAccess.retrieveVersionInformation(); // terminates on failure
        if (!Toolkit.clientVersion.equals(cl.webAccess.getNeededClientVersion())) {
            Toolkit.AbortWithError("Your client is out-of-date!\nPlease get the newest version from\n  "+cl.baseurl+"arb_probe_library.jar");
        }

        // load and parse the most recent tree
        {
            boolean reload_tree = cmdline.getOption("tree") && cmdline.getOptionValue("tree").equals("reload");
            cl.treeString       = readTree(cl.webAccess, reload_tree); // terminates on failure
            cl.root             = (new TreeParser(cl.treeString)).getRootNode();
            cl.fillShortNameHash(cl.root);
            // System.out.println("Number of shortNameHash-entries: " + cl.shortNameHash.size());
        }

        if (cl.root == null)
            {
                System.out.println("in Client(): no valid node given to display");
                System.exit(1);
            }

        cl.root.setPath("");
        cl.display = new ProbesGUI(cl.root, 10, Toolkit.clientName+" v"+Toolkit.clientVersion, cl);

        // cl.display.getTreeDisplay().setBoss(cl); // obtain reference to Treedisplay first !
        // cl.display.setLocation(200, 200); // this seems to cause display problems with fvwm
        // cl.display.setVisible(true);
        // cl.display.setLocation(200, 200);
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

public void matchProbes(String probeInfo) {
    boolean needUpdate = false;
    String  error      = null;

    clearDetails();

    if (probeInfo == null) {
        root.unmarkSubtree();   // unmark all
        needUpdate = true;
    }
    else {
        int komma1 = probeInfo.indexOf(',');
        int komma2 = probeInfo.indexOf(',', komma1+1);

        if (komma1 == -1 || komma2 == -1) {
            error = "Kommas expected in '"+probeInfo+"'";
        }
        else {
            String groupId = probeInfo.substring(komma2+1);
            members        = groupCache.getGroupMembers(webAccess, groupId, komma1);
            if (members == null) {
                error = "during probe match: "+groupCache.getError();
            }
            else {
                updateDetails(probeInfo,members);
                needUpdate = root.markSpecies(","+members+",");
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
}

public void updateNodeInformation(String encodedPath, boolean exactMatches)
    {
        ProbeList list = display.getProbeList();
        list.removeAll();
        list.add("wait..");

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

public void clearDetails()
    {
        TextArea ta = display.getDetails();
        ta.setText("");
    }

public void showError(String error)
    {
        TextArea ta = display.getDetails();
        ta.setText(error);
        ta.setCaretPosition(0);
    }

public void updateDetails (String probeInfo, String shortNames)
    {
        TextArea ta = display.getDetails();
        ta.setText("");
        int komma1 = probeInfo.indexOf(',');
        int komma2 = probeInfo.indexOf(',', komma1+1);

        if (komma1 == -1 || komma2 == -1) {
            System.out.println("Kommas expected in '"+probeInfo+"'");
        }
        else
            {
                String probeSequenz = probeInfo.substring(0,komma1);
                ta.append("Details for probe: " + probeSequenz + "\n");
                int tm = 0;
                for (int i = 0; i < probeSequenz.length(); i ++)
                    {
                        tm = tm + ((probeSequenz.charAt(i) == 'A' || probeSequenz.charAt(i) == 'U') ? 2 : 4); // 2 + 4 rule
                    }
                ta.append("Melting temperature (2+4 rule):  " + tm+ "°C\n");
                // GC content
                int gc = 0;
                for (int i = 0; i < probeSequenz.length(); i ++)
                    {
                        gc = gc + ((probeSequenz.charAt(i) == 'G' || probeSequenz.charAt(i) == 'C') ? 1: 0);
                    }
                ta.append("%GC-Content:  " + (gc*100)/probeSequenz.length() + "%\n");

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

                        ta.append("\nNumber of hits: "+hit_count+"\n");

                        komma = 0;
                        begin = 0;
                        while (begin < snlen)
                            {
                                komma                  = shortNames.indexOf(',', begin);
                                if (komma == -1) komma = snlen;

                                String sn = shortNames.substring(begin,komma);
                                String details;

                                TreeNode ref = (TreeNode)shortNameHash.get(sn);
                                details = ref.getFullName() + " , " + ref.getAccessionNumber() + "\n";
                                ta.append(details);

                                begin = komma + 1;
                            }
                    }
                else {
                    ta.append("Error: probe does not match any species (internal error)");
                }

                // System.out.println("probeInfo:  " + probeInfo);
                // System.out.println("probe sequence: " + probeSequenz);



            }
        ta.setCaretPosition(0);
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


}

