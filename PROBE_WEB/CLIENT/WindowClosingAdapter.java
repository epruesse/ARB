//package de.arbhome.arb_probe_library;

import java.awt.*;
import java.awt.event.*;

public class WindowClosingAdapter
extends WindowAdapter
{
private boolean exitSystem;
public WindowClosingAdapter(boolean exitSystem)
    {
        this.exitSystem = exitSystem;
    }

public WindowClosingAdapter()
    {
        this(true);
    }
public void windowClosing(WindowEvent event)
    {
        // System.out.println("in WindowClosingAdapter::windowClosing");

        ProbesGUI mainFrame = (ProbesGUI)event.getWindow();

        mainFrame.getClient().saveConfig();
        mainFrame.setVisible(false);
        mainFrame.dispose();

//         event.getWindow().setVisible(false);
//         event.getWindow().dispose();
        if(exitSystem) {
            System.exit(0);
        }
    }
}
