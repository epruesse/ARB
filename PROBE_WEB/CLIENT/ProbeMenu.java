import java.awt.*;
import java.awt.event.*;
import java.util.*;

public class ProbeMenu extends MenuBar
{
    private ProbesGUI gui;
    
    private Vector menuEntries; // work as command as well
    private Vector menu;        // corresponding menus
    private Vector hotkeys;     // corresponding hotkeys

    private Client client() { return gui.getClient(); }
    private TreeDisplay tree_display() { return gui.getTreeDisplay(); }
    private ProbeList probe_list() { return gui.getProbeList(); }

    private void add_menu_entry(String m, String me, MenuShortcut hk) {
        menu.addElement(m);
        menuEntries.addElement(me);
        hotkeys.addElement(hk);
    }
    private void add_menu_entry(String m, String me, int hk) {
        add_menu_entry(m, me, hk == 0 ? null : new MenuShortcut(hk));
    }

    private void init_menus() {
        menuEntries = new Vector();
        menu        = new Vector();
        hotkeys     = new Vector();

        add_menu_entry("File", "Save probes",                   0);
        add_menu_entry("File", "Save probes as",                0);
        add_menu_entry("File", "Save details",                  0);
        add_menu_entry("File", "Save details as",               0);
        add_menu_entry("File", "Quit",                          KeyEvent.VK_Q);

        add_menu_entry("Mark", "Unmark all",                    KeyEvent.VK_U);
        add_menu_entry("Mark", "Count marked species",          KeyEvent.VK_M);

        add_menu_entry("Tree", "Reset root",                    KeyEvent.VK_R);
        add_menu_entry("Tree", "Go up",                         KeyEvent.VK_H);
        add_menu_entry("Tree", "Enter upper branch",            KeyEvent.VK_O);
        add_menu_entry("Tree", "Enter lower branch",            KeyEvent.VK_L);
        add_menu_entry("Tree", "Back",                          KeyEvent.VK_B);
        add_menu_entry("Tree", "Jump to marked",                KeyEvent.VK_J);

        add_menu_entry("Fold", "Collapse all",                  0);
        add_menu_entry("Fold", "Collapse unmarked",             0);
        add_menu_entry("Fold", "Collapse fully marked",         0);
        add_menu_entry("Fold", "Collapse partial marked",       0);
        add_menu_entry("Fold", "Expand all",                    0);
        add_menu_entry("Fold", "Expand unmarked",               0);
        add_menu_entry("Fold", "Expand fully marked",           0);
        add_menu_entry("Fold", "Expand partial marked",         KeyEvent.VK_P);
        add_menu_entry("Fold", "Smart expand",                  KeyEvent.VK_X);
        add_menu_entry("Fold", "Smart collapse",                KeyEvent.VK_C);
        add_menu_entry("Fold", "Show marked",                   KeyEvent.VK_Z);
        
        add_menu_entry("Help", "Help",                          0);
    }

    public void performCommand(String cmd) throws Exception {
        if (cmd.equals("Quit")) {
            client().saveConfig();
            System.exit(0);
        }

        else if (cmd.equals("Save probes as"))          client().saveProbes(true);
        else if (cmd.equals("Save probes"))             client().saveProbes(false);
        else if (cmd.equals("Save details as"))         client().saveDetails(true);
        else if (cmd.equals("Save details"))            client().saveDetails(false);

        else if (cmd.equals("Unmark all"))              tree_display().unmarkNodes();
        else if (cmd.equals("Count marked species"))    tree_display().countMarkedSpecies();

        else if (cmd.equals("Reset root"))              tree_display().resetRoot();
        else if (cmd.equals("Go up"))                   tree_display().goUp();
        else if (cmd.equals("Enter upper branch"))      tree_display().enterUBr();
        else if (cmd.equals("Enter lower branch"))      tree_display().enterLBr();
        else if (cmd.equals("Back"))                    tree_display().previousRoot();
        else if (cmd.equals("Jump to marked"))          tree_display().gotoRootOfMarked();

        else if (cmd.equals("Collapse all"))            tree_display().foldAll();
        else if (cmd.equals("Collapse unmarked"))       tree_display().foldUnmarked();
        else if (cmd.equals("Collapse fully marked"))   tree_display().foldCompleteMarked();
        else if (cmd.equals("Collapse partial marked")) tree_display().foldPartiallyMarked();
        else if (cmd.equals("Expand all"))              tree_display().unfoldAll();
        else if (cmd.equals("Expand unmarked"))         tree_display().unfoldUnmarked();
        else if (cmd.equals("Expand fully marked"))     tree_display().unfoldCompleteMarked();
        else if (cmd.equals("Expand partial marked"))   tree_display().unfoldPartiallyMarked();
        else if (cmd.equals("Smart expand"))            tree_display().smartUnfold();
        else if (cmd.equals("Smart collapse"))          tree_display().smartFold();
        else if (cmd.equals("Show marked"))             tree_display().unfoldMarkedFoldRest();
        
        else if (cmd.equals("Help"))                    gui.showHelp();

        else {
            System.out.println("Command not implemented yet: '"+cmd+"'");
        }
    }

    private void build_menus(ActionListener al) {
        int    size     = menuEntries.size();
        String lastMenu = "";
        Menu   m        = null;
        Set    usedKeys = new HashSet();

        for (int idx = 0; idx<size; ++idx) {
            String menu_name = (String)menu.get(idx);
            if (!menu_name.equals(lastMenu)) {
                if (m != null) add(m);
                m = new Menu(menu_name);
                m.addActionListener(al);
                lastMenu = menu_name;
            }

            String       entry  = (String)menuEntries.get(idx);
            MenuItem     item   = new MenuItem(entry);
            MenuShortcut hotkey = (MenuShortcut)hotkeys.get(idx);
            if (hotkey != null) {
                item.setShortcut(hotkey);
                if (usedKeys.contains(hotkey)) {
                    System.out.println("Hotkey duplicated in '"+entry+"'");
                }
                usedKeys.add(hotkey);
            }
            m.add(item);
        }

        if (m != null) add(m);

        menuEntries = null;
        menu        = null;
        hotkeys     = null;
    }


    public ProbeMenu(ProbesGUI gui) {
        this.gui = gui;
        init_menus();
        build_menus(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    String cmdName = e.getActionCommand();
                    try {
                        performCommand(cmdName);
                    }
                    catch (ClientException ce) {
                        Toolkit.showError(ce.getMessage());
                    }
                    catch (Exception ex) {
                        Toolkit.showError("in ProbeMenu/ActionListener: "+ex.getMessage());
                        ex.printStackTrace();
                    }
                }
            });

    }
}
