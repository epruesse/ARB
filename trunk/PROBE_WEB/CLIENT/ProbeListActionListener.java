// implements the action listener for the selection of ProbeList-Items

import java.awt.*;
import java.awt.event.*;


public class ProbeListActionListener implements /*ActionListener,*/ ItemListener 
{
private ProbesGUI gui;
private TreeDisplay td;

public ProbeListActionListener(ProbesGUI g) throws Exception
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

public void itemStateChanged(ItemEvent e) 
    {
        if (e.getStateChange() == ItemEvent.SELECTED) {
            try {
                ProbeList pl         = (ProbeList)e.getSource();
                int       selected   = pl.getSelectedIndex();
                // String    probe_info = pl.getProbeInfo(selected);
                //             System.out.println("Action Source: " + probe_info);
                // gui.getClient().matchProbes(probe_info);
                gui.getClient().matchProbes(pl.getProbe(selected));
            }
            catch (ClientException ce) {
                gui.getClient().showError(ce.getMessage());
            }
            catch (Exception ex) {
                gui.getClient().showError("in itemStateChanged: "+ex.getMessage());
                ex.printStackTrace();
            }
        }
    }

}

