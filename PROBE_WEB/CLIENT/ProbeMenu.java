import java.awt.*;
import java.awt.event.*;

public class ProbeMenu extends MenuBar
{


public ProbeMenu(ActionListener al)
    {
        Menu m;
        MenuItem mi;

        m = new Menu("File");

        m.add(new MenuItem("Save"));

        m.add(new MenuItem("Dump Tree"));

        mi = new MenuItem("Quit");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_Q));
        m.add(mi);

        m.addActionListener(al);
        add(m);

        m = new Menu("Tree");

        mi = new MenuItem("Unmark Nodes");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_U));
        m.add(mi);

        mi = new MenuItem("Back to Previous Scope");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_B));
        m.add(mi);

        mi = new MenuItem("Go Down");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_H));
        m.add(mi);

        mi = new MenuItem("Enter Upper Branch");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_K));
        m.add(mi);

        mi = new MenuItem("Enter Lower Branch");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_J));
        m.add(mi);

        mi = new MenuItem("Reset Root");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_R));
        m.add(mi);

        mi = new MenuItem("Count Marked Species");
        mi.setShortcut(new MenuShortcut(KeyEvent.VK_M));
        m.add(mi);


        m.add(new MenuItem("Span Marked"));
        m.addActionListener(al);
        add(m);
        m = new Menu("Search");
        m.add(new MenuItem("Find Species"));
        m.add(new MenuItem("Find Probes"));
        m.add(new MenuItem("Keywords"));
        m.addActionListener(al);
        add(m);
        m = new Menu("Customize");
        m.add(new MenuItem("Adjust Displayed Levels"));
        m.add(new MenuItem("Change Marked Color"));
        m.add(new MenuItem("Change Source URL"));
        m.add(new MenuItem("Change Debugging behaviour"));
        m.addActionListener(al);
        add(m);
    }

public ProbeMenu()
    {
        Menu m;
        m = new Menu("File");
        m.add(new MenuItem("Save"));
        m.add(new MenuItem("Dump Tree"));
        m.add(new MenuItem("Quit"));
        add(m);
        m = new Menu("Tree");
        m.add(new MenuItem("Unmark Nodes"));
        m.add(new MenuItem("Back to Previous Scope"));
        m.add(new MenuItem("Go down"));
        m.add(new MenuItem("Enter Upper Branch"));
        m.add(new MenuItem("Enter Lower Branch"));
        m.add(new MenuItem("Reset Root"));
        m.add(new MenuItem("Count Marked Species"));
        m.add(new MenuItem("Span Marked"));
        add(m);
        m = new Menu("Search");
        m.add(new MenuItem("Find Species"));
        m.add(new MenuItem("Find Probes"));
        m.add(new MenuItem("Keywords"));
        add(m);
        m = new Menu("Customize");
        m.add(new MenuItem("Adjust Displayed Levels"));
        m.add(new MenuItem("Change Marked Color"));
        m.add(new MenuItem("Change Source URL"));
        m.add(new MenuItem("Change Debugging behaviour"));
        add(m);
    }

}
