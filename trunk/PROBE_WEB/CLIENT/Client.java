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
            Toolkit.AbortWithError("Your client is out-of-date! Please download the new version from http://arb-home.de/");
        }

        // load and parse the most recent tree
        {
            boolean reload_tree = cmdline.getOption("tree") && cmdline.getOptionValue("tree").equals("reload");
            cl.treeString       = readTree(cl.webAccess, reload_tree); // terminates on failure
            cl.root             = (new TreeParser(cl.treeString)).getRootNode();
        }

        if (cl.root == null)
            {
                System.out.println("in Client(): no valid node given to display");
                System.exit(1);
            }

        cl.root.setPath("");
        cl.display = new ProbesGUI(cl.root, 10, Toolkit.clientName+" Version "+Toolkit.clientVersion, cl);

        // cl.display.getTreeDisplay().setBoss(cl); // obtain reference to Treedisplay first !
        // cl.display.setLocation(200, 200); // this seems to cause display problems with fvwm
        // cl.display.setVisible(true);
        // cl.display.setLocation(200, 200);
    }

public Client()
    {
        groupCache = new GroupCache();
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
    if (probeInfo == null) {
        root.unmarkSubtree();   // unmark all
        needUpdate = true;
    }
    else {
        int komma1 = probeInfo.indexOf(',');
        int komma2 = probeInfo.indexOf(',', komma1+1);

        if (komma1 == -1 || komma2 == -1) {
            System.out.println("Kommas expected in '"+probeInfo+"'");
        }
        else {
            String groupId = probeInfo.substring(komma2+1);
            String members = groupCache.getGroupMembers(webAccess, groupId, komma1);

            if (members == null) {
                System.out.println("Error during probe match: "+groupCache.getError());
            }
            else {
                System.out.println("Members='"+members+"'");
                needUpdate = root.markSpecies(","+members+",");
            }
        }
    }
    if (needUpdate) display.getTreeDisplay().repaint();
    else System.out.println("Nothing changed");
}

public void updateNodeInformation(String encodedPath)
    {
        ProbeList list = display.getProbeList();
        list.removeAll();
        list.add("wait..");

        String       answer       = webAccess.retrieveNodeInformation(encodedPath);
        ServerAnswer parsedAnswer = new ServerAnswer(answer, true, true);

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

