// implements the action listener for the selection of ProbeList-Items

import java.awt.*;
import java.awt.event.*;


public class ProbeListActionListener implements ActionListener
{
private ProbesGUI gui;
private TreeDisplay td;

public ProbeListActionListener(ProbesGUI g)

    {

        if (g == null) {
            Toolkit.InternalError("ProbesGUIActionListener: no gui obtained");
        }

        gui = g;

        td = gui.getTreeDisplay();
        if (td == null) {
            Toolkit.InternalError("ProbesGUIActionListener: no display obtained");
        }

    }

public void actionPerformed(ActionEvent e)

    {

	String probeCandidate = new String(((ProbeList)e.getSource()).getSelectedItem());
        System.out.println("Action Source: " + probeCandidate);
	gui.getClient().matchProbes(probeCandidate);
//         System.out.println("Action ID: " + e.getID());
//         System.out.println("Action command: " + e.getActionCommand());
        // yet to fill

//         String menuName = ((MenuItem)e.getSource()).getLabel();
//         String cmdName = e.getActionCommand();

        // command listed in File menue



    }

}

