import java.util.*;
import java.awt.*;

class ProbeList
extends java.awt.List
{
private int              count;
private java.util.Vector info;
private String           error;
private int              preferredHeight;
private int              preferredWidth;

public ProbeList(int width, int height)
    {
        preferredHeight = height;
        preferredWidth  = width;
        count           = 0;
        error           = null;
        setVisible(true);
    }

public String getProbeInfo(int index)
    {
        if (index<0 || index >= count) return null;
        return (String)info.elementAt(index);
    }

private void rebuildList()
    {
        removeAll();
        if (count>0) {
            for (int i = 0; i<count; i++) {
                String pinfo = getProbeInfo(i);
                int    komma = pinfo.indexOf(',');

                if (komma != -1) add(pinfo.substring(0, komma));
                else add(pinfo);
            }
        }
        else {
            add("No probe found.");
        }
    }

public void setContents(ServerAnswer parsed)
    {
        if (parsed.hasError()) {
            error = parsed.getError();
        }
        else {
            error        = null;
            String found = parsed.getValue("found");
            count        = Integer.parseInt(found);

            info = new Vector(count);

            for (int i = 0; i<count; i++) {
                String probe = parsed.getValue("probe"+i);
                // System.out.println("probe='"+probe+"' index="+i);
                info.addElement(probe);
            }
        }

        rebuildList();
    }

public boolean hasError()
    {
        return error != null;
    }

public String getError()
    {
        return error;
    }

// overloading of java.awt.component method, is called from layout manager to determine optimal size of widget
public Dimension getPreferredSize()
    {
        return new Dimension(preferredWidth, preferredHeight);
    }


}
