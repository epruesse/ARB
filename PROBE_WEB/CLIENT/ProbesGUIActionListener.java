// implements the action listener for all the functions offered in ProbesGUI menu bar

import java.awt.*;
import java.awt.event.*;


public class ProbesGUIActionListener implements ActionListener
{
private ProbesGUI gui;
private TreeDisplay td;

public ProbesGUIActionListener(ProbesGUI g)

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
//         System.out.println("Action Source: " + ((MenuItem)e.getSource()).getLabel());
//         System.out.println("Action ID: " + e.getID());
//         System.out.println("Action command: " + e.getActionCommand());
        // yet to fill

        String menuName = ((MenuItem)e.getSource()).getLabel();
        String cmdName = e.getActionCommand();

        // command listed in File menue
        if (menuName.equals("File"))
            {
                if (cmdName.equals("Quit"))
                    {
                        gui.getClient().saveConfig();
                        System.exit(0);
                    }

                if (cmdName.equals("Dump Tree"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }

                if (cmdName.equals("Save"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }

            }

        if (menuName.equals("Tree"))
            {
                if (cmdName.equals("Unmark Nodes"))
                    {
                        //                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                        td.unmarkNodes();
                    }
                if (cmdName.equals("Back to Previous Scope"))
                    {
                        //   System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                        td.previousRoot();

                    }
                if (cmdName.equals("Go Down"))
                    {
                        //   System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                        td.goDown();
                    }
                if (cmdName.equals("Enter Upper Branch"))
                    {
                        //   System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                        td.enterUBr();
                    }
                if (cmdName.equals("Enter Lower Branch"))
                    {
                        //    System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                        td.enterLBr();
                    }
                if (cmdName.equals("Reset Root"))
                    {
                        //    System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                        td.resetRoot();
                    }
                if (cmdName.equals("Count Marked Species"))
                    {
                        td.countMarkedSpecies();
                        //  System.out.println("Not implemented yet: " + menuName + "/" + cmdName);


                    }
                if (cmdName.equals("Span Marked"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }

            }
        if (menuName.equals("Search"))
            {
                if (cmdName.equals("Find Species"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }
                if (cmdName.equals("Find Probes"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }

                if (cmdName.equals("Keywords"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }

            }

        if (menuName.equals("Customize"))
            {
                if (cmdName.equals("Adjust Displayed Levels"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }
                if (cmdName.equals("Change Marked Color"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }
                if (cmdName.equals("Change Source URL"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }
                if (cmdName.equals("Change Debugging behaviour"))
                    {
                        System.out.println("Not implemented yet: " + menuName + "/" + cmdName);
                    }

            }




    }

}

