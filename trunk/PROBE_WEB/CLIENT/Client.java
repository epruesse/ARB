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

    private Vector     memberList    = new Vector();
    private HashMap    shortNameHash = new HashMap();

    private TreeNode lastNode = null; // last node (for which probes were retrieved)
    private String   members  = null; // members hit by last selected probe
    
    private String       configFileName = ".arb_probe_library"; // name of config file
    private ServerAnswer config;

    public Client() {
        Probe.groupCache = new GroupCache(this);
        Probe.setCompareOrder(Probe.SORT_BY_SEQUENCE); // 1 is default sort order
        Toolkit.registerClient(this);
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
            Toolkit.showMessage("You have tree version '" + localTreeVersion+"'");
            Toolkit.showMessage("Downloading updated tree '"+serverTreeVersion+"' ..");

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
                    Toolkit.showMessage("Warning: downloaded tree has unexpected version '"+tr.getVersionString()+"'");
                }
            }
        }
        else {
            Toolkit.showMessage("Your tree is up to date.");
        }

        return tr.getTreeString();
    }


    public ProbesGUI getDisplay() { return display; }

    public void matchProbes(Probe probe) throws Exception {
        boolean needUpdate = false;
        String  error      = null;

        needUpdate = root.unmarkSubtree(); // unmark all

        if (probe != null) {
            members = probe.members();
            if (members == null) {
                error = "during probe match: "+probe.getError();
            }
            else {
                needUpdate = root.markSpecies(members, shortNameHash) || needUpdate;
                updateDetails(probe, members);
            }
        }

        if (error != null) {
            root.unmarkSubtree();
            Toolkit.showError(error);
            needUpdate = true;
        }

        if (needUpdate) {
            display.getTreeDisplay().repaint();
        }
        else {
            System.out.println("Marks did not change.");
        }
    }

    public void updateNodeInformation(TreeNode clickedNode) throws Exception
    {
        ProbeList list = display.getProbeList();
        list.removeAll();

        // String testAnswer = Toolkit.askUser("Online-Warnung", "Sonden vom Server holen?", "Ja,Nein,Egal");
        // System.out.println("testAnswer='"+testAnswer+"'");

        list.add("retrieving probes..");

        String emptyMessage = null;
        try {
            lastNode                = clickedNode;
            boolean    exactMatches = clickedNode.getExactMatches() > 0;
            NodeProbes probes       = clickedNode.getNodeProbes(exactMatches);

            // Toolkit.clickOK("Nachricht", "Die Sonden wurden geholt");

            list.setContents(probes);

            if (list.length() == 0) {
                root.unmarkSubtree();
            }
            else {
                // list.selectProbe(0); // done by list.setContents
                // list.requestFocus(); // doesn't work
                // matchProbes(list.getProbe(0)); // so we do the action manually
            }
        }
        catch (ClientException e) {
            Toolkit.showError(e.getMessage());
            emptyMessage = new String("An error occurred!");
        }
        catch (Exception e) {
            Toolkit.showError("in updateNodeInformation: "+e.getMessage());
            e.printStackTrace();
            emptyMessage = new String("Oops! Uncaught exception!");
        }

        if (emptyMessage != null) {
            list.removeAll();
            list.add(emptyMessage);
            root.unmarkSubtree();
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

    public void show_error(String error) {
        TextArea ta       = display.getDetails();
        if (detailState == 0)
            ta.setText("\n"+formatError(error, 0)+"\n");
        else
            ta.append("\n"+formatError(error, 0)+"\n");

        detailState = 1;
        ta.setCaretPosition(999999999); // ugly way to go to end of text
    }

    public void show_message(String message)
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
        StringBuffer theText = new StringBuffer();

        theText.append("Selected probe: " + probe.sequence() + "\n");
        theText.append("Melting temperature (2+4 rule):  " + probe.temperature() + "�C\n");
        theText.append("%GC-Content:  " + (int)(probe.gc_content()+0.5) + "%\n");

        int snlen = shortNames.length();
        if (snlen != 0) {
            int komma              = 0;
            int begin              = 0;
            int hit_count          = probe.no_of_hits();
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

        TextArea ta = display.getDetails();
        ta.setVisible(false);
        ta.setText(theText.toString());
        ta.setCaretPosition(0);
        ta.setVisible(true);

        detailState = 0;
    }

    public void fillShortNameHash(TreeNode node) {
        if (!node.isLeaf()) {
            fillShortNameHash(node.upperSon());
            fillShortNameHash(node.lowerSon());
        }
        else {
            shortNameHash.put(node.getShortName(), node);
        }
    }

    public HashMap getShortNameHash() { return shortNameHash; }

    private static Dimension detectScreenSize() {
        GraphicsEnvironment   ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        GraphicsDevice        gd = ge.getDefaultScreenDevice();
        GraphicsConfiguration gc = gd.getDefaultConfiguration();
        Rectangle             sr = gc.getBounds();

        int width  = (int)sr.getWidth();
        int height = (int)sr.getHeight();

        // assume minimum size to ignore errors in detection:
        if (width<640) width   = 640;
        if (height<480) height = 480;

        return new Dimension(width, height);
    }

    public static void showRect(Rectangle rect, String name) {
        System.out.println("Rectangle "+name+": lu="+rect.x+"/"+rect.y+
                           " rl="+(rect.x+rect.width-1)+"/"+(rect.y+rect.height-1)+
                           " sz="+rect.width+"/"+rect.height);
    }

    private void createProbesGUI() throws Exception {
        Point     wantedOrigin         = null;
        Dimension wantedScrollPaneSize = null; 

        if (config != null) {
            try {
                wantedScrollPaneSize = new Dimension(config.getIntValue("Width"), config.getIntValue("Height"));
                wantedOrigin         = new Point(config.getIntValue("LocationX"), config.getIntValue("LocationY"));
            }
            catch (Exception e) {
                System.out.println("Your config seems to be corrupted:");
                System.out.println(e.getMessage());
                
                wantedScrollPaneSize = null;
                wantedOrigin         = null;
            }
        }

        Rectangle wantedBounds = null;
        if (wantedOrigin != null && wantedScrollPaneSize != null) {
            wantedBounds = new Rectangle(wantedOrigin.x, wantedOrigin.y, 640, 480); // size does not matter
        }
        else {
            // no dimension from config -> try to guess a good size:

            Dimension screenSize = detectScreenSize();
            System.out.println("* detected screen size = "+screenSize.width+"/"+screenSize.height);

            // test other screensizes:
            Dimension fakedSize = null;
            // fakedSize        = new Dimension(640, 480);
            // fakedSize        = new Dimension(800, 600);
            if (screenSize.width>1024) {
                fakedSize    = new Dimension(1024, 768); // leave it for the moment (otherwise sizing has a bug!)
            }
            // fakedSize = new Dimension(1280, 1024);
            if (fakedSize != null) {
                screenSize = fakedSize;
                System.out.println("* faked screen size = "+screenSize.width+"/"+screenSize.height);
            }

            int    dxborder = screenSize.width/7;
            int    dyborder = screenSize.height/7;

            wantedBounds = new Rectangle(dxborder/2, dyborder/2, screenSize.width-dxborder, screenSize.height-dyborder);
            // showRect(wantedBounds, "Estimated application bounds:");
        }

        String title = Toolkit.clientName+" v"+Toolkit.client_version;
        display      = new ProbesGUI(10, title, this, wantedBounds, wantedScrollPaneSize, 40);
    }

    public static void main(String[] args)
    {
        Client cl          = new Client();
        Toolkit.clientName = "ARB probe library";

        System.out.println(Toolkit.clientName+" v"+Toolkit.client_version+" -- (C) 2003/2004 Lothar Richter & Ralf Westram");
        try {
            CommandLine cmdline = new CommandLine(args, cl.knownOptions());

            if (cmdline.getOption("server")) {
                cl.baseurl = cmdline.getOptionValue("server");
            }
            else {
                cl.baseurl = new String("http://probeserver.mikro.biologie.tu-muenchen.de/probe_library/"); // final server URL
                // cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/"); // URL for debugging
            }

            cl.loadConfig();
            cl.createProbesGUI();
            cl.iom = new IOManager(cl.display);

            // check correctness of application placement:
            Rectangle resBounds = cl.display.getBounds(null);
            // showRect(resBounds, "resBounds (wrong value, but seems correct)");

            try {
                cl.webAccess         = new HttpSubsystem(cl.baseurl);
                NodeProbes.webAccess = cl.webAccess;

                // ask server for version info
                Toolkit.showMessage("Contacting probe server ..");
                cl.webAccess.retrieveVersionInformation(); // terminates on failure

                if (!Toolkit.interface_version.equals(cl.webAccess.getNeededInterfaceVersion())) {
                    Toolkit.AbortWithError("Your client is out-of-date!\n"+
                                           "Please get the newest version from\n  "+cl.baseurl+"arb_probe_library.jar");
                }
                if (!Toolkit.client_version.equals(cl.webAccess.getAvailableClientVersion())) {
                    String whatToDo = Toolkit.askUser("Notice", "A newer version of this client is available", "Ignore,Exit");

                    if (whatToDo.equals("Download")) {
                        Toolkit.AbortWithError("download not implemented yet.");
                    }
                    if (whatToDo.equals("Exit")) {
                        System.exit(1);
                    }
                }
                else {
                    Toolkit.showMessage("Your client is up to date.");
                }

                // load and parse the most recent tree
                {
                    boolean reload_tree = cmdline.getOption("tree") && cmdline.getOptionValue("tree").equals("reload");
                    cl.treeString       = cl.readTree(cl.webAccess, reload_tree); // terminates on failure
                    cl.root             = (new TreeParser(cl.treeString)).getRootNode();
                    cl.fillShortNameHash(cl.root);
                }

                if (cl.root == null) Toolkit.AbortWithError("no valid node given to display");

                cl.root.setPath("");
                cl.display.initTreeDisplay(cl.root);
            }
            catch (ClientException e) {
                Toolkit.showError(e.getMessage());
                Toolkit.clickButton(e.get_kind(), e.get_plain_message(), "Exit");
                System.exit(e.get_exitcode());
            }
            catch (Exception e) {
                Toolkit.showError(e.getMessage());
                e.printStackTrace();
                Toolkit.clickButton("Uncaught exception", e.getMessage(), "Exit");
                System.exit(3);
            }
        }
        catch (Exception e) {
            System.out.println("Uncaught exception: "+e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    public void clearMatches() {
        display.getProbeList().clearContents();
        display.getTreeDisplay().clearMatches();
        Toolkit.showMessage("Cleared.");
    }

    private void save(String content_type, String content, boolean ask_for_name) throws Exception {
        if (ask_for_name) iom.saveAsk(content_type, content);
        else              iom.save(content_type, content);
    }

    public void saveProbes(boolean ask_for_name) throws Exception {
        save("probe list", display.getProbeList().getProbesAsString(), ask_for_name);
    }
    public void saveDetails(boolean ask_for_name) throws Exception {
        save("details", display.getDetails().getText(), ask_for_name);
    }

    public void saveConfig() throws Exception // called at shutdown
    {
        Point     location = display.getLocationOnScreen();
        Dimension psize    = display.getPreferredScrollPaneSize();

        // System.out.println("Current location on screen: x="+location.x+" y="+location.y);
        // System.out.println("Scroll pane dimension:      width="+psize.width+" height="+psize.height+" (getPreferredScrollPaneSize)");

        String configuration =
            "LocationX="+location.x+"\n"+
            "LocationY="+location.y+"\n"+
            "Width="+psize.width+"\n"+
            "Height="+psize.height+"\n";

        System.out.println("Saving config to "+configFileName);
        iom.saveAs("config", configuration, configFileName);
    }

    public void loadConfig() throws Exception {
        try {
            String           content = "";
            FileReader       infile  = new FileReader(configFileName);
            LineNumberReader in      = new LineNumberReader(infile);

            while (true) {
                String line  = in.readLine();
                if (line == null) break;
                content     += line+"\n";
            }
            config = new ServerAnswer(content, false, false);
        }
        catch (IOException e) {
            System.out.println("Loading config: "+e.getMessage());
        }
    }


}

